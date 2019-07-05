#include "stdafx.h"

#include "Server.h"

Packet::Packet(char* data, size_t size) {
	this->data = new char[size + 2];
	memcpy(this->data, data, size);
	this->data[size] = NULL; //NULL-terminator

	this->size = size;
}

Packet::~Packet() {
	delete this->data;
}

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
	else {
		printf("All received data:\n");

		for (uint16_t i = 0; i < receivedPackets.size(); i++) {
			printf("  %d:\n    size: %d\n    data: %s\n", i, receivedPackets[i]->size, receivedPackets[i]->data);
		}
	}

	if (state > SERVER_STATE::GET_ADDR && state <= SERVER_STATE::BIND) freeaddrinfo(socketDesc);

	if (state > SERVER_STATE::CREATE_SOCKET) closesocket(connectSocket);
	if (state > SERVER_STATE::INIT_WINSOCK)  WSACleanup();

	receivedPackets.clear();
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

	freeaddrinfo(socketDesc);

	printf("The server is running\n");

	state = SERVER_STATE::OK;
	return false;
}

bool Server::closeServer() {
	// Shutdown the connection since no more data will be sent
	state = SERVER_STATE::SHUTDOWN;

	if (shutdown(clientSocket, SD_SEND) == SOCKET_ERROR) return true;

	// Close the socket
	state = SERVER_STATE::CLOSE_SOCKET;

	if (closesocket(clientSocket)  == SOCKET_ERROR) return true;
	if (closesocket(connectSocket) == SOCKET_ERROR) return true;

	WSACleanup();

	printf("The server was stopped\n");

	state = SERVER_STATE::OK;
	return false;
}

bool Server::sendData(std::string_view data) {
	// Send an initial buffer
	state = SERVER_STATE::SEND;

	bytesSent = send(clientSocket, data.data(), data.size(), 0);
	if (bytesSent == SOCKET_ERROR) return true;

	printf("Bytes sent: %d, data: %s\n", bytesSent, data);

	state = SERVER_STATE::OK;
	return false;
}

bool Server::receiveData() {
	// Receive until the peer closes the connection
	state = SERVER_STATE::RECEIVE;

	char recBuff[NET_BUFFER_SIZE];

	int bytesRec;

	do {
		bytesRec = recv(clientSocket, recBuff, NET_BUFFER_SIZE, 0);
		if (bytesRec > 0) { //Записываем данные от клиента (TODO: писать туда и ID клиента)
			receivedPackets.push_back(std::make_unique<Packet>(recBuff, bytesRec));

			printf("Bytes received: %d, data: %s\n", bytesRec, receivedPackets.back()->data);
		}
		else if (bytesRec == 0) printf("Connection closed\n");
		else
			return true;
	}
	while (bytesRec > 0);

	state = SERVER_STATE::OK;
	return false;
}

bool Server::handleRequests() {
	// Listening the port
	state = SERVER_STATE::LISTEN;

	if (listen(connectSocket, SOMAXCONN) == SOCKET_ERROR) return true;

	while (true) {
		SOCKADDR_IN addr_c;
		int addrlen = sizeof(addr_c);

		clientSocket = accept(connectSocket, (struct sockaddr*)&addr_c, &addrlen);
		if (clientSocket == INVALID_SOCKET) continue;

		// Connecting to client
		state = SERVER_STATE::CONNECT;

		char addr_str[16];

		if (!inet_ntop(AF_INET, socketDesc->ai_addr, addr_str, 32)) printf("Cannot to get addr string of server IP\n");
		else                                                        printf("Client connected from %s (%u)\n", addr_str, ntohs(addr_c.sin_port));

		//добавить клиента в вектор и что-нибудь ему отправить
		//SClient* client = new SClient(acceptS, addr_c);

		if(receiveData()) printf("RECEIVE - error: %d", WSAGetLastError());

		std::string_view resp = "Some data from the server";

		if (sendData(resp)) printf("RECEIVE - error: %d", WSAGetLastError());

		Sleep(50);
	}
	
	state = SERVER_STATE::OK;
	return false;
}