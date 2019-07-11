#include "pch.h"

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

	cout << "Client (IP: " << IP_str << ", host: " << host << ") connected on port " << port << endl;

	setState(CLIENT_STATE::OK);
}

ConnectedClient::~ConnectedClient() {
	if(started) disconnect();

	error_code = WSAGetLastError();

	// Вывод сообщения об ошибке

	if (state != CLIENT_STATE::OK) cout << "state " << (int)state << " - error: " << error_code << endl;
	else if (!receivedPackets.empty()) {
#ifdef _DEBUG
		cout << "All received data:" << endl;
		size_t i = 1;

		for (auto& it : receivedPackets) cout << i++ << ':' << endl << "  size: " << it->size << endl << "  data: " << it->data << endl;
#endif

		receivedPackets.clear();
	}
}

void ConnectedClient::createThreads() {
	sender = std::thread(&ConnectedClient::senderThread, this);
	sender.detach();

	receiver = std::thread(&ConnectedClient::receiverThread, this);
	receiver.detach();
}

int ConnectedClient::handlePacketIn(std::function<int(PacketPtr)>handler) { // Обработка пакета из очереди
	auto packet = std::make_shared<Packet>(); // TODO: сделать с этим что-нибудь

	if (int err = receiveData(packet)) return err; // Произошла ошибка

	// Добавить пакет
	receivedPackets.push_back(packet);

	// Обработка пришедшего пакета
	return handler(packet);
}

int handle1(PacketPtr packet) { // Обработка пакета ACK
	cout << packet->data << endl;
}

int handle2(PacketPtr packet) { // Обработка любого входящего пакета
	cout << packet->data << endl;
}

int ConnectedClient::handlePacketOut(PacketPtr packet) { // Обработка пакета из очереди
	if (sendData(packet)) return 1;

	if (packet->needACK) {
		if (handlePacketIn(handle1)) return 2;
	}

	return 0;
}

void ConnectedClient::receiverThread() { // Поток обработки входящих пакетов
	while (started) {
		if (int err = handlePacketIn(handle2)) {
			if (err > 0) break;    // Критическая ошибка или соединение сброшено
			else         continue; // Неудачный пакет, продолжить прием        
		}
	}

	// Закрываем поток
	cout << "Closing receiver thread " << receiver.get_id() << endl;
}

void ConnectedClient::senderThread() { // Поток отправки пакетов
	while (started) {
		// Обработать основные пакеты
		while (!mainPackets.empty()) {
			PacketPtr packet = mainPackets.back();

			if (handlePacketOut(packet)) {
				cout << "Packet not confirmed, adding to sync queue" << endl; // TODO: писать Packet ID
				syncPackets.push_back(packet);
			}
			
			mainPackets.pop();
		}

		// Обработать пакеты, не подтвержденные клиентами
		auto packetIt = syncPackets.begin();
		while (packetIt != syncPackets.end()) {
			if (handlePacketOut(*packetIt)) {
				cout << "Packet not confirmed" << endl; // TODO: писать Packet ID
				packetIt++;
			}
			else packetIt = syncPackets.erase(packetIt);
		}

		Sleep(100);
	}

	// Закрываем поток
	cout << "Closing sender thread " << sender.get_id() << endl;
}

int ConnectedClient::receiveData(PacketPtr dest) {
	// Receive until the peer closes the connection
	setState(CLIENT_STATE::RECEIVE);

	char respBuff[NET_BUFFER_SIZE];

	int respSize = recv(readSocket, respBuff, NET_BUFFER_SIZE, 0);

	if (respSize > 0) { //Записываем данные от клиента
		dest->data = respBuff;
		dest->size = respSize;
		dest->needACK = false;
	}
	else if (!respSize) {
		cout << "Connection closed" << endl;
		return 1;
	}
	else return -1;

	setState(CLIENT_STATE::OK);
	return 0;
}

int ConnectedClient::sendData(PacketPtr packet) {
	// Send an initial buffer
	setState(CLIENT_STATE::SEND);

	/*if (!packet->data) { //Ввести данные
		std::string req;

		cout << "Type what you want to send to client: " << endl << '>';
		std::getline(std::cin, req);

		packet = std::make_shared<Packet>(req.c_str(), req.size());
	}*/

	if (send(writeSocket, packet->data, packet->size, 0) == SOCKET_ERROR) return 1;

	sendedPackets.push_back(packet);

	setState(CLIENT_STATE::OK);
	return 0;
}

int ConnectedClient::disconnect() {
	if (!started) return 0;

	// Shutdown the connection since no more data will be sent
	setState(CLIENT_STATE::SHUTDOWN);

	if (shutdown(readSocket,  SD_BOTH) == SOCKET_ERROR) cout << "Error while shutdowning connection" << endl;
	if (shutdown(writeSocket, SD_BOTH) == SOCKET_ERROR) cout << "Error while shutdowning connection" << endl;

	// Close the socket
	setState(CLIENT_STATE::CLOSE_SOCKET);

	if (closesocket(readSocket)  == SOCKET_ERROR) cout << "Error while closing read socket" << endl;
	if (closesocket(writeSocket) == SOCKET_ERROR) cout << "Error while closing write socket" << endl;

	cout << "Connected client was stopped" << endl;

	started = false;

	setState(CLIENT_STATE::OK);
	return 0;
}

void ConnectedClient::setState(CLIENT_STATE state)
{
#ifdef _DEBUG
	const char state_desc[32];

#define PRINT_STATE(X) case SERVER_STATE:: X: \
	state_desc = #X; \
	break;

	switch (state) {
		PRINT_STATE(OK);
		PRINT_STATE(SEND);
		PRINT_STATE(RECEIVE);
		PRINT_STATE(SHUTDOWN);
		PRINT_STATE(CLOSE_SOCKET);
	default:
		std::cout << "Unknown state: " << (int)state << std::endl;
		return;
}
#undef PRINT_STATE

	std::cout << "State changed to: " << state_desc << std::endl;
#endif

	this->state = state;
}