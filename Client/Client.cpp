#include "pch.h"

#include "Client.h"

Packet::Packet(const char* data, size_t size, bool needACK)
	: needACK(needACK)
	, size(size)
{
	this->data = new char[size + 2];
	memcpy(this->data, data, size);
	this->data[size] = NULL; //NULL-terminator

#ifdef _DEBUG
	cout << "Packet: " << size << ", data: " << this->data << endl;;
#endif
}

Packet::~Packet() {
	delete[] this->data;
}


Client::Client(PCSTR IP, uint16_t readPort, uint16_t writePort)
	: readSocket(INVALID_SOCKET)
	, writeSocket(INVALID_SOCKET)
	, readPort(readPort)
	, writePort(writePort)
	, IP(IP)
	, started(false)
{
	setState(CLIENT_STATE::OK);
}

Client::~Client() {
	if (started) disconnect();

	if (state > CLIENT_STATE::INIT_WINSOCK) error_code = WSAGetLastError();

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


int Client::init() {
	// Initialize Winsock
	setState(CLIENT_STATE::INIT_WINSOCK);

	WSADATA wsaData;

	if (error_code = WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) return 1;

	if (int err = handshake()) return err;

	setState(CLIENT_STATE::OK);
	return 0;
}

int Client::connect2server(uint16_t port) {
	sockaddr_in socketDesc;

	socketDesc.sin_family = AF_INET;
	socketDesc.sin_port = htons(port);
	inet_pton(AF_INET, IP, &(socketDesc.sin_addr.s_addr));

	readSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (readSocket == INVALID_SOCKET) return 1;

	// Connect to server
	if (connect(readSocket, (SOCKADDR*)&socketDesc, sizeof(socketDesc)) == SOCKET_ERROR) return 2;

	setState(CLIENT_STATE::OK);
	return 0;
}

int Client::handshake() {
	// Create a read socket that receiving data from server (UDP protocol)
	setState(CLIENT_STATE::CREATE_READ_SOCKET);

	if(connect2server(readPort)) return 1;

	cout << "The client can read the data from the port " << readPort << endl;

	// Create a write socket that sending data to the server (UDP protocol)
	setState(CLIENT_STATE::CREATE_WRITE_SOCKET);

	if (connect2server(writePort)) return 1;

	cout << "The client can write the data to the port " << writePort << endl;

	//TODO: receive ACK from the server

	createThreads();

	started = true;

	setState(CLIENT_STATE::OK);
	return 0;
}

void Client::createThreads() {
	sender = std::thread(&Client::senderThread, this);
	sender.detach();

	receiver = std::thread(&Client::receiverThread, this);
	receiver.detach();
}

int Client::handleACK(PacketPtr packet) { // Обработка пакета ACK
	cout << packet->data << endl;
}

int Client::handleAll(PacketPtr packet) { // Обработка любого входящего пакета
	cout << packet->data << endl;
}

int Client::handlePacketIn(std::function<int(PacketPtr)> handler) { // Обработка пакета из очереди
	auto packet = std::make_shared<Packet>(); // TODO: сделать с этим что-нибудь

	if (int err = receiveData(packet)) return err; // Произошла ошибка

	// Добавить пакет
	receivedPackets.push_back(packet);

	// Обработка пришедшего пакета
	return handler(packet);
}

int Client::handlePacketOut(PacketPtr packet) { // Обработка пакета из очереди
	if (sendData(packet)) return 1;

	if (packet->needACK) {
		if (handlePacketIn(this->handleACK)) return 2; //FIX ME
	}

	return 0;
}

void Client::receiverThread() { // Поток обработки входящих пакетов
	while (started) {
		if (int err = handlePacketIn(this->handleAll)) {  //FIX ME
			if (err > 0) break;    // Критическая ошибка или соединение сброшено
			else         continue; // Неудачный пакет, продолжить прием        
		}
	}

	// Закрываем поток
	cout << "Closing receiver thread " << receiver.get_id() << endl;
}

void Client::senderThread() { // Поток отправки пакетов
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

		// Обработать пакеты, не подтвержденные сервером
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

int Client::sendData(PacketPtr packet) {
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

int Client::receiveData(PacketPtr dest) {
	// Receive until the peer closes the connection
	setState(CLIENT_STATE::RECEIVE);

	char respBuff[NET_BUFFER_SIZE];

	int respSize = recv(readSocket, respBuff, NET_BUFFER_SIZE, 0);

	if (respSize > 0) { //Записываем данные от сервера
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

int Client::disconnect() {
	if (!started) return 0;

	// Shutdown the connection since no more data will be sent
	setState(CLIENT_STATE::SHUTDOWN);

	if (shutdown(readSocket,  SD_BOTH) == SOCKET_ERROR) cout << "Error while shutdowning read socket"  << endl;
	if (shutdown(writeSocket, SD_BOTH) == SOCKET_ERROR) cout << "Error while shutdowning write socket" << endl;

	// Close the socket
	setState(CLIENT_STATE::CLOSE_SOCKET);

	if (closesocket(readSocket)  == SOCKET_ERROR) cout << "Error while closing read socket" << endl;
	if (closesocket(writeSocket) == SOCKET_ERROR) cout << "Error while closing write socket" << endl;

	WSACleanup();

	cout << "The client was stopped" << endl;

	started = false;

	setState(CLIENT_STATE::OK);
	return 0;
}

void Client::setState(CLIENT_STATE state)
{
#ifdef _DEBUG
#define PRINT_STATE(X) case CLIENT_STATE::X:           \
	std::cout << "State changed to: " #X << std::endl; \
	break;

	switch (state) {
		PRINT_STATE(OK);
		PRINT_STATE(INIT_WINSOCK);
		PRINT_STATE(CREATE_READ_SOCKET);
		PRINT_STATE(CREATE_WRITE_SOCKET);
		PRINT_STATE(SEND);
		PRINT_STATE(SHUTDOWN);
		PRINT_STATE(RECEIVE);
		PRINT_STATE(CLOSE_SOCKET);
	default:
		std::cout << "Unknown state: " << (int)state << std::endl;
		return;
	}
#undef PRINT_STATE
#endif

	this->state = state;
}