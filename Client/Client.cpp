#include "pch.h"
#include <string_view>
#include "Client.h"


std::mutex msg_mutex;

void log_raw_nonl(const char* str) {
	msg_mutex.lock();
	cout << str;
	msg_mutex.unlock();
}

void log_raw(const char* str) {
	msg_mutex.lock();
	cout << str << endl;
	msg_mutex.unlock();
}


void log_nonl(const char* fmt, ...) {
	msg_mutex.lock();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	msg_mutex.unlock();
}

void log(const char* fmt, ...) {
	msg_mutex.lock();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	cout << endl;
	msg_mutex.unlock();
}

Packet::Packet(const char* data, size_t size, bool needACK)
	: needACK(needACK)
	, size(size)
{
	this->data = new char[size];
	memcpy(this->data, data, size);

#ifdef _DEBUG
	log("Packet: %d bytes, data: %s", size, std::string_view(data, size));
	//cout << *this << endl;
#endif
}

Packet::~Packet() { delete[] this->data; }


//std::ostream& operator<< (std::ostream& os, const Packet& packet)
//{
//	os << "Packet: " << packet.size << ", " << "data: " << std::string_view(packet.data, packet.size);
//	return os;
//}


void __wsa_print_err(const char* file, uint16_t line) { log("%s:%d - WSA Error %d", file, line, WSAGetLastError()); }


Client::Client(PCSTR IP, uint16_t readPort, uint16_t writePort)
	: readSocket(INVALID_SOCKET)
	, writeSocket(INVALID_SOCKET)
	, readPort(readPort)
	, writePort(writePort)
	, IP(IP)
	, started(false)
{}

Client::~Client() {
	if (started) disconnect();

	if (!receivedPackets.empty()) {
#ifdef _DEBUG
		msg_mutex.lock();
		cout << "All received data:" << endl;
		size_t i = 1;

		for (auto& it : receivedPackets) cout << i++ << ':' << endl << "  size: " << it->size << endl << "  data: " << it->data << endl;
		msg_mutex.unlock();
#endif

		receivedPackets.clear();
	}
}


int Client::init() {
	// Initialize Winsock
	setState(ClientState::InitWinSock);

	WSADATA wsaData;

	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != NO_ERROR) {
		wsa_print_err();
		return 1;
	}

	if (err = handshake()) return err;

	return 0;
}

SOCKET Client::connect2server(uint16_t port) {
	SOCKET result = INVALID_SOCKET;

	sockaddr_in socketDesc;

	socketDesc.sin_family = AF_INET;
	socketDesc.sin_port = htons(port);
	inet_pton(AF_INET, IP, &(socketDesc.sin_addr.s_addr));

	//result = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	result = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (result == INVALID_SOCKET) {
		wsa_print_err();
		return INVALID_SOCKET;
	}

	// Connect to server
	if (connect(result, (SOCKADDR*)&socketDesc, sizeof(socketDesc)) == SOCKET_ERROR) {
		wsa_print_err();
		return INVALID_SOCKET;
	}

	return result;
}

int Client::handshake() {
	// Create a read socket that receiving data from server (UDP protocol)
	setState(ClientState::CreateReadSocket);

	readSocket = connect2server(readPort);
	if(readSocket == INVALID_SOCKET) return 1;

	log("The client can read the data from the port %d", readPort);

	// Create a write socket that sending data to the server (UDP protocol)
	setState(ClientState::CreateWriteSocket);

	writeSocket = connect2server(writePort);
	if (writeSocket == INVALID_SOCKET) return 1;

	log("The client can write the data to the port %d", writePort);

	//TODO: receive ACK from the server

	started = true;

	createThreads();

	return 0;
}

void Client::createThreads() {
	sender = std::thread(&Client::senderThread, this);
	sender.detach();

	receiver = std::thread(&Client::receiverThread, this);
	receiver.detach();
}

int Client::ack_handler(PacketPtr packet) { // Обработка пакета ACK
	log_raw(packet->data);
	return 0;
}

int Client::any_packet_handler(PacketPtr packet) { // Обработка любого входящего пакета
	log_raw(packet->data);
	return 0;
}

int Client::handlePacketIn(std::function<int(PacketPtr)> handler) { // Обработка пакета из очереди
	PacketPtr packet;

	if (int err = receiveData(packet)) return err; // Произошла ошибка

	// Добавить пакет
	receivedPackets.push_back(packet);

	// Обработка пришедшего пакета
	return handler(packet);
}

int Client::handlePacketOut(PacketPtr packet) { // Обработка пакета из очереди
	if (sendData(packet)) return 1;

	if (packet->needACK) {
		if (handlePacketIn(std::bind(&Client::ack_handler, this, std::placeholders::_1)))
			return 2;
	}

	return 0;
}

void Client::receiverThread() { // Поток обработки входящих пакетов
	while (started) {
		if (int err = handlePacketIn(std::bind(&Client::any_packet_handler, this, std::placeholders::_1))) {
			if (err > 0) break;    // Критическая ошибка или соединение сброшено
			else         continue; // Неудачный пакет, продолжить прием        
		}
	}

	// Закрываем поток
	log("Closing receiver thread %d", receiver.get_id());
}

void Client::senderThread() { // Поток отправки пакетов
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

		// Обработать пакеты, не подтвержденные сервером
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

int Client::sendData(PacketPtr packet) {
	// Send an initial buffer
	setState(ClientState::Send);

	/*if (!packet->data) { //Ввести данные
		std::string req;

		log_raw("Type what you want to send to client: \n>");

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

int Client::receiveData(PacketPtr dest) {
	// Receive until the peer closes the connection
	setState(ClientState::Receive);

	char respBuff[NET_BUFFER_SIZE];

	int respSize = recv(readSocket, respBuff, NET_BUFFER_SIZE, 0);

	if (respSize > 0) { //Записываем данные от сервера
		dest->data = respBuff;
		dest->size = respSize;
		dest->needACK = false;
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

int Client::disconnect() {
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
		if (shutdown(readSocket,  SD_BOTH) == SOCKET_ERROR)
			log("Error while shutdowning read socket: %d", WSAGetLastError());

	if (writeSocket != INVALID_SOCKET)
		if (shutdown(writeSocket, SD_BOTH) == SOCKET_ERROR)
			log("Error while shutdowning write socket: %d", WSAGetLastError());

	// Close the socket
	setState(ClientState::CloseSockets);
	if (readSocket != INVALID_SOCKET)
		if (closesocket(readSocket)  == SOCKET_ERROR)
			log("Error while closing read socket: %d", WSAGetLastError());

	if (writeSocket != INVALID_SOCKET)
		if (closesocket(writeSocket) == SOCKET_ERROR)
			log("Error while closing write socket: %d", WSAGetLastError());

	WSACleanup();

	log_raw("The client was stopped");

	return 0;
}

void Client::setState(ClientState state)
{
#ifdef _DEBUG
	const char* state_desc;

#define PRINT_STATE(X) case CLIENT_STATE::X: \
	state_desc = #X;                         \
	break;

	switch (state) {
		PRINT_STATE(InitWinSock);
		PRINT_STATE(CreateReadSocket);
		PRINT_STATE(CreateWriteSocket);
		PRINT_STATE(Send);
		PRINT_STATE(Shutdown);
		PRINT_STATE(Receive);
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