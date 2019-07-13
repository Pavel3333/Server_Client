#include "pch.h"
#include "Server.h"


Server::Server(uint16_t readPort, uint16_t writePort)
    : listeningReadSocket(INVALID_SOCKET)
	, listeningWriteSocket(INVALID_SOCKET)
	, readPort(readPort)
	, writePort(writePort)
	, started(false)
{
}


Server::~Server()
{
	if (started)
		closeServer();
}


bool Server::isRunning() { return this->started; }

int Server::startServer()
{
	// Initialize Winsock
	setState(ServerState::InitWinSock);

	WSADATA wsData;

	int err = WSAStartup(MAKEWORD(2, 2), &wsData);
	if (err) {
		wsa_print_err();
		return 1;
	}

	if (err = initSockets()) return err;

	log_raw_colored(ConsoleColor::SuccessHighlighted, "The server is running");

	started = true;
	
	firstHandshakesHandler = std::thread(&Server::handleNewClients, this, true);
	firstHandshakesHandler.detach();

	secondHandshakesHandler = std::thread(&Server::handleNewClients, this, false);
	secondHandshakesHandler.detach();

	return 0;
}

int Server::closeServer()
{
	if (!started)
		return 0;

	started = false;

	// Closing handler threads

	if (firstHandshakesHandler.joinable())
		firstHandshakesHandler.join();

	if (secondHandshakesHandler.joinable())
		secondHandshakesHandler.join();

	// Close the socket
	setState(ServerState::CloseSockets);

	if (listeningReadSocket != INVALID_SOCKET) {
		int err = closesocket(listeningReadSocket);
		if (err == SOCKET_ERROR)
			log_colored(ConsoleColor::DangerHighlighted, "Error while closing socket: %d", WSAGetLastError());
	}

	if (listeningWriteSocket != INVALID_SOCKET) {
		int err = closesocket(listeningWriteSocket);
		if (err == SOCKET_ERROR)
			log_colored(ConsoleColor::DangerHighlighted, "Error while closing socket: %d", WSAGetLastError());
	}

	WSACleanup();

	log_raw_colored(ConsoleColor::InfoHighlighted, "The server was stopped");

	return 0;
}

SOCKET Server::initSocket(uint16_t port) {
	SOCKET result = INVALID_SOCKET;

	// result = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); - у меня не работает
	result = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (result == INVALID_SOCKET) {
		wsa_print_err();
		return INVALID_SOCKET;
	}

	// Bind the socket
	setState(ServerState::Bind);

	sockaddr_in hint;
	ZeroMemory(&hint, sizeof(hint));
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	int err = bind(result, (sockaddr*)&hint, sizeof(hint));
	if (err == SOCKET_ERROR) {
		wsa_print_err();
		return INVALID_SOCKET;
	}

	// Listening the port
	setState(ServerState::Listen);

	err = listen(result, SOMAXCONN);
	if (err == SOCKET_ERROR) {
		wsa_print_err();
		return INVALID_SOCKET;
	}

	return result;
}

int Server::initSockets() {
	// Create a read socket that receiving data from server (UDP protocol)
	setState(ServerState::CreateReadSocket);

	listeningReadSocket = initSocket(readPort);
	if (listeningReadSocket == INVALID_SOCKET)
		return 1;

	log_colored(ConsoleColor::SuccessHighlighted, "The server can accept clients on the port %d", readPort);

	// Create a write socket that sending data to the server (UDP protocol)
	setState(ServerState::CreateWriteSocket);

	listeningWriteSocket = initSocket(writePort);
	if (listeningWriteSocket == INVALID_SOCKET)
		return 1;

	log_colored(ConsoleColor::SuccessHighlighted, "The server can accept clients on the port %d", writePort);

	started = true;

	return 0;
}

void Server::handleNewClients(bool isReadSocket)
{
	// Set thread description
	setThreadDesc(L"NCH"); //New Client Handler

	// Init local vars
	uint16_t clientID = 0;

	uint16_t port = readPort;
	SOCKET socket = listeningReadSocket;

	if (!isReadSocket) {
		port = writePort;
		socket = listeningWriteSocket;
	}

	sockaddr_in clientDesc;
	int clientLen = sizeof(clientDesc);

	// 10 clients limit
	while (started && clientPool.size() < 10) {
		log_colored(ConsoleColor::InfoHighlighted, "Wait for client on port %d...", port);

		SOCKET clientSocket = accept(socket, (sockaddr*)&clientDesc, &clientLen);
		if (clientSocket == INVALID_SOCKET)
			continue;

		// Connected to client
		setState(ServerState::Connect);

		uint32_t client_ip = clientDesc.sin_addr.s_addr;

		// Получаем итератор
		auto client_it = clientPool.find(client_ip);
		if (client_it == end(clientPool) && isReadSocket) {
			// Такого клиента нет, добавить и начать рукопожатие
			auto client = std::make_shared<ConnectedClient>(clientID++, clientDesc, clientLen);

			clientPool[client_ip] = client;

			if (client->first_handshake(clientSocket))
				log_colored(ConsoleColor::WarningHighlighted, "Error while first handshaking. Client ID: %d", client->getID());
		}
		else {
			// Уже есть клиент с таким же IP, продолжить рукопожатие
			ConnectedClient& client = *(client_it->second);

			if (client.second_handshake(clientSocket))
				log_colored(ConsoleColor::WarningHighlighted, "Error while second handshaking. Client ID: %d", client.getID());
		}
	}

	if(clientPool.size() == 10)  log_raw_colored(ConsoleColor::WarningHighlighted, "Client connections count limit exceeded");
}


void Server::setState(ServerState state)
{
#ifdef _DEBUG
	const char* state_desc;

#define PRINT_STATE(X) case ServerState:: X: \
	state_desc = #X;                         \
	break;

	switch (state) {
		PRINT_STATE(InitWinSock)
		PRINT_STATE(CreateReadSocket)
		PRINT_STATE(CreateWriteSocket)
		PRINT_STATE(Bind)
		PRINT_STATE(Listen)
		PRINT_STATE(Connect)
		PRINT_STATE(CloseSockets)
	default:
		log_colored(ConsoleColor::WarningHighlighted, "Unknown state: %d", (int)state);
		return;
	}
#undef PRINT_STATE

	log_colored(ConsoleColor::Info, "State changed to: %s", state_desc);
#endif

	this->state = state;
}