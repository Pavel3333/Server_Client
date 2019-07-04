#include "stdafx.h"

#include "Client.h"

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
		error_msg = "CREATE_SOCKET - error: %ld\n";
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
			printf("%d : %s\n", receivedPackets);
		}
	}

	if (state > CLIENT_STATE::CREATE_SOCKET) closesocket(connectSocket);
	if (state > CLIENT_STATE::INIT_WINSOCK)  WSACleanup();

	while (!receivedPackets.empty()) {
		delete receivedPackets.back();
		receivedPackets.pop_back();
	}
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

bool Client::sendData(const char* data, int size) {
	// Send an initial buffer
	state = CLIENT_STATE::SEND;

	bytesSent = send(connectSocket, data, size, 0);
	if (bytesSent == SOCKET_ERROR) return true;

	printf("Bytes sent: %d\n", bytesSent);

	// shutdown the connection since no more data will be sent
	state = CLIENT_STATE::SHUTDOWN;

	if (shutdown(connectSocket, SD_SEND) == SOCKET_ERROR) return true;

	state = CLIENT_STATE::OK;
	return false;
}

bool Client::receiveData() {
	// shutdown the connection since no more data will be sent
	state = CLIENT_STATE::SHUTDOWN;

	if (shutdown(connectSocket, SD_SEND) == SOCKET_ERROR) return true;

	// Receive until the peer closes the connection
	state = CLIENT_STATE::RECEIVE;

	char recStr[NET_BUFFER_SIZE];

	uint16_t bytesRec;

	do {
		bytesRec = recv(connectSocket, recStr, NET_BUFFER_SIZE, 0);
		if (bytesRec > 0) { //Записываем данные от сервера
			printf("Bytes received: %d\n", bytesRec);

			Packet* packet = new Packet{
				recStr,
				bytesRec
			};

			receivedPackets.push_back(packet);
		}
		else if (bytesRec == 0) printf("Connection closed\n");
		else return true;
	} while (bytesRec > 0);

	state = CLIENT_STATE::OK;
	return false;
}

bool Client::disconnect() {
	// Close the socket
	state = CLIENT_STATE::CLOSE_SOCKET;

	if (closesocket(connectSocket) == SOCKET_ERROR) return true;
	WSACleanup();

	printf("The client was stopped\n");

	state = CLIENT_STATE::OK;
	return false;
}