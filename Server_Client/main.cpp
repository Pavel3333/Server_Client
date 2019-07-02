#include "main.h"

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

Client::Client() {
	state = CLIENT_STATE::SUCCESS;

	ConnectSocket = INVALID_SOCKET;
}

Client::~Client() {
	if(state > INIT_WINSOCK) error_code = WSAGetLastError();

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

	if(error_msg) printf(error_msg, error_code);

	if(state > CREATE_SOCKET) closesocket(ConnectSocket);
	if(state > INIT_WINSOCK)  WSACleanup();
}

bool Client::connect2Server() {
	// Initialize Winsock
	state = CLIENT_STATE::INIT_WINSOCK;

	if (error_code = WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) return true;

	// Create a SOCKET for connecting to server (TCP/IP protocol)
	state = CLIENT_STATE::CREATE_SOCKET;

	ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	if (ConnectSocket == INVALID_SOCKET) return true;

	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.

	clientService.sin_family = AF_INET;
	clientService.sin_port   = htons(DEFAULT_PORT);
	inet_pton(AF_INET, SERVER_IP, &clientService.sin_addr.s_addr);

	// Connect to server.
	state = CLIENT_STATE::CONNECT;

	if (connect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) return true;

	return state = CLIENT_STATE::SUCCESS;
}

bool Client::sendData(const char* data, int size) {
	// Send an initial buffer
	state = CLIENT_STATE::SEND;

	bytesSent = send(ConnectSocket, data, size, 0);
	if (bytesSent == SOCKET_ERROR) return true;

	printf("Bytes Sent: %d\n", bytesSent);

	return state = CLIENT_STATE::SUCCESS;
}

bool Client::receiveData(char* data, int size) {
	// shutdown the connection since no more data will be sent
	state = CLIENT_STATE::SHUTDOWN;

	if (shutdown(ConnectSocket, SD_SEND) == SOCKET_ERROR) return true;

	// Receive until the peer closes the connection
	state = CLIENT_STATE::RECEIVE;

	do {
		bytesRec = recv(ConnectSocket, data, size, 0);
		if (bytesRec > 0)       printf("Bytes received: %d\n", bytesRec);
		else if (bytesRec == 0) printf("Connection closed\n");
		else return true;
	} while (bytesRec > 0);

	return state = CLIENT_STATE::SUCCESS;
}

bool Client::disconnect() {
	// close the socket
	state = CLIENT_STATE::CLOSE_SOCKET;

	if (closesocket(ConnectSocket) == SOCKET_ERROR) return true;

	return state = CLIENT_STATE::SUCCESS;
}

int main() {
	Client* client = new Client();

	if (client->connect2Server()) {
		delete client;
		client = nullptr;
	}

	return 0;
}