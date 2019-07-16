#include "pch.h"
#include "Server.h"


Server::Server(uint16_t readPort, uint16_t writePort)
    : listeningReadSocket(INVALID_SOCKET)
	, listeningWriteSocket(INVALID_SOCKET)
	, readPort(readPort)
	, writePort(writePort)
	, started(false)
	, cleanerStarted(false)
	, cleanerMode(CleanerMode::OnlyDisconnect)
{
}


Server::~Server()
{
	closeServer();
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

	firstHandshakesHandler  = std::thread(&Server::processIncomeConnection, this, false);
	secondHandshakesHandler = std::thread(&Server::processIncomeConnection, this, true);

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

	clients_mutex.lock();
	clientPool.clear();
	clients_mutex.unlock();

	// Close the socket
	setState(ServerState::CloseSockets);

	if (listeningReadSocket != INVALID_SOCKET) {
		int err = closesocket(listeningReadSocket);
		if (err == SOCKET_ERROR)
			wsa_print_err();
	}

	listeningReadSocket = INVALID_SOCKET;

	if (listeningWriteSocket != INVALID_SOCKET) {
		int err = closesocket(listeningWriteSocket);
		if (err == SOCKET_ERROR)
			wsa_print_err();
	}

	listeningWriteSocket = INVALID_SOCKET;

	WSACleanup();

	log_raw_colored(ConsoleColor::InfoHighlighted, "The server was stopped");
}


// Запуск клинера
void Server::startCleaner()
{
	if (cleanerStarted)
		closeCleaner();

	cleaner = std::thread(&Server::inactiveClientsCleaner, this);

	cleanerStarted = true;

	cleanerMode = CleanerMode::OnlyDisconnect;

	log_raw_colored(ConsoleColor::SuccessHighlighted, "Cleaner enabled!");
	printCleanerMode();
}

void Server::printCleanerCommands() {
	log_raw_colored(ConsoleColor::InfoHighlighted, "Commands for managing the cleaner:");
	if (!cleanerStarted)
		log_raw_colored(ConsoleColor::Info,        "  \"enable_cleaner\"      => Enable inactive clients cleaner");
	else {
		log_raw_colored(ConsoleColor::Info,        "  \"get_cleaner_mode\"    => Print cleaner mode");
		log_raw_colored(ConsoleColor::Info,        "  \"change_cleaner_mode\" => Disable inactive clients cleaner");
		log_raw_colored(ConsoleColor::Warning,     "  \"disable_cleaner\"     => Disable inactive clients cleaner");
	}
}

void Server::printCleanerMode() {
	const char* mode = "-";

	if      (cleanerMode == CleanerMode::OnlyDisconnect)
		mode = "Only disconnect";
	else if (cleanerMode == CleanerMode::AgressiveMode)
		mode = "Agressive mode";

	log_colored(ConsoleColor::InfoHighlighted, "Cleaner mode: %s", mode);
}

// Очистка неактивных клиентов
void Server::cleanInactiveClients(bool ext)
{
	size_t cleaned = 0;

	clients_mutex.lock();

	auto client_it = clientPool.begin();
	while (client_it != clientPool.end()) {
		ConnectedClient& client = *(client_it->second);

		if (!client.isRunning() && !client.isDisconnected()) {
			if (client.disconnect()) {
				cleaned++;
				if (cleanerMode == CleanerMode::AgressiveMode) {
					client_it = clientPool.erase(client_it);
					continue;
				}
			}
		}

		client_it++;
	}

	clients_mutex.unlock();

	if (cleaned) {
		const char* verb = "Disconnected";
		if (cleanerMode == CleanerMode::AgressiveMode)
			verb = "Cleaned";
		
		log_colored(ConsoleColor::InfoHighlighted, "%s %d inactive clients", verb, cleaned);
	}
	else if (ext)
		log_raw_colored(ConsoleColor::InfoHighlighted, "All clients are active");
}

// Отключение клинера
void Server::closeCleaner()
{
	if(!cleanerStarted)
		return;

	cleanerStarted = false;

	if (cleaner.joinable())
		cleaner.join();

	log_raw_colored(ConsoleColor::SuccessHighlighted, "Cleaner disabled!");
	printCleanerCommands();
}


void Server::printCommandsList()
{
	log_raw_colored(ConsoleColor::InfoHighlighted, "Commands for managing the server:");
	log_raw_colored(ConsoleColor::Info,            "  \"list\"          => Print list of all active clients");
	log_raw_colored(ConsoleColor::Info,            "  \"list_detailed\" => Print list of all active clients with extra info");
	log_raw_colored(ConsoleColor::Info,            "  \"send\"          => Send the packet to client");
	log_raw_colored(ConsoleColor::Info,            "  \"send_all\"      => Send the packet to all clients");
	log_raw_colored(ConsoleColor::Info,            "  \"save\"          => Save all data into the file");
	log_raw_colored(ConsoleColor::Info,            "  \"clean\"         => Clean all inactive users");
	log_raw_colored(ConsoleColor::Info,            "  \"commands\"      => Print all available commands");
	log_raw_colored(ConsoleColor::Danger,          "  \"close\"         => Close the server");
	printCleanerCommands();
}

// Получение числа активных клиентов
size_t Server::getActiveClientsCount()
{
	size_t counter = 0;

	size_t* ctr_ptr = &counter;

	processClientsByPair(
		true, // Увеличивать counter только для активных клиентов
		[ctr_ptr](ConnectedClient& client) -> int { (*ctr_ptr)++; return 0; }
	);

	return counter;
}


// Обход списка клиентов и их обработка функцией handler
int Server::processClientsByPair(bool onlyActive, std::function<int(ConnectedClient&)> handler)
{
	int err = 0;
	clients_mutex.lock();

	for (auto pair : clientPool) {
		ConnectedClient& client = *(pair.second);

		if (onlyActive && !client.isRunning()) continue;

		err = handler(client);
		if (err) break; // В случае ошибки прервать выполнение
	}

	clients_mutex.unlock();
	return err;
}

// Нахождение итератора клиента, удовлетворяющего условию
ConnectedClientConstIter Server::findClientIter(bool onlyActive, std::function<bool(ConnectedClient&)> handler)
{
	clients_mutex.lock();

	auto client_it = clientPool.cbegin();
	auto end       = clientPool.cend();

	for (; client_it != end; client_it++) {
		ConnectedClient& client = *(client_it->second);

		if (onlyActive && !client.isRunning()) continue;

		if (handler(client)) break; // Если клиент удовлетворяет условию - выходим из цикла
	}

	clients_mutex.unlock();

	return client_it;
}

// Получить итератор клиента по ID
ConnectedClientConstIter Server::getClientByID(bool onlyActive, uint32_t ID)
{
	return findClientIter(
		onlyActive,
		[ID](ConnectedClient& client) -> bool 
		{ 
			// Проверка по ID
			if (client.getID() == ID) return true;
			return false;
		});
}

// Получить итератор клиента по IP (возможно, с портом)
ConnectedClientConstIter Server::getClientByIP(bool onlyActive, uint32_t IP, int port, bool isReadPort)
{
	return findClientIter(
		onlyActive,
		[IP, port, isReadPort](ConnectedClient& client) -> bool
		{
			// Проверка по IP (и порту)
			if (client.getIP_u32() == IP) {
				if (port == -1) return true;
				auto client_port = client.getPort(isReadPort);
				if (client_port != -1 && client_port == port) return true;
			}
			return false;
		});
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
	
	uint32_t value = TIMEOUT * 1000;
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
	if (listeningReadSocket == INVALID_SOCKET) {
		wsa_print_err();
		return 1;
	}

	log_colored(ConsoleColor::SuccessHighlighted, "The server can accept clients on the port %d", readPort);

	// Create a write socket that sending data to the server
	setState(ServerState::CreateWriteSocket);

	listeningWriteSocket = initSocket(writePort);
	if (listeningWriteSocket == INVALID_SOCKET) {
		wsa_print_err();
		return 1;
	}

	log_colored(ConsoleColor::SuccessHighlighted, "The server can accept clients on the port %d", writePort);

	started = true;

	return 0;
}


// Каждые 5 секунд очищать неактивных клиентов
void Server::inactiveClientsCleaner() {
	// Задать имя потоку
	setThreadDesc(L"[Server][Cleaner]");

	while (isRunning() && cleanerStarted) {
		cleanInactiveClients();

		Sleep(5000);
	}
}


// Обработка входящих подключений
void Server::processIncomeConnection(bool isReadSocket)
{
	// Задать имя потоку
	setThreadDesc(L"[Server][PIC]"); // Processing Incoming Connections

	// Init local vars
	uint16_t port = readPort;
	SOCKET socket = listeningReadSocket;

	if (!isReadSocket) {
		port = writePort;
		socket = listeningWriteSocket;
	}

	sockaddr_in clientDesc;
	int clientLen = sizeof(clientDesc);

	// 10 clients limit
	while (isRunning()) {
		if(getActiveClientsCount() > 10) {
			log_raw_colored(ConsoleColor::WarningHighlighted, "Client connections count limit exceeded");
			Sleep(10000);
			continue;
		}

		log_colored(ConsoleColor::InfoHighlighted, "Wait for client on port %d...", port);

		// Ожидание новых подключений
		setState(ServerState::Waiting);

		int select_res = 0;
		while (isRunning()) {
			fd_set s_set;
			FD_ZERO(&s_set);
			FD_SET(socket, &s_set);
			timeval timeout = { TIMEOUT, 0 }; // Таймаут

			select_res = select(select_res + 1, &s_set, 0, 0, &timeout);
			if (select_res) break;

			Sleep(1000);
		}

		if (!isRunning())
			break;

		if (select_res == SOCKET_ERROR)
			wsa_print_err();

		if (select_res <= 0)
			continue;

		// Connection to client
		setState(ServerState::Connect);

		SOCKET clientSocket = accept(socket, (sockaddr*)&clientDesc, &clientLen);
		if (clientSocket == INVALID_SOCKET) {
			wsa_print_err();
			continue;
		}

		uint32_t client_ip   = clientDesc.sin_addr.s_addr;
		uint16_t client_port = ntohs(clientDesc.sin_port);

		// Получаем итератор
		auto client_it = getClientByIP(false, client_ip);

		clients_mutex.lock();

		auto end = clientPool.cend();
		if (!isReadSocket && client_it == end) {
			// Такого клиента нет, добавить и начать рукопожатие
			auto client = ConnectedClientFactory::create(clientDesc, clientLen);

			clientPool[client->getID()] = client;

			client->first_handshake(clientSocket, client_port);
		}
		else if (isReadSocket && client_it != end) {
			// Уже есть клиент с таким же IP, продолжить рукопожатие
			ConnectedClient& client = *(client_it->second);

			if (!client.isRunning())
				client.second_handshake(clientSocket, client_port);
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
		PRINT_STATE(Waiting)
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