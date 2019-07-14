#include "pch.h"
#include <array>
#include "Client.h"

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
	disconnect();
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

void Client::disconnect() {
	if (!started)
		return;

	started = false;

	// Closing handler threads

	if (receiver.joinable())
		receiver.join();

	if (sender.joinable())
		sender.join();

	// Clear all data

	receivedPackets.clear();
	sendedPackets.clear();
	syncPackets.clear();

	while (!mainPackets.empty())
		mainPackets.pop();

	WSACleanup();

	log_raw_colored(ConsoleColor::InfoHighlighted, "The client was stopped");
}

void Client::printCommandsList() {
	log_raw_colored(ConsoleColor::InfoHighlighted, "You can use these commands to manage the client:");
	log_raw_colored(ConsoleColor::Info,   "  \"send\"     => Send the packet to server");
	log_raw_colored(ConsoleColor::Info,   "  \"commands\" => Print all available commands");
	log_raw_colored(ConsoleColor::Danger, "  \"close\"    => Close the client");
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

	// Set socket options
	setState(ClientState::SetOpts);

	uint32_t value = TIMEOUT * 1000;
	uint32_t size = sizeof(value);

	// Set timeout for sending
	int err = setsockopt(result, SOL_SOCKET, SO_SNDTIMEO, (char *)&value, size);
	if (err == SOCKET_ERROR) {
		wsa_print_err();
		return INVALID_SOCKET;
	}

	// Set timeout for receiving
	err = setsockopt(result, SOL_SOCKET, SO_RCVTIMEO, (char *)&value, size);
	if (err == SOCKET_ERROR) {
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
	// Create a read socket that receiving data from server
	setState(ClientState::CreateReadSocket);

	readSocket = connect2server(readPort);
	if (readSocket == INVALID_SOCKET) {
		wsa_print_err();
		return 1;
	}

	log_colored(ConsoleColor::SuccessHighlighted, "The client can read the data from the port %d", readPort);

	// Create a write socket that sending data to the server
	setState(ClientState::CreateWriteSocket);

	writeSocket = connect2server(writePort);
	if (writeSocket == INVALID_SOCKET) {
		wsa_print_err();
		return 1;
	}

	log_colored(ConsoleColor::SuccessHighlighted, "The client can write the data to the port %d", writePort);

	started = true;

	//TODO: ������� Hello ����� �� �������, ��������� � ����� ������� Hello �����

	createThreads();

	return 0;
}


// ���������� ����� ACK
int Client::ack_handler(PacketPtr packet)
{
	log_raw(std::string_view(packet->data, packet->size));
	return 0;
}

// ���������� ����� �������� �����
int Client::any_packet_handler(PacketPtr packet)
{
	log_raw(std::string_view(packet->data, packet->size));
	return 0;
}


// ��������� ��������� ������
int Client::handlePacketIn(std::function<int(PacketPtr)> handler, bool closeAfterTimeout) {
	PacketPtr packet;

	int err = receiveData(packet, closeAfterTimeout);
	if (err)
		return err; // ��������� ������

	if (!packet) return -1;

	// ��������� ���������� ������
	return handler(packet);
}

// ��������� ���������� ������
int Client::handlePacketOut(PacketPtr packet) {
	if (sendData(packet))
		return 1;

	if (packet->needACK) {                                                         // ���� ����� ������������� ������������� ������
		int err = handlePacketIn(                                                  // ����������� ������� �������������
			std::bind(&Client::ack_handler, this, std::placeholders::_1),
			true                                                                   // ������� 3 �������
		);

		if (err)
			return 2;
	}

	return 0;
}


// ����� ��������� �������� �������
void Client::receiverThread() {
	// ������ ��� ������
	setThreadDesc(L"Receiver");

	int err = 0;

	// �������� ����� �������� �������
	// ������� �� �����
	while (isRunning()) {
		err = handlePacketIn(
			std::bind(&Client::any_packet_handler, this, std::placeholders::_1),
			false
		);

		if (err) {
			if (err > 0) break;    // ����������� ������ ��� ���������� ��������
			else         continue; // ��������� �����, ���������� �����        
		}
	}

	// ���������� ���������� (����� ��� ������)
	setState(ClientState::Shutdown);

	if (readSocket != INVALID_SOCKET)
		if (shutdown(readSocket, SD_BOTH) == SOCKET_ERROR)
			wsa_print_err();

	// �������� ������ (������)
	setState(ClientState::CloseSocket);

	if (readSocket != INVALID_SOCKET)
		if (closesocket(readSocket) == SOCKET_ERROR)
			wsa_print_err();

	readSocket = INVALID_SOCKET;

	// ��������� �����
	log_colored(ConsoleColor::InfoHighlighted, "Receiver thread closed");
}

// ����� �������� �������
void Client::senderThread()
{
	// ������ ��� ������
	setThreadDesc(L"Sender");

	while (isRunning()) {
		// ���������� �������� ������
		while (!mainPackets.empty()) {
			PacketPtr packet = mainPackets.back();

			if (handlePacketOut(packet)) {
				log_colored(ConsoleColor::Warning, "Packet %d not confirmed, adding to sync queue", packet->ID);
				syncPackets.push_back(packet);
			}

			mainPackets.pop();
		}

		// ���������� ������, �� �������������� ��������
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

	// ���������� ���������� (����� ��� ������)
	setState(ClientState::Shutdown);

	if (writeSocket != INVALID_SOCKET)
		if (shutdown(writeSocket, SD_BOTH) == SOCKET_ERROR)
			wsa_print_err();

	// �������� ������ (������)
	setState(ClientState::CloseSocket);

	if (writeSocket != INVALID_SOCKET)
		if (closesocket(writeSocket) == SOCKET_ERROR)
			wsa_print_err();

	writeSocket = INVALID_SOCKET;

	// ��������� �����
	log_colored(ConsoleColor::InfoHighlighted, "Sender thread closed");
}

// �������� �������
void Client::createThreads()
{
	receiver = std::thread(&Client::receiverThread, this);
	sender   = std::thread(&Client::senderThread,   this);
}


// �������� ������
int Client::receiveData(PacketPtr& dest, bool closeAfterTimeout)
{
	setState(ClientState::Receive);

	std::array<char, NET_BUFFER_SIZE> respBuff;

	while (isRunning()) {
		// ���� �������� ���������.  ����� �����������:
		// - ����� ����������� ������,
		// - ����� �������� ����������,
		// - ����� �������� (��. closeAfterTimeout)

		int respSize = recv(readSocket, respBuff.data(), NET_BUFFER_SIZE, 0);

		if      (respSize > 0) {
			// ���������� ������ �� �������
			dest = packetFactory.create(respBuff.data(), respSize, false);

			// �������� �����
			receivedPackets.push_back(dest);

			break;
		}
		else if (!respSize) {
			// ���������� ��������
			log_raw_colored(ConsoleColor::InfoHighlighted, "Connection closed");
			return 1;
		}
		else {
			int err = WSAGetLastError();

			if      (err == WSAETIMEDOUT) {
				// �������
				if (closeAfterTimeout) return -1;
				else                   continue;
			}
			else if (err == WSAEMSGSIZE) {
				// ������ ������ �������� ������ ������
				// ������� ��������������
				log_raw_colored(ConsoleColor::WarningHighlighted, "The size of received packet is larger than the buffer size!");
				return -2;
			}
			else if (err == WSAECONNRESET || err == WSAECONNABORTED) {
				// ���������� ��������
				log_raw_colored(ConsoleColor::InfoHighlighted, "Connection closed");
				return 2;
			}
			else {
				// ����������� ������
				wsa_print_err();
				return 3;
			}
		}
	}

	return 0;
}

// �������� ������
int Client::sendData(PacketPtr packet) {
	setState(ClientState::Send);

	if (send(writeSocket, packet->data, packet->size, 0) == SOCKET_ERROR) {
		wsa_print_err();
		return 1;
	}

	// �������� �����
	sendedPackets.push_back(packet);

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
		PRINT_STATE(SetOpts);
		PRINT_STATE(Send);
		PRINT_STATE(Shutdown);
		PRINT_STATE(Receive);
		PRINT_STATE(CloseSocket);
	default:
		log_colored(ConsoleColor::WarningHighlighted, "Unknown state: %d", (int)state);
		return;
	}
#undef PRINT_STATE

	log_colored(ConsoleColor::Info, "State changed to: %s", state_desc);
#endif

	this->state = state;
}