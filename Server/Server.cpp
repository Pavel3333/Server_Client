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

	// Create a SOCKET for connecting to clients (TCP/IP protocol)
	setState(SERVER_STATE::CREATE_SOCKET);

	connectSocket = socket(AF_INET, SOCK_STREAM, 0);
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


int Server::handleRequests() //TODO: вынести в поток
{
	sockaddr_in client;
	int clientlen = sizeof(client);

	// 10 clients limit

	while (clientPool.size() < 10) {
		cout << "Wait for client..." << endl;

		SOCKET clientSocket = accept(connectSocket, (sockaddr*)&client, &clientlen);
		if (clientSocket == INVALID_SOCKET) continue;

		setState(SERVER_STATE::CONNECT); // Connected to client

		// Collecting data about client IP, port and extra data (host, service)

		char client_IP [16]         = "-";
		char host      [NI_MAXHOST] = "-";
		char service   [NI_MAXSERV] = "-";

		uint16_t client_port = ntohs(client.sin_port);

		inet_ntop(AF_INET, &client.sin_addr, client_IP, 16); //get IP addr string

		int err = getnameinfo((sockaddr*)&client, clientlen, host, NI_MAXHOST, service, NI_MAXSERV, 0);


		cout << "Client (IP: " << client_IP << ", host: " << host << ") connected on port ";

		if (err) cout << client_port;
		else     cout << service;

		cout << endl;

		// Add the client into the clients vector

		auto client = std::make_shared<ConnectedClient>(clientSocket, client_IP, client_port);

		client->handshake();

		clientPool.push_back(client);

		Sleep(1000);
	}

	cout << "Client connections count limit exceeded" << endl;

	setState(SERVER_STATE::OK);
	return 0;
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