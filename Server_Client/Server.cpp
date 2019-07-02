#include "stdafx.h"

#include "Server.h"

Server::Server(uint16_t port) {
	// Инициализация
	state = SERVER_STATE::SUCCESS;

	connectSocket = INVALID_SOCKET;

	this->port = port;
}

Server::~Server() {
	if (state > INIT_WINSOCK) error_code = WSAGetLastError();

	// Вывод сообщения об ошибке

	const char* error_msg = nullptr;


	//TODO
	switch (state)
	{
		//...
	}

	if (error_msg) printf(error_msg, error_code);

	if (state > CREATE_SOCKET) closesocket(connectSocket);
	if (state > INIT_WINSOCK)  WSACleanup();
}

bool Server::startServer() {
	// Initialize Winsock
	state = SERVER_STATE::INIT_WINSOCK;

	if (error_code = WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) return true;

	// Create a SOCKET for connecting to clients (TCP/IP protocol)
	state = SERVER_STATE::CREATE_SOCKET;

	connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connectSocket == INVALID_SOCKET) return true;

	socketDesc.sin_family = AF_INET;
	socketDesc.sin_port = htons(port);
	socketDesc.sin_addr.S_un.S_addr = INADDR_ANY;

	// Bind the socket
	state = SERVER_STATE::BIND;

	if (bind(connectSocket, (struct sockaddr*)&socketDesc, sizeof(socketDesc)) == SOCKET_ERROR) return true;

	printf("The server is running\n");

	return state = SERVER_STATE::SUCCESS;
}

bool Server::closeServer() {
	// Close the socket
	state = SERVER_STATE::CLOSE_SOCKET;

	if (closesocket(connectSocket) == SOCKET_ERROR) return true;
	WSACleanup();

	printf("The server was stopped\n");

	return state = SERVER_STATE::SUCCESS;
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

		printf("Client connected from %s (%u)\n", inet_ntoa(addr_c.sin_addr), ntohs(addr_c.sin_port));
		//SClient* client = new SClient(acceptS, addr_c);

		//добавить клиента в вектор, прочитать данные и что-нибудь ему отправить

		break;

		Sleep(50);
	}
	
	return state = SERVER_STATE::SUCCESS;
}