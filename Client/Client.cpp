#include "stdafx.h"

#include "Client.h"

Packet::Packet(char* data, size_t size) {
	this->data = new char[size + 2];
	memcpy(this->data, data, size);
	this->data[size] = NULL; //NULL-terminator

	this->size = size;
}

Packet::~Packet() {
	delete this->data;
}

Client::Client(const char* IP, uint16_t port) {
	// Инициализация
	state = CLIENT_STATE::OK;

	connectSocket = INVALID_SOCKET;

	this->IP   = IP;
	this->port = port;
}

Client::~Client() {
	if (state > CLIENT_STATE::INIT_WINSOCK) error_code = WSAGetLastError();

	// Вывод сообщения об ошибке

	const char* error_msg = nullptr;

	switch (state)
	{
	case CLIENT_STATE::INIT_WINSOCK:
		error_msg = "INIT_WINSOCK - error: %d\n";
		break;
	case CLIENT_STATE::CREATE_SOCKET:
		error_msg = "CREATE_SOCKET - error: %d\n";
		break;
	case CLIENT_STATE::CONNECT:
		error_msg = "CONNECT - error: %d\n";
		break;
	case CLIENT_STATE::SEND:
		error_msg = "SEND - error: %d\n";
		break;
	case CLIENT_STATE::SHUTDOWN:
		error_msg = "SHUTDOWN - error: %d\n";
		break;
	case CLIENT_STATE::RECEIVE:
		error_msg = "RECEIVE - error: %d\n";
		break;
	case CLIENT_STATE::CLOSE_SOCKET:
		error_msg = "CLOSE_SOCKET - error: %d\n";
		break;
	default: break;
	}

	if (error_msg) printf(error_msg, error_code);
	else {
		printf("All received data:\n");

		for (uint16_t i = 0; i < receivedPackets.size(); i++) {
			printf("  %d:\n    size: %d\n    data: %s\n", i, receivedPackets[i]->size, receivedPackets[i]->data);
		}
	}

	if (state > CLIENT_STATE::CREATE_SOCKET) closesocket(connectSocket);
	if (state > CLIENT_STATE::INIT_WINSOCK)  WSACleanup();

	receivedPackets.clear();
}

bool Client::connect2server() {
	// Initialize Winsock
	state = CLIENT_STATE::INIT_WINSOCK;

	if (error_code = WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) return true;

	// Create a SOCKET for connecting to server (TCP/IP protocol)
	state = CLIENT_STATE::CREATE_SOCKET;

	connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connectSocket == INVALID_SOCKET) return true;

	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.

	socketDesc.sin_family = AF_INET;
	socketDesc.sin_port = htons(port);
	inet_pton(AF_INET, IP, &socketDesc.sin_addr.s_addr);

	// Connect to server.
	state = CLIENT_STATE::CONNECT;

	if (connect(connectSocket, (SOCKADDR*)&socketDesc, sizeof(socketDesc)) == SOCKET_ERROR) return true;

	char addr_str[16];

	if (!inet_ntop(AF_INET, &(socketDesc.sin_addr), addr_str, 32)) printf("Cannot to get addr string of server IP\n");
	else                                                           printf("The client was connected to the server %s (%u)\n", addr_str, ntohs(socketDesc.sin_port));

	state = CLIENT_STATE::OK;
	return false;
}

bool Client::sendData(std::string_view data) {
	// Send an initial buffer
	state = CLIENT_STATE::SEND;

	bytesSent = send(connectSocket, data.data(), data.size(), 0);
	if (bytesSent == SOCKET_ERROR) return true;

	printf("Bytes sent: %d, data: %s\n", bytesSent, data);

	state = CLIENT_STATE::OK;
	return false;
}

bool Client::receiveData() {
	// Receive until the peer closes the connection
	state = CLIENT_STATE::RECEIVE;

	char recBuff[NET_BUFFER_SIZE];

	int bytesRec;

	do {
		bytesRec = recv(connectSocket, recBuff, NET_BUFFER_SIZE, 0);
		if (bytesRec > 0) { //Записываем данные от клиента (TODO: писать туда и ID клиента)
			receivedPackets.push_back(std::make_unique<Packet>(recBuff, bytesRec));

			printf("Bytes received: %d, data: %s\n", bytesRec, receivedPackets.back()->data);
		}
		else if (bytesRec == 0) printf("Connection closed\n");
		else
			return true;
	}
	while (bytesRec > 0);

	state = CLIENT_STATE::OK;
	return false;
}

bool Client::disconnect() {
	// Shutdown the connection since no more data will be sent
	state = CLIENT_STATE::SHUTDOWN;

	if (shutdown(connectSocket, SD_SEND) == SOCKET_ERROR) return true;

	// Close the socket
	state = CLIENT_STATE::CLOSE_SOCKET;

	if (closesocket(connectSocket) == SOCKET_ERROR) return true;

	WSACleanup();

	printf("The client was stopped\n");

	state = CLIENT_STATE::OK;
	return false;
}