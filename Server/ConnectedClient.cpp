#include "pch.h"
#include <array>
#include <functional>
#include "ConnectedClient.h"


ConnectedClient::ConnectedClient(uint16_t ID, sockaddr_in clientDesc, int clientLen)
	: readSocket(INVALID_SOCKET)
	, writeSocket(INVALID_SOCKET)
	, ID(ID)
	, IP(clientDesc.sin_addr.s_addr)
	, port(ntohs(clientDesc.sin_port))
	, started(false)
{
	// Collecting data about client IP, port and host

	inet_ntop(AF_INET, &(clientDesc.sin_addr), IP_str, 16);                          // get IP addr string
	getnameinfo((sockaddr*)&clientDesc, clientLen, host, NI_MAXHOST, NULL, NULL, 0); // get host

	log("Client %d (IP: %s, host: %s) connected on port %d", ID, IP_str, host, port);
}

ConnectedClient::~ConnectedClient() {
	if(started) disconnect();

	if (!receivedPackets.empty()) {
#ifdef _DEBUG
		msg_mutex.lock();
		cout << "All received data:" << endl;
		size_t i = 1;

		for (auto& it : receivedPackets)
			cout << i++ << ':' << endl << "  size: " << it->size << endl << "  data: " << it->data << endl;
		msg_mutex.unlock();
#endif

		receivedPackets.clear();
	}
}


bool     ConnectedClient::isRunning() { return this->started; }
uint16_t ConnectedClient::getID()     { return this->ID; }
uint16_t ConnectedClient::getIP_u16() { return this->IP; }
char*    ConnectedClient::getIP_str() { return this->IP_str; }

// Первое рукопожатие с соединенным клиентом
int ConnectedClient::first_handshake(SOCKET socket)
{
	// Присвоить сокет на запись
	setState(ClientState::FirstHandshake);

	writeSocket = socket;

	return 0;
}


// Второе рукопожатие с соединенным клиентом
int ConnectedClient::second_handshake(SOCKET socket)
{
	// Присвоить сокет на чтение
	// Создать потоки-обработчики
	// Отправить пакет ACK, подтвердить получение
	setState(ClientState::SecondHandshake);

	readSocket = socket;
	started = true;
	createThreads();

	//TODO: Отправить пакет ACK, подтвердить получение

	return 0;
}

void ConnectedClient::createThreads()
{
	sender = std::thread(&ConnectedClient::senderThread, this);
	sender.detach();

	receiver = std::thread(&ConnectedClient::receiverThread, this);
	receiver.detach();
}


// Обработка пакета ACK
int ConnectedClient::ack_handler(PacketPtr packet)
{
	log_raw(packet->data);
	return 0;
}


// Обработка любого входящего пакета
int ConnectedClient::any_packet_handler(PacketPtr packet)
{
	log_raw(packet->data);
	return 0;
}


// Обработка пакета из очереди
int ConnectedClient::handlePacketIn(std::function<int(PacketPtr)> handler)
{
	PacketPtr packet;

	int err = receiveData(packet);
	if (err)
		return err; // Произошла ошибка

	// Добавить пакет
	receivedPackets.push_back(packet);

	// Обработка пришедшего пакета
	return handler(packet);
}


int ConnectedClient::handlePacketOut(PacketPtr packet)
{
	// Обработка пакета из очереди

	if (sendData(packet))
		return 1;

	if (packet->needACK) {
		if (handlePacketIn(std::bind(&ConnectedClient::ack_handler, this, std::placeholders::_1)))
			return 2;
	}

	return 0;
}


// Поток обработки входящих пакетов
void ConnectedClient::receiverThread()
{
	// Set thread description
	setThreadDesc(L"Receiver");

	while (started) {
		int err = handlePacketIn(std::bind(&ConnectedClient::any_packet_handler, this, std::placeholders::_1));
		if (err) {
			if (err > 0) break;    // Критическая ошибка или соединение сброшено
			else         continue; // Неудачный пакет, продолжить прием        
		}
	}

	// Закрываем поток
	log("Closing receiver thread %d", receiver.get_id());
}


// Поток отправки пакетов
void ConnectedClient::senderThread()
{
	// Set thread description
	setThreadDesc(L"Sender");

	while (started) {
		// Обработать основные пакеты
		while (!mainPackets.empty()) {
			PacketPtr packet = mainPackets.back();

			if (handlePacketOut(packet)) {
				log_raw("Packet not confirmed, adding to sync queue"); // TODO: писать Packet ID
				syncPackets.push_back(packet);
			}
			
			mainPackets.pop();
		}

		// Обработать пакеты, не подтвержденные клиентами
		auto packetIt = syncPackets.begin();
		while (packetIt != syncPackets.end()) {
			if (handlePacketOut(*packetIt)) {
				log_raw("Packet not confirmed"); // TODO: писать Packet ID
				packetIt++;
			}
			else packetIt = syncPackets.erase(packetIt);
		}

		Sleep(100);
	}

	// Закрываем поток
	log("Closing sender thread %d", sender.get_id());
}

int ConnectedClient::receiveData(PacketPtr& dest)
{
	// Receive until the peer closes the connection
	setState(ClientState::Receive);

	std::array<char, NET_BUFFER_SIZE> respBuff;
	int respSize = recv(readSocket, respBuff.data(), NET_BUFFER_SIZE, 0);

	if (respSize > 0) {
		//Записываем данные от клиента
		dest = std::make_shared<Packet>(respBuff.data(), respSize, false);
	}
	else if (!respSize) {
		log_raw("Connection closed");
		return 1;
	}
	else {
		wsa_print_err();
		return -1;
	}

	return 0;
}

int ConnectedClient::sendData(PacketPtr packet) {
	// Send an initial buffer
	setState(ClientState::Send);

	/*if (!packet->data) { //Ввести данные
		std::string req;

		log_raw_nonl("Type what you want to send to client: \n>";
		std::getline(std::cin, req);

		packet = std::make_shared<Packet>(req.c_str(), req.size());
	}*/

	if (send(writeSocket, packet->data, packet->size, 0) == SOCKET_ERROR) {
		wsa_print_err();
		return 1;
	}

	sendedPackets.push_back(packet);

	return 0;
}

int ConnectedClient::disconnect() {
	if (!started) return 0;

	started = false;

	// Closing handler threads

	if (receiver.joinable())
		receiver.join();

	if (sender.joinable())
		sender.join();

	// Shutdown the connection since no more data will be sent
	setState(ClientState::Shutdown);

	if (readSocket != INVALID_SOCKET)
		if (shutdown(readSocket, SD_BOTH) == SOCKET_ERROR)
			log("Error while shutdowning read socket: %d", WSAGetLastError());

	if (writeSocket != INVALID_SOCKET)
		if (shutdown(writeSocket, SD_BOTH) == SOCKET_ERROR)
			log("Error while shutdowning write socket: %d", WSAGetLastError());

	// Close the socket
	setState(ClientState::CloseSockets);

	if(readSocket != INVALID_SOCKET)
		if (closesocket(readSocket) == SOCKET_ERROR)
			log("Error while closing read socket: %d", WSAGetLastError());
	
	if (writeSocket != INVALID_SOCKET)
		if (closesocket(writeSocket) == SOCKET_ERROR)
			log("Error while closing write socket: %d", WSAGetLastError());

	log("Connected client %d was stopped", ID);

	return 0;
}

void ConnectedClient::setState(ClientState state)
{
#ifdef _DEBUG
	const char* state_desc;

#define PRINT_STATE(X) case ClientState:: X: \
	state_desc = #X; \
	break;

	switch (state) {
		PRINT_STATE(FirstHandshake);
		PRINT_STATE(SecondHandshake);
		PRINT_STATE(Send);
		PRINT_STATE(Receive);
		PRINT_STATE(Shutdown);
		PRINT_STATE(CloseSockets);
	default:
		log("Unknown state: %d", (int)state);
		return;
}
#undef PRINT_STATE

	log("State changed to: %s", state_desc);
#endif

	this->state = state;
}