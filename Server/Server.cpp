#include "pch.h"
#include "Server.h"
#include "Cleaner.h"


// Запуск сервера
ServerError Server::startServer(uint16_t authPort)
{
	if (started)
		return SW_ALREADY_STARTED;

    ServerError err;

	// Init class members
	this->authSocket = INVALID_SOCKET;
	this->authPort = authPort;
	this->started = false;

	// Initialize Winsock
	setState(ServerState::InitWinSock);

	WSADATA wsData;

    int init_winsock = WSAStartup(MAKEWORD(2, 2), &wsData);
	if (init_winsock != NO_ERROR) {
		wsa_print_err();
		return SE_INIT_WINSOCK;
	}

	if (err = initSockets())
		return err;

	LOG::raw_colored(CC_SuccessHL, "The server is running");

	started = true;

	firstHandshakesHandler  = std::thread(&Server::processIncomeConnection, this);
	secondHandshakesHandler = std::thread(&Server::processIncomeConnection, this);

	return SE_OK;
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

	Cleaner::getInstance().closeCleaner();

	clients_mutex.lock();
	clientPool.clear();
	clients_mutex.unlock();

	// Close the socket
	setState(ServerState::CloseSockets);

	if (authSocket != INVALID_SOCKET) {
		int err = closesocket(authSocket);
		if (err == SOCKET_ERROR)
			wsa_print_err();
	}

	authSocket = INVALID_SOCKET;

	WSACleanup();

	LOG::raw_colored(CC_InfoHL, "The server was stopped");
}


// Получение числа активных клиентов
size_t Server::getActiveClientsCount()
{
	return processClientsByPair(
		true, // Увеличивать counter только для активных клиентов
		nullptr
	);
}

// Нахождение клиента, удовлетворяющего условию
ConnectedClientPtr Server::findClient(bool lockMutex, bool onlyActive, std::function<bool(ConnectedClientPtr)> handler)
{
	ConnectedClientPtr result;

	if(lockMutex) clients_mutex.lock();

	auto client_it = clientPool.cbegin();
	auto end       = clientPool.cend();

	for (; client_it != end; client_it++) {
		ConnectedClientPtr client = client_it->second;

		if (onlyActive && !client->isRunning()) continue;

		if (handler(client)) { // Если клиент удовлетворяет условию - выходим из цикла
			result = client;
			break;
		}
	}

	if (lockMutex) clients_mutex.unlock();

	return result;
}

// Получить указатель на клиента по ID
ConnectedClientPtr Server::getClientByID(bool lockMutex, bool onlyActive, uint32_t ID)
{
	return findClient(
		lockMutex,
		onlyActive,
		[ID](ConnectedClientPtr client) -> bool 
		{ 
			// Проверка по ID
			if (client->getID() == ID) return true;
			return false;
		});
}

// Получить указатель на клиента по IP (возможно, с портом)
ConnectedClientPtr Server::getClientByIP(bool lockMutex, bool onlyActive, uint32_t IP, int port, bool isReadPort)
{
	return findClient(
		lockMutex,
		onlyActive,
		[IP, port, isReadPort](ConnectedClientPtr client) -> bool
		{
			// Проверка по IP (и порту)
			if (client->getIP_u32() == IP) {
				if (port == -1) return true;
				auto client_port = client->getPort(isReadPort);
				if (client_port != -1 && client_port == port) return true;
			}
			return false;
		});
}

// Получить указатель на клиента по логину
// Если указан clientID - исключает пользователя с таким ID
ConnectedClientPtr Server::getClientByLogin(bool lockMutex, bool onlyActive, uint32_t loginHash, int16_t clientID)
{
	return findClient(
		lockMutex,
		onlyActive,
		[loginHash, clientID](ConnectedClientPtr client) -> bool
	{
		// Проверка по логину
		if (client->getLoginHash() == loginHash) {
			if (clientID != -1 && client->getID() == clientID) return false;
			return true;
		}
		return false;
	});
}


// Вывести список команд
void Server::printCommandsList() const
{
	LOG::raw_colored(CC_InfoHL, "Commands for managing the server:");
	LOG::raw_colored(CC_Info, "  \"list\"          => Print list of all active clients");
	LOG::raw_colored(CC_Info, "  \"list_detailed\" => Print list of all active clients with extra info");
	LOG::raw_colored(CC_Info, "  \"send\"          => Send the packet to client");
	LOG::raw_colored(CC_Info, "  \"send_all\"      => Send the packet to all clients");
	LOG::raw_colored(CC_Info, "  \"save\"          => Save all data into the file");
	LOG::raw_colored(CC_Info, "  \"clean\"         => Clean all inactive users");
	LOG::raw_colored(CC_Info, "  \"commands\"      => Print all available commands");
	LOG::raw_colored(CC_Danger, "  \"close\"         => Close the server");
}

// Вывести список всех клиентов
void Server::printClientsList(bool ext)
{
	processClientsByPair(
		false,
		[ext](ConnectedClient& client) -> int { client.printInfo(ext); return 0; }
	);
}

// Послать пакет клиенту
void Server::send(ConnectedClientPtr client, PacketPtr packet)
{
	clients_mutex.lock();

	if (!client) {
		LOG::raw_colored(CC_Info, "Please type the client ID/IP/login");

		std::string cmd;
		std::getline(std::cin, cmd);

		client = getClientByID(false, true, std::stoi(cmd));
		if (!client) {
			IN_ADDR IP_struct;
			inet_pton(AF_INET, cmd.data(), &IP_struct);

			client = getClientByIP(false, true, IP_struct.s_addr);
			if (!client) {
				std::hash<std::string> hashObj;
				uint32_t loginHash = hashObj(cmd);

				client = getClientByLogin(false, true, loginHash);
				if (!client) {
					LOG::raw_colored(CC_WarningHL, "Client not found!"); // Клиент не найден
					return;
				}
			}
		}
	}

	if (!packet) {
		LOG::raw_colored(CC_Info, "Please type the data you want to send");

		std::string cmd;
		std::getline(std::cin, cmd);

		packet = PacketFactory::create(DT_MSG, cmd.data(), cmd.size(), false);
	}

	client->sendPacket(packet);

	clients_mutex.unlock();

	LOG::raw_colored(CC_SuccessHL, "Data was successfully sended!");
}

// Послать пакет всем клиентам
void Server::sendAll(PacketPtr packet)
{
	if (!packet) {
		LOG::raw_colored(CC_Info, "Please type the data you want to send");

		std::string cmd;
		std::getline(std::cin, cmd);

		packet = PacketFactory::create(DT_MSG, cmd.data(), cmd.size(), false);
	}

	size_t count = processClientsByPair(
		true,
		[packet](ConnectedClient& client) -> int
		{ client.sendPacket(packet); return 0; }
	);

	LOG::colored(CC_SuccessHL, "Data was successfully sended to %u clients", count);
}

// Сохранение данных клиентов
void Server::save()
{
	processClientsByPair(
		false,
		[](ConnectedClient& client) -> int { client.saveData(); return 0; }
	);

	LOG::colored(CC_SuccessHL, "Data was successfully saved!");
}

// Обход списка клиентов и их обработка функцией handler. Возвращает количество успешно обработанных клиентов.
size_t Server::processClientsByPair(bool onlyActive, std::function<int(ConnectedClient&)> handler)
{
	int err = 0;
    
    size_t processed = 0;
	clients_mutex.lock();

	for (auto pair : clientPool) {
		ConnectedClient& client = *(pair.second);

		if (onlyActive && !client.isRunning()) continue;

        if (handler) {
            err = handler(client);
            if (err) break; // В случае ошибки прервать выполнение
        }

        processed++;
	}

	clients_mutex.unlock();
	return err ? 0 : processed;
}

// Инициализация сокета по порту
SOCKET Server::initSocket(uint16_t port) // TODO: protocol
{
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

// Инициализация сокетов сервера
ServerError Server::initSockets()
{
	// Create a read socket that receiving data from server
	setState(ServerState::CreateAuthSocket);

	authSocket = initSocket(authPort);
	if (authSocket == INVALID_SOCKET) {
		wsa_print_err();
		return SE_CREATE_SOCKET;
	}

	LOG::colored(CC_SuccessHL, "The server can authorize clients on the port %d", authPort);

	started = true;

	return SE_OK;
}


// Обработка входящих подключений
void Server::processIncomeConnection()
{
	// Задать имя потоку
	setThreadDesc(L"[Server][PIC]"); // Processing Incoming Connections

	sockaddr_in clientDesc;
	int clientLen = sizeof(clientDesc);

	while (isRunning()) {
		if(getActiveClientsCount() > MAX_CLIENT_COUNT) {
			LOG::raw_colored(CC_WarningHL, "Client connections count limit exceeded");
			Sleep(10000);
			continue;
		}

		LOG::colored(CC_InfoHL, "Wait for client on port %d...", authPort);

		// Ожидание новых подключений
		setState(ServerState::Waiting);

		int select_res = 0;
		while (isRunning()) {
			fd_set s_set;
			FD_ZERO(&s_set);
			FD_SET(authSocket, &s_set);
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

		SOCKET clientSocket = accept(authSocket, (sockaddr*)&clientDesc, &clientLen);
		if (clientSocket == INVALID_SOCKET) {
			wsa_print_err();
			continue;
		}

		uint32_t client_ip   = clientDesc.sin_addr.s_addr;
		uint16_t client_port = ntohs(clientDesc.sin_port);

		clients_mutex.lock();

        // TODO: client auth

		clients_mutex.unlock();
	}

	// Закрываем поток
	LOG::colored(CC_InfoHL, "Closing PIC (Processing Incoming Connections) thread");
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
		PRINT_STATE(CreateAuthSocket)
		PRINT_STATE(Bind)
		PRINT_STATE(SetOpts)
		PRINT_STATE(Listen)
		PRINT_STATE(Waiting)
		PRINT_STATE(Connect)
		PRINT_STATE(CloseSockets)
	default:
		LOG::colored(CC_WarningHL, "Unknown state: %d", (int)state);
		return;
	}
#undef PRINT_STATE

	LOG::colored(CC_Info, "State changed to: %s", state_desc);
#endif

	this->state = state;
}