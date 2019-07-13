#include "pch.h"
#include <array>
#include <string_view>
#include "Client.h"


std::mutex msg_mutex;
static std::mutex pf_mutex;

static void printThreadDesc() {
	std::wstring threadDesc;
	threadDesc.reserve(32);

	wchar_t* desc;
	getThreadDesc(&desc);

	wsprintfW(threadDesc.data(), L"[%s]", desc);
	wprintf(L"%-11s", threadDesc.data());
}

static void setConsoleColor(ConsoleColor color) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (uint16_t)color); }


void log_raw_nonl(std::string_view str) {
	msg_mutex.lock();
	printThreadDesc();
	cout << str;
	msg_mutex.unlock();
}

void log_raw_colored(ConsoleColor color, std::string_view str) {
	msg_mutex.lock();
	printThreadDesc();
	setConsoleColor(color);
	cout << str << endl;
	setConsoleColor(ConsoleColor::Default);
	msg_mutex.unlock();
}

void log_raw(std::string_view str) {
	msg_mutex.lock();
	printThreadDesc();
	cout << str << endl;
	msg_mutex.unlock();
}


void log_nonl(const char* fmt, ...) {
	msg_mutex.lock();
	printThreadDesc();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	msg_mutex.unlock();
}

void log_colored(ConsoleColor color, const char* fmt, ...) {
	msg_mutex.lock();
	printThreadDesc();
	setConsoleColor(color);
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	setConsoleColor(ConsoleColor::Default);
	cout << endl;
	msg_mutex.unlock();
}

void log(const char* fmt, ...) {
	msg_mutex.lock();
	printThreadDesc();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	cout << endl;
	msg_mutex.unlock();
}


Packet::Packet(uint32_t ID, const char* data, size_t size, bool needACK)
	: ID(ID)
	, size(size)
	, needACK(needACK)
{
	this->data = new char[size];
	memcpy(this->data, data, size);
}

Packet::~Packet() { delete[] this->data; }


//std::ostream& operator<< (std::ostream& os, const Packet& packet)
//{
//	os << "Packet: " << packet.size << ", " << "data: " << std::string_view(packet.data, packet.size);
//	return os;
//}

PacketFactory::PacketFactory()
	: ID(0)
{}

PacketPtr PacketFactory::create(const char* data, size_t size, bool needACK)
{
	pf_mutex.lock();
	PacketPtr result = std::make_shared<Packet>(this->ID++, data, size, needACK);
	pf_mutex.unlock();
	return result;
}

PacketFactory packetFactory;


const char* __get_filename(const char* file) {
	const char* last_slash = strrchr(file, '\\');
	return last_slash ? last_slash + 1 : file;
}

void __wsa_print_err(const char* file, int line)
{
	msg_mutex.lock();
	int err = WSAGetLastError();

	char err_msg[256] = "";
	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
		err_msg, sizeof(err_msg), NULL);

	printf("%s:%d - WSA Error %d:\n%s", file, line, err, err_msg);
	msg_mutex.unlock();
}

// Set description to current thread
void setThreadDesc(const wchar_t* desc) { SetThreadDescription(GetCurrentThread(), desc); }

// Get description of current thread
void getThreadDesc(wchar_t** dest) { GetThreadDescription(GetCurrentThread(), dest); }


Client::Client(PCSTR IP, uint16_t readPort, uint16_t writePort)
	: readSocket(INVALID_SOCKET)
	, writeSocket(INVALID_SOCKET)
	, readPort(readPort)
	, writePort(writePort)
	, IP(IP)
	, started(false)
{
	packetFactory = PacketFactory();
}

Client::~Client() {
	if (started) disconnect();

	receivedPackets.clear();
	sendedPackets.clear();
	syncPackets.clear();

	while (!mainPackets.empty())
		mainPackets.pop();
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

	if (err = handshake())
		return err;

	return 0;
}

int Client::disconnect() {
	if (!started)
		return 0;

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
			log_colored(ConsoleColor::DangerHighlighted, "Error while shutdowning read socket: %d", WSAGetLastError());

	if (writeSocket != INVALID_SOCKET)
		if (shutdown(writeSocket, SD_BOTH) == SOCKET_ERROR)
			log_colored(ConsoleColor::DangerHighlighted, "Error while shutdowning write socket: %d", WSAGetLastError());

	// Close the socket
	setState(ClientState::CloseSockets);
	if (readSocket != INVALID_SOCKET)
		if (closesocket(readSocket) == SOCKET_ERROR)
			log_colored(ConsoleColor::DangerHighlighted, "Error while closing read socket: %d", WSAGetLastError());

	if (writeSocket != INVALID_SOCKET)
		if (closesocket(writeSocket) == SOCKET_ERROR)
			log_colored(ConsoleColor::DangerHighlighted, "Error while closing write socket: %d", WSAGetLastError());

	WSACleanup();

	log_raw_colored(ConsoleColor::InfoHighlighted, "The client was stopped");

	return 0;
}


SOCKET Client::connect2server(uint16_t port) {
	SOCKET result = INVALID_SOCKET;

	sockaddr_in socketDesc;

	socketDesc.sin_family = AF_INET;
	socketDesc.sin_port = htons(port);
	inet_pton(AF_INET, IP, &(socketDesc.sin_addr.s_addr));

	//result = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	result = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
	if(readSocket == INVALID_SOCKET)
		return 1;

	log_colored(ConsoleColor::SuccessHighlighted, "The client can read the data from the port %d", readPort);

	// Create a write socket that sending data to the server (UDP protocol)
	setState(ClientState::CreateWriteSocket);

	writeSocket = connect2server(writePort);
	if (writeSocket == INVALID_SOCKET)
		return 1;

	log_colored(ConsoleColor::SuccessHighlighted, "The client can write the data to the port %d", writePort);

	//TODO: receive ACK from the server

	started = true;

	createThreads();

	return 0;
}

void Client::createThreads() {
	receiver = std::thread(&Client::receiverThread, this);
	receiver.detach();

	sender = std::thread(&Client::senderThread, this);
	sender.detach();
}

int Client::ack_handler(PacketPtr packet) { // Обработка пакета ACK
	log_raw(std::string_view(packet->data, packet->size));
	return 0;
}

int Client::any_packet_handler(PacketPtr packet) { // Обработка любого входящего пакета
	log_raw(std::string_view(packet->data, packet->size));
	return 0;
}

int Client::handlePacketIn(std::function<int(PacketPtr)> handler) { // Обработка пакета из очереди
	PacketPtr packet;

	if (int err = receiveData(packet))
		return err; // Произошла ошибка

	// Обработка пришедшего пакета
	return handler(packet);
}

int Client::handlePacketOut(PacketPtr packet) { // Обработка пакета из очереди
	if (sendData(packet))
		return 1;

	if (packet->needACK)
		if (handlePacketIn(std::bind(&Client::ack_handler, this, std::placeholders::_1)))
			return 2;

	return 0;
}

void Client::receiverThread() { // Поток обработки входящих пакетов
	// Set thread description
	setThreadDesc(L"Receiver");

	while (started) {
		if (int err = handlePacketIn(std::bind(&Client::any_packet_handler, this, std::placeholders::_1))) {
			if (err > 0) break;    // Критическая ошибка или соединение сброшено
			else         continue; // Неудачный пакет, продолжить прием        
		}
	}

	// Закрываем поток
	log_colored(ConsoleColor::InfoHighlighted, "Closing receiver thread");

	disconnect();
}

void Client::senderThread() { // Поток отправки пакетов
	// Set thread description
	setThreadDesc(L"Sender");

	while (started) {
		// Обработать основные пакеты
		while (!mainPackets.empty()) {
			PacketPtr packet = mainPackets.back();

			if (handlePacketOut(packet)) {
				log_colored(ConsoleColor::Warning, "Packet %d not confirmed, adding to sync queue", packet->ID);
				syncPackets.push_back(packet);
			}

			mainPackets.pop();
		}

		// Обработать пакеты, не подтвержденные сервером
		auto packetIt = syncPackets.begin();
		while (packetIt != syncPackets.end()) {
			if (handlePacketOut(*packetIt)) {
				log_colored(ConsoleColor::Warning, "Sync packet %d not confirmed", (*packetIt)->ID);
				packetIt++;
			}
			else packetIt = syncPackets.erase(packetIt);
		}

		Sleep(100);
	}

	// Закрываем поток
	log_colored(ConsoleColor::InfoHighlighted, "Closing sender thread");
}

int Client::sendData(PacketPtr packet) {
	// Send an initial buffer
	setState(ClientState::Send);

	/*if (!packet->data) { //Ввести данные
		std::string req;

		log_raw("Type what you want to send to client: \n>");

		std::getline(std::cin, req);

		packet = packetFactory.create(req.c_str(), req.size(), false);
	}*/

	if (send(writeSocket, packet->data, packet->size, 0) == SOCKET_ERROR) {
		wsa_print_err();
		return 1;
	}

	// Добавить пакет
	sendedPackets.push_back(packet);

	return 0;
}

int Client::receiveData(PacketPtr& dest)
{
	// Receive until the peer closes the connection
	setState(ClientState::Receive);

	std::array<char, NET_BUFFER_SIZE> respBuff;
	int respSize = recv(readSocket, respBuff.data(), NET_BUFFER_SIZE, 0);

	if (respSize > 0) {
		// Записываем данные от клиента
		dest = packetFactory.create(respBuff.data(), respSize, false);

		// Добавить пакет
		receivedPackets.push_back(dest);
	}
	else if (!respSize) {
		log_raw_colored(ConsoleColor::InfoHighlighted, "Connection closed");
		return 1;
	}
	else {
		wsa_print_err();
		return 2;
	}

	return 0;
}

void Client::setState(ClientState state)
{
#ifdef _DEBUG
	const char* state_desc;

#define PRINT_STATE(X) case ClientState::X: \
	state_desc = #X;                        \
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
		log_colored(ConsoleColor::WarningHighlighted, "Unknown state: %d", (int)state);
		return;
	}
#undef PRINT_STATE

	log_colored(ConsoleColor::Info, "State changed to: %s", state_desc);
#endif

	this->state = state;
}