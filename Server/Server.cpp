#include "stdafx.h"

#include "Server.h"

Server::Server(uint16_t port) {
	// Инициализация
	state = SERVER_STATE::OK;

	connectSocket = INVALID_SOCKET;
	socketDesc = nullptr;

	this->port = port;

	_itoa_s(this->port, const_cast<char*>(this->port_str), 7, 10);
}

Server::~Server() {
	if (state > SERVER_STATE::INIT_WINSOCK) error_code = WSAGetLastError();

	// Вывод сообщения об ошибке

	const char* error_msg = nullptr;

	//TODO
	//switch (state)
	//{
	//	...
	//}

	if (error_msg) printf(error_msg, error_code);

	if (state > SERVER_STATE::CREATE_SOCKET) closesocket(connectSocket);
	if (state > SERVER_STATE::INIT_WINSOCK)  WSACleanup();
}

bool Server::startServer() {
	// Initialize Winsock
	state = SERVER_STATE::INIT_WINSOCK;

	if (error_code = WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) return true;

	// Resolve the server address and port
	state = SERVER_STATE::GET_ADDR;

	ZeroMemory(&socketDescTemp, sizeof(socketDescTemp));
	socketDescTemp.ai_family   = AF_INET;
	socketDescTemp.ai_socktype = SOCK_STREAM;
	socketDescTemp.ai_protocol = IPPROTO_TCP;
	socketDescTemp.ai_flags    = AI_PASSIVE;

	if (getaddrinfo(NULL, port_str, &socketDescTemp, &socketDesc)) return true;

	// Create a SOCKET for connecting to clients (TCP/IP protocol)
	state = SERVER_STATE::CREATE_SOCKET;

	connectSocket = socket(socketDesc->ai_family, socketDesc->ai_socktype, socketDesc->ai_protocol);
	if (connectSocket == INVALID_SOCKET) return true;

	// Bind the socket
	state = SERVER_STATE::BIND;

	if (bind(connectSocket, socketDesc->ai_addr, (int)socketDesc->ai_addrlen) == SOCKET_ERROR) return true;

	printf("The server is running\n");

	state = SERVER_STATE::OK;
	return false;
}

bool Server::closeServer() {
	// Close the socket
	state = SERVER_STATE::CLOSE_SOCKET;

	if (closesocket(connectSocket) == SOCKET_ERROR) return true;
	WSACleanup();

	printf("The server was stopped\n");

	state = SERVER_STATE::OK;
	return false;
}

bool Server::handleRequests() {
	// Listening the port
	state = SERVER_STATE::LISTEN;

	if (listen(connectSocket, SOMAXCONN) == SOCKET_ERROR) return true;

	while (true)
	{
		SOCKADDR_IN addr_c;
		int addrlen = sizeof(addr_c);

		if (SOCKET clientSocket = accept(connectSocket, (struct sockaddr*)&addr_c, &addrlen) == INVALID_SOCKET) continue;

		// Connecting to client
		state = SERVER_STATE::CONNECT;

		char addr_str[16];

		if (!inet_ntop(AF_INET, socketDesc->ai_addr, addr_str, 32)) printf("Cannot to get addr string of server IP\n");
		else                                                        printf("Client connected from %s (%u)\n", addr_str, ntohs(addr_c.sin_port));

		//SClient* client = new SClient(acceptS, addr_c);

		//добавить клиента в вектор, прочитать данные и что-нибудь ему отправить

		Sleep(50);
	}
	
	state = SERVER_STATE::OK;
	return false;
}