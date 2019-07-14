#include "pch.h"
#include "Server.h"


Server::Server(uint16_t readPort, uint16_t writePort)
    : listeningReadSocket(INVALID_SOCKET)
	, listeningWriteSocket(INVALID_SOCKET)
	, readPort(readPort)
	, writePort(writePort)
	, started(false)
	, cleanerStarted(false)
{
	packetFactory = PacketFactory();
}


Server::~Server()
{
	if (started)
		closeServer();

	clientPool.clear();
}

// Запуск сервера
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

	if (err = initSockets())
		return err;

	log_raw_colored(ConsoleColor::SuccessHighlighted, "The server is running");

	started = true;

	firstHandshakesHandler = std::thread(&Server::processIncomeConnection, this, false);
	firstHandshakesHandler.detach();

	secondHandshakesHandler = std::thread(&Server::processIncomeConnection, this, true);
	secondHandshakesHandler.detach();

	return 0;
}

// Отключение сервера
void Server::closeServer()
{
	if (!started)
		return;

	started = false;

	// Closing handler threads

	if (firstHandshakesHandler.joinable())
		firstHandshakesHandler.join();

	if (secondHandshakesHandler.joinable())
		secondHandshakesHandler.join();

	closeCleaner();

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
}


// Запуск клинера
void Server::startCleaner() {
	if (cleanerStarted)
		closeCleaner();

	cleaner = std::thread(&Server::inactiveClientsCleaner, this);
	cleaner.detach();

	cleanerStarted = true;

	log_raw_colored(ConsoleColor::SuccessHighlighted, "Cleaner enabled! You can disable it by \"disable_cleaner\" command");
}

// Отключение клинера
void Server::closeCleaner() {
	cleanerStarted = false;

	if (cleaner.joinable())
		cleaner.join();

	log_raw_colored(ConsoleColor::SuccessHighlighted, "Cleaner disabled! You can enable it by \"enable_cleaner\" command");
}


void Server::printCommandsList() {
	log_raw_colored(ConsoleColor::InfoHighlighted, "You can use these commands to manage the server:");
	log_raw_colored(ConsoleColor::Info,            "  \"list\"            => Print list of all active clients");
	log_raw_colored(ConsoleColor::Info,            "  \"list_detailed\"   => Print list of all active clients with extra info");
	log_raw_colored(ConsoleColor::Info,            "  \"send\"            => Send the packet to client");
	log_raw_colored(ConsoleColor::Info,            "  \"send_all\"        => Send the packet to all clients");
	log_raw_colored(ConsoleColor::Info,            "  \"save\"            => Save all data into the file");

	if(!cleanerStarted)
		log_raw_colored(ConsoleColor::Info,        "  \"enable_cleaner\"  => Enable inactive clients cleaner");
	else
		log_raw_colored(ConsoleColor::Info,        "  \"disable_cleaner\" => Disable inactive clients cleaner");

	log_raw_colored(ConsoleColor::Info,            "  \"clean\"           => Clean all inactive users");
	log_raw_colored(ConsoleColor::Info,            "  \"commands\"        => Print all available commands");
	log_raw_colored(ConsoleColor::Danger,          "  \"close\"           => Close the server");
}

// Получение числа активных клиентов
size_t Server::getActiveClientsCount() {
	size_t counter = 0;

	size_t* ctr_ptr = &counter;

	processClients(
		true, // Увеличивать counter только для активных клиентов
		[ctr_ptr](ConnectedClient& client) -> int { (*ctr_ptr)++; return 0; }
	);

	return counter;
}

// Очистка неактивных клиентов
void Server::cleanInactiveClients() {
	size_t cleaned = 0;

	clients_mutex.lock();

	auto client_it = clientPool.begin();
	while (client_it != clientPool.end()) {
		ConnectedClient& client = *(client_it->second);

		if (!client.isRunning()) {
			client_it = clientPool.erase(client_it);
			cleaned++;
		}
		else
			client_it++;
	}

	clients_mutex.unlock();

	if(cleaned) log_colored(ConsoleColor::InfoHighlighted, "Cleaned %d inactive clients", cleaned);
}

// Обход списка клиентов и их обработка функцией handler
int Server::processClients(bool onlyActive, std::function<int(ConnectedClient&)> handler) {
	int err = 0;
	clients_mutex.lock();

	for (auto client_it : clientPool) {
		ConnectedClient& client = *(client_it.second);

		if (onlyActive && !client.isRunning()) continue;

		err = handler(client);
		if (err) break; // В случае ошибки прервать выполнение
	}

	clients_mutex.unlock();
	return err;
}


SOCKET Server::initSocket(uint16_t port) {
	SOCKET result = INVALID_SOCKET;

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

	// Set socket options
	setState(ServerState::SetOpts);
	
	uint32_t value = 3000;
	uint32_t size = sizeof(value);

	// Set timeout for sending
	err = setsockopt(result, SOL_SOCKET, SO_SNDTIMEO, (char *)&value, size);
	if (err == SOCKET_ERROR) {
		wsa_print_err();
		return INVALID_SOCKET;
	}

	// Set timeout for receiving
	err = setsockopt(result, SOL_SOCKET, SO_RCVTIMEO, (char *)&value, size);
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
	// Create a read socket that receiving data from server
	setState(ServerState::CreateReadSocket);

	listeningReadSocket = initSocket(readPort);
	if (listeningReadSocket == INVALID_SOCKET)
		return 1;

	log_colored(ConsoleColor::SuccessHighlighted, "The server can accept clients on the port %d", readPort);

	// Create a write socket that sending data to the server
	setState(ServerState::CreateWriteSocket);

	listeningWriteSocket = initSocket(writePort);
	if (listeningWriteSocket == INVALID_SOCKET)
		return 1;

	log_colored(ConsoleColor::SuccessHighlighted, "The server can accept clients on the port %d", writePort);

	started = true;

	return 0;
}


// Каждые 5 секунд очищать неактивных клиентов
void Server::inactiveClientsCleaner() {
	// Задать имя потоку
	setThreadDesc(L"Cleaner");

	while (started && cleanerStarted) {
		cleanInactiveClients();

		Sleep(5000);
	}
}


// Обработка входящих подключений
void Server::processIncomeConnection(bool isReadSocket)
{
	// Задать имя потоку
	setThreadDesc(L"PIC"); // Processing Incoming Connections

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
	while (started) {
		if(getActiveClientsCount() > 10) {
			log_raw_colored(ConsoleColor::WarningHighlighted, "Client connections count limit exceeded");
			Sleep(10000);
			continue;
		}

		log_colored(ConsoleColor::InfoHighlighted, "Wait for client on port %d...", port);

		SOCKET clientSocket = accept(socket, (sockaddr*)&clientDesc, &clientLen);
		if (clientSocket == INVALID_SOCKET)
			continue;

		// Connected to client
		setState(ServerState::Connect);

		uint32_t client_ip = clientDesc.sin_addr.s_addr;

		clients_mutex.lock();

		// Получаем итератор
		auto client_it = clientPool.find(client_ip);
		if (!isReadSocket && client_it == end(clientPool)) {
			// Такого клиента нет, добавить и начать рукопожатие
			auto client = std::make_shared<ConnectedClient>(clientID++, clientDesc, clientLen);

			clientPool[client_ip] = client;

			if (client->first_handshake(clientSocket))
				log_colored(ConsoleColor::WarningHighlighted, "Error while first handshaking. Client ID: %d", client->getID());
		}
		else if (isReadSocket && client_it != end(clientPool)) {
			// Уже есть клиент с таким же IP, продолжить рукопожатие
			ConnectedClient& client = *(client_it->second);

			if (client.second_handshake(clientSocket))
				log_colored(ConsoleColor::WarningHighlighted, "Error while second handshaking. Client ID: %d", client.getID());
		}
		else {
			// Ошибка, сбросить соединение
			shutdown(clientSocket, SD_BOTH);
			closesocket(clientSocket);
		}

		clients_mutex.unlock();
	}

	// Закрываем поток
	log_colored(ConsoleColor::InfoHighlighted, "Closing PIC (Processing Incoming Connections) thread");
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
		PRINT_STATE(SetOpts)
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