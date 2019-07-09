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

	if (state != SERVER_STATE::OK) cout << "state " << (int)state << " - error: " << error_code << endl;
	else if (!receivedPackets.empty()) {
#ifdef _DEBUG
		cout << "All received data:" << endl;
		size_t i = 1;

		for (auto& it : receivedPackets) cout << i++ << ':' << endl << "  size: " << it->size << endl << "  data: " << it->data << endl;
#endif
		
		receivedPackets.clear();
	}

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

	if (shutdown(clientSocket, SD_SEND) == SOCKET_ERROR) return 1;

	// Close the socket
	setState(SERVER_STATE::CLOSE_SOCKET);

	if (closesocket(clientSocket)  == SOCKET_ERROR) return 1;
	if (closesocket(connectSocket) == SOCKET_ERROR) return 1;

	WSACleanup();

	cout << "The server was stopped" << endl;

	setState(SERVER_STATE::OK);
	return 0;
}


int Server::sendData() {
	// Send an initial buffer
	setState(SERVER_STATE::SEND);

	std::string req;

	cout << "Type what you want to send to client: " << endl << '>';
	std::getline(std::cin, req);

	auto packet = std::make_unique<Packet>(req.c_str(), req.size());

	int bytesSent = send(clientSocket, packet->data, packet->size, 0);
	if (bytesSent == SOCKET_ERROR) return 1;

	sendedPackets.push_back(std::move(packet));

	setState(SERVER_STATE::OK);
	return 0;
}

int Server::receiveData(SOCKET clientSocket) {
	// Receive until the peer closes the connection
	setState(SERVER_STATE::RECEIVE);

	char respBuff[NET_BUFFER_SIZE];
	int respSize;

	do {
		respSize = recv(clientSocket, respBuff, NET_BUFFER_SIZE, 0);
		if (respSize > 0) { //Записываем данные от клиента (TODO: писать туда и ID клиента)
			receivedPackets.push_back(std::make_unique<Packet>(respBuff, respSize));

			if (sendData()) cout << "SEND - error: " << WSAGetLastError() << endl;
		}
		else if (respSize == 0) cout << "Connection closed" << endl;
		else
			return 1;
	}
	while (respSize > 0);

	setState(state);
	return 0;
}

int Server::handleRequests()
{
	sockaddr_in client;
	int clientlen = sizeof(client);

	while (true) {
		cout << "Wait for client..." << endl;

		clientSocket = accept(connectSocket, (sockaddr*)&client, &clientlen);
		if (clientSocket == INVALID_SOCKET) continue;

		char host[NI_MAXHOST];
		char service[NI_MAXSERV];

		ZeroMemory(host, NI_MAXHOST);
		ZeroMemory(service, NI_MAXSERV);

		if (int err = getnameinfo((sockaddr*)&client, clientlen, host, NI_MAXHOST, service, NI_MAXSERV, 0)) {
			inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
			cout << host << " connected on port " << ntohs(client.sin_port) << endl;
		}
		else cout << host << " connected on port " << service << endl;

		// Connecting to client
		setState(SERVER_STATE::CONNECT);

		if (receiveData(clientSocket)) cout << "RECEIVE - error: " << WSAGetLastError() << endl;

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
