#include "stdafx.h"

#include "Client.h"

Client::Client(const char* IP, uint16_t port) {
	// Инициализация
	state = CLIENT_STATE::SUCCESS;

	connectSocket = INVALID_SOCKET;

	this->IP   = IP;
	this->port = port;
}

Client::~Client() {
	if (state > INIT_WINSOCK) error_code = WSAGetLastError();

	// Вывод сообщения об ошибке

	const char* error_msg = nullptr;

	switch (state)
	{
	case INIT_WINSOCK:
		error_msg = "INIT_WINSOCK - error: %d\n";
		break;
	case CREATE_SOCKET:
		error_msg = "CREATE_SOCKET - error: %ld\n";
		break;
	case CONNECT:
		error_msg = "CONNECT - error: %d\n";
		break;
	case SEND:
		error_msg = "SEND - error: %d\n";
		break;
	case SHUTDOWN:
		error_msg = "SHUTDOWN - error: %d\n";
		break;
	case RECEIVE:
		error_msg = "RECEIVE - error: %d\n";
		break;
	case CLOSE_SOCKET:
		error_msg = "CLOSE_SOCKET - error: %d\n";
		break;
	default: break;
	}

	if (error_msg) printf(error_msg, error_code);

	if (state > CREATE_SOCKET) closesocket(connectSocket);
	if (state > INIT_WINSOCK)  WSACleanup();
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

	printf("The client was connected to the server %s (%u)\n", inet_ntoa(socketDesc.sin_addr), ntohs(socketDesc.sin_port));

	return state = CLIENT_STATE::SUCCESS;
}

bool Client::sendData(const char* data, int size) {
	// Send an initial buffer
	state = CLIENT_STATE::SEND;

	bytesSent = send(connectSocket, data, size, 0);
	if (bytesSent == SOCKET_ERROR) return true;

	printf("Bytes sent: %d\n", bytesSent);

	return state = CLIENT_STATE::SUCCESS;
}

bool Client::receiveData(char* data, int size) {
	// shutdown the connection since no more data will be sent
	state = CLIENT_STATE::SHUTDOWN;

	if (shutdown(connectSocket, SD_SEND) == SOCKET_ERROR) return true;

	// Receive until the peer closes the connection
	state = CLIENT_STATE::RECEIVE;

	do {
		bytesRec = recv(connectSocket, data, size, 0);
		if      (bytesRec >  0) printf("Bytes received: %d\n", bytesRec);
		else if (bytesRec == 0) printf("Connection closed\n");
		else return true;
	} while (bytesRec > 0);

	return state = CLIENT_STATE::SUCCESS;
}

bool Client::disconnect() {
	// Close the socket
	state = CLIENT_STATE::CLOSE_SOCKET;

	if (closesocket(connectSocket) == SOCKET_ERROR) return true;
	WSACleanup();

	printf("The client was stopped\n");

	return state = CLIENT_STATE::SUCCESS;
}