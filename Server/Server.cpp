#include "pch.h"
#include "Server.h"


Server::Server(USHORT port)
    : connectSocket(INVALID_SOCKET)
	, port(port)
	, server_started(false)
{
	setState(ServerState::Ok);
}


Server::~Server()
{
	if (server_started)
		closeServer();

	int err = 0;
	if (state > ServerState::InitWinSock)
		err = WSAGetLastError();

	if (state != ServerState::Ok)
		cout << "state " << (int)state << " - error: " << err << endl;

	if (state > ServerState::GetAddr && state <= ServerState::Bind)
		freeaddrinfo(socketDesc);
}


int Server::startServer()
{
	// Initialize Winsock
	setState(ServerState::InitWinSock);

	WSADATA wsData;

	int err = WSAStartup(MAKEWORD(2, 2), &wsData);
	if (err)
		return 1;

	// Create a SOCKET for connecting to clients (UDP protocol)
	setState(ServerState::CreateSocket);

	// connectSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); - у меня не работает
	connectSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (connectSocket == INVALID_SOCKET)
		return 2;

	// Bind the socket
	setState(ServerState::Bind);

	sockaddr_in hint;
	ZeroMemory(&hint, sizeof(hint));
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	err = bind(connectSocket, (sockaddr*)&hint, sizeof(hint));
	if (err == SOCKET_ERROR)
		return 1;

	// Listening the port
	setState(ServerState::Listen);

	err = listen(connectSocket, SOMAXCONN);
	if (err == SOCKET_ERROR)
		return 1;

	freeaddrinfo(socketDesc);

	handler = std::thread(&Server::handleRequests, this);
	handler.join(); // accept в потоке очень сстранно себя ведет, пока join

	cout << "The server is running" << endl;

	server_started = true;

	setState(ServerState::Ok);
	return 0;
}


int Server::closeServer()
{
	if (!server_started)
		return 0;

	// Shutdown the connection since no more data will be sent
	setState(ServerState::Shutdown);

	int err = shutdown(connectSocket, SD_SEND);
	if (err == SOCKET_ERROR)
		cout << "Error while shutdowning connection" << endl;

	// Close the socket
	setState(ServerState::CloseSocket);

	err = closesocket(connectSocket);
	if (err == SOCKET_ERROR)
		cout << "Error while closing socket" << endl;

	WSACleanup();

	cout << "The server was stopped" << endl;

	server_started = false;

	setState(ServerState::Ok);
	return 0;
}


// Первое рукопожатие с соединенным клиентом
int Server::first_handshake(ConnectedClient& client, SOCKET socket)
{
	// Присвоить сокет на запись
	setState(ServerState::FirstHandshake);

	client.writeSocket = socket;

	setState(ServerState::Ok);
	return 0;
}


// Второе рукопожатие с соединенным клиентом
int Server::second_handshake(ConnectedClient& client, SOCKET socket)
{
	// Присвоить сокет на чтение
	// Создать потоки-обработчики
	// Отправить пакет ACK, подтвердить получение
	setState(ServerState::SecondHandshake);

	client.readSocket = socket;
	client.started    = true;
	client.createThreads();

	//TODO: Отправить пакет ACK, подтвердить получение

	setState(ServerState::Ok);
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
		if (clientSocket == INVALID_SOCKET)
			continue;

		// Connected to client
		setState(ServerState::Connect);

		uint32_t client_ip = clientDesc.sin_addr.s_addr;

		// получаем итератор
		auto client_it = clientPool.find(client_ip);
		if (client_it != end(clientPool)) {
			ConnectedClient& client = *(client_it->second);

			// Уже есть клиент с таким же IP, продолжить рукопожатие
			if (second_handshake(client, clientSocket)) {
				cout << "Error while second handshaking. "
					 << "Client ID: " << client.ID << endl;
			}
		}
		else {
			// Такого клиента нет, добавить и начать рукопожатие
			auto client = std::make_shared<ConnectedClient>(clientID++, clientDesc, clientLen);

			clientPool[client_ip] = client;

			if (first_handshake(*client, clientSocket))
				cout << "Error while first handshaking. Client ID:"
					 << client->ID << endl;
		}

		// accept блокирующая операция, Sleep не нужен
		 Sleep(500);
	}

	cout << "Client connections count limit exceeded" << endl;

	setState(ServerState::Ok);
}


void Server::setState(ServerState state)
{
#ifdef _DEBUG
	const char* state_desc;

#define PRINT_STATE(X) case ServerState:: X: \
	state_desc = #X; \
	break;

	switch (state) {
		PRINT_STATE(Ok)
		PRINT_STATE(InitWinSock)
		PRINT_STATE(GetAddr)
		PRINT_STATE(CreateSocket)
		PRINT_STATE(Bind)
		PRINT_STATE(Listen)
		PRINT_STATE(Connect)
		PRINT_STATE(FirstHandshake)
		PRINT_STATE(SecondHandshake)
		PRINT_STATE(Shutdown)
		PRINT_STATE(CloseSocket)
	default:
		std::cout << "Unknown state: " << (int)state << std::endl;
		return;
	}
#undef PRINT_STATE

	std::cout << "State changed to: " << state_desc << std::endl;
#endif

	this->state = state;
}