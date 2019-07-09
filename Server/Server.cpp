#include "pch.h"

#include "Server.h"

Server::Server(USHORT port)
    : connectSocket(INVALID_SOCKET)
	, port(port)
{
	setState(SERVER_STATE::OK);
}

Server::~Server()
{
	if (state > SERVER_STATE::INIT_WINSOCK) error_code = WSAGetLastError();

	if (state > SERVER_STATE::GET_ADDR && state <= SERVER_STATE::BIND) freeaddrinfo(socketDesc);

	if (state > SERVER_STATE::CREATE_SOCKET) closesocket(connectSocket);
	if (state > SERVER_STATE::INIT_WINSOCK)  WSACleanup();
}


int Server::startServer()
{
	// Initialize Winsock
	setState(SERVER_STATE::INIT_WINSOCK);

	int err = WSAStartup(MAKEWORD(2, 2), &wsData);
	if (err) {
		return 1;
	}

	// Create a SOCKET for connecting to clients (TCP/IP protocol)
	setState(SERVER_STATE::CREATE_SOCKET);

	connectSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (connectSocket == INVALID_SOCKET) {
		return 2;
	}

	// Bind the socket
	setState(SERVER_STATE::BIND);

	sockaddr_in hint;
	ZeroMemory(&hint, sizeof(hint));
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	err = bind(connectSocket, (sockaddr*)&hint, sizeof(hint));
	if (err == SOCKET_ERROR) {
		closesocket(connectSocket);
		return 3;
	}

	// Listening the port
	setState(SERVER_STATE::LISTEN);

	err = listen(connectSocket, SOMAXCONN);
	if (err == SOCKET_ERROR) {
		closesocket(connectSocket);
		return 1;
	}

	freeaddrinfo(socketDesc);

	cout << "The server is running" << endl;

	setState(SERVER_STATE::OK);
	return 0;
}

int Server::closeServer()
{
	// Shutdown the connection since no more data will be sent
	setState(SERVER_STATE::SHUTDOWN);

	if (shutdown(connectSocket, SD_SEND) == SOCKET_ERROR) return 1;

	// Close the socket
	setState(SERVER_STATE::CLOSE_SOCKET);

	if (closesocket(connectSocket) == SOCKET_ERROR) return 1;

	WSACleanup();

	cout << "The server was stopped" << endl;

	setState(SERVER_STATE::OK);
	return 0;
}


int Server::handleRequests()
{
	sockaddr_in client;
	int clientlen = sizeof(client);

	while (clients.size() < 10) {
		cout << "Wait for client..." << endl;

		SOCKET clientSocket = accept(connectSocket, (sockaddr*)&client, &clientlen);
		if (clientSocket == INVALID_SOCKET) continue;

		setState(SERVER_STATE::CONNECT); // Connected to client


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


		clients.push_back(std::make_unique<Client>(clientSocket, client_IP, client_port));


		//if (receiveData(clientSocket)) cout << "RECEIVE - error: " << WSAGetLastError() << endl;

		Sleep(100);
	}

	setState(SERVER_STATE::OK);
	return 0;
}


void Server::setState(SERVER_STATE state)
{
#ifdef _DEBUG
#define PRINT_STATE(X) case SERVER_STATE:: X: \
	std::cout << "state changed to: " #X << std::endl; \
	break;

	switch (state) {
		PRINT_STATE(OK);
		PRINT_STATE(INIT_WINSOCK);
		PRINT_STATE(GET_ADDR);
		PRINT_STATE(CREATE_SOCKET);
		PRINT_STATE(BIND);
		PRINT_STATE(LISTEN);
		PRINT_STATE(CONNECT);
		PRINT_STATE(RECEIVE);
		PRINT_STATE(SEND);
		PRINT_STATE(SHUTDOWN);
	default:
		std::cout << "unknown state: " << (int)state << std::endl;
	}
#undef PRINT_STATE
#endif

	this->state = state;
}