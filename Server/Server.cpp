#include "pch.h"

#include "Server.h"

Server::Server(USHORT port)
    : connectSocket(INVALID_SOCKET)
	, port(port)
	, server_started(false)
{
	setState(SERVER_STATE::OK);
}

Server::~Server()
{
	if (server_started) closeServer();

	if (state > SERVER_STATE::INIT_WINSOCK) error_code = WSAGetLastError();

	if (state != SERVER_STATE::OK) cout << "state " << (int)state << " - error: " << error_code << endl;

	if (state > SERVER_STATE::GET_ADDR && state <= SERVER_STATE::BIND) freeaddrinfo(socketDesc);
}


int Server::startServer()
{
	// Initialize Winsock
	setState(SERVER_STATE::INIT_WINSOCK);

	WSADATA wsData;

	if (error_code = WSAStartup(MAKEWORD(2, 2), &wsData)) return 1;

	// Create a SOCKET for connecting to clients (UDP protocol)
	setState(SERVER_STATE::CREATE_SOCKET);

	connectSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (connectSocket == INVALID_SOCKET) return 2;

	// Bind the socket
	setState(SERVER_STATE::BIND);

	sockaddr_in hint;
	ZeroMemory(&hint, sizeof(hint));
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	if (error_code = bind(connectSocket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) return 1;

	// Listening the port
	setState(SERVER_STATE::LISTEN);

	if (error_code = listen(connectSocket, SOMAXCONN) == SOCKET_ERROR) return 1;

	freeaddrinfo(socketDesc);

	handler = std::thread(&Server::handleRequests, this);
	handler.detach();

	cout << "The server is running" << endl;

	server_started = true;

	setState(SERVER_STATE::OK);
	return 0;
}

int Server::closeServer()
{
	if (!server_started) return 0;

	// Shutdown the connection since no more data will be sent
	setState(SERVER_STATE::SHUTDOWN);

	if (shutdown(connectSocket, SD_SEND) == SOCKET_ERROR) cout << "Error while shutdowning connection" << endl;

	// Close the socket
	setState(SERVER_STATE::CLOSE_SOCKET);

	if (closesocket(connectSocket) == SOCKET_ERROR) cout << "Error while closing socket" << endl;

	WSACleanup();

	cout << "The server was stopped" << endl;

	server_started = false;

	setState(SERVER_STATE::OK);
	return 0;
}

int Server::handshake_1(ConnectedClientPtr client, SOCKET socket) { // Первое рукопожатие с соединенным клиентом
	/*
	Присвоить сокет на запись
	*/
	setState(SERVER_STATE::HANDSHAKE_1);

	client->writeSocket = socket;

	setState(SERVER_STATE::OK);
	return 0;
}

int Server::handshake_2(ConnectedClientPtr client, SOCKET socket) { // Второе рукопожатие с соединенным клиентом
	/*
	Присвоить сокет на чтение
	Создать потоки-обработчики
	Отправить пакет ACK, подтвердить получение
	*/
	setState(SERVER_STATE::HANDSHAKE_2);

	client->readSocket = socket;
	client->started    = true;
	client->createThreads();

	//TODO: Отправить пакет ACK, подтвердить получение

	setState(SERVER_STATE::OK);
	return 0;
}

void Server::handleRequests()
{
	sockaddr_in clientDesc;
	int clientLen = sizeof(clientDesc);

	// 10 clients limit

	uint16_t clientID = 0;

	while (clientPool.size() < 10) {
		cout << "Wait for client..." << endl;

		SOCKET clientSocket = accept(connectSocket, (sockaddr*)&clientDesc, &clientLen);
		if (clientSocket == INVALID_SOCKET) continue;

		// Connected to client
		setState(SERVER_STATE::CONNECT);

		unsigned long IP = clientDesc.sin_addr.s_addr;

		auto clientWithSameIP = std::find_if(clientPool.cbegin(), clientPool.cend(), [IP](ConnectedClientPtr p) { return p->clientDesc.sin_addr.s_addr == IP; });
		
		if (clientWithSameIP != clientPool.cend()) { // Уже есть клиент с таким же IP, продолжить рукопожатие
			if (handshake_2(*clientWithSameIP, clientSocket)) cout << "Error while second handshaking. Client ID:" << (*clientWithSameIP)->ID << endl;
		}
		else {                                       // Такого клиента нет, добавить и начать рукопожатие
			auto client = std::make_shared<ConnectedClient>(clientID++, clientDesc, clientLen);

			clientPool.push_back(client);

			if(handshake_1(client, clientSocket)) cout << "Error while first handshaking. Client ID:" << client->ID << endl;
		}

		Sleep(500);
	}

	cout << "Client connections count limit exceeded" << endl;

	setState(SERVER_STATE::OK);
}

void Server::setState(SERVER_STATE state)
{
#ifdef _DEBUG
	const char state_desc[32];

#define PRINT_STATE(X) case SERVER_STATE:: X: \
	state_desc = #X; \
	break;

	switch (state) {
		PRINT_STATE(OK);
		PRINT_STATE(INIT_WINSOCK);
		PRINT_STATE(GET_ADDR);
		PRINT_STATE(CREATE_SOCKET);
		PRINT_STATE(BIND);
		PRINT_STATE(LISTEN);
		PRINT_STATE(CONNECT);
		PRINT_STATE(HANDSHAKE_1);
		PRINT_STATE(HANDSHAKE_2);
		PRINT_STATE(SHUTDOWN);
		PRINT_STATE(CLOSE_SOCKET);
	default:
		std::cout << "Unknown state: " << (int)state << std::endl;
		return;
	}
#undef PRINT_STATE

	std::cout << "State changed to: " << state_desc << std::endl;
#endif

	this->state = state;
}