#include "pch.h"
#include "Server.h"
#include "Cleaner.h"


// Запуск сервера
int Server::startServer(uint16_t readPort, uint16_t writePort)
{
	if (started)
		return -1;

	// Init class members
	this->listeningReadSocket = INVALID_SOCKET;
	this->listeningWriteSocket = INVALID_SOCKET;
	this->readPort = readPort;
	this->writePort = writePort;
	this->started = false;

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

	Cleaner::getInstance().closeCleaner();

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

// Получить указатель на клиента по IP (возможно, с портом)
ConnectedClientPtr Server::getClientByLogin(bool lockMutex, bool onlyActive, std::string_view login)
{
	return findClient(
		lockMutex,
		onlyActive,
		[login](ConnectedClientPtr client) -> bool
	{
		// Проверка по логину
		if (client->getLogin() == login) return true;
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

		clients_mutex.lock();

		// Находим клиента с таким же IP
		ConnectedClientPtr found_client_same_IP = getClientByIP(false, false, client_ip);

		if (!isReadSocket) {
			if (!found_client_same_IP) {
				// Такого клиента нет, добавить и начать рукопожатие
				auto client = ConnectedClientFactory::create(clientDesc, clientLen);

				clientPool[client->getID()] = client;

				client->first_handshake(clientSocket, client_port);
			}
			else {
				// Уже есть клиент с таким же IP
				if (!found_client_same_IP->isRunning()) {
					// Если было когда-то разорвано соединение
					if (!found_client_same_IP->isDisconnected())
						found_client_same_IP->disconnect();
					// Начать новое рукопожатие
					found_client_same_IP->first_handshake(clientSocket, client_port);
				}
				else {
					// Ошибка, сбросить соединение
					shutdown(clientSocket, SD_BOTH);
					closesocket(clientSocket);
				}
			}
		}
		else if (isReadSocket && found_client_same_IP) {
			// Уже есть клиент с таким же IP
			if (!found_client_same_IP->isRunning()) {
				// Если было когда-то разорвано соединение
				if (!found_client_same_IP->isDisconnected())
					found_client_same_IP->disconnect();
				// Продолжить рукопожатие
				int err = found_client_same_IP->second_handshake(clientSocket, client_port);
				if (err) {
					// В случае ошибки выводим код ошибки и сбрасываем соединение
					log_colored(ConsoleColor::DangerHighlighted, "Client %d: second handshake failed: error %d", err);
					shutdown(clientSocket, SD_BOTH);
					closesocket(clientSocket);
				}
				else {
					// Ищем клиент с таким же логином
					ConnectedClientPtr found_client_same_login = getClientByLogin(false, false, found_client_same_IP->getLogin());

					if (!found_client_same_login)
						// Клиент не найден - создаем потоки-обработчики пакетов
						found_client_same_IP->createThreads();
					else if(!found_client_same_login->isRunning()) {
						// Есть клиент с таким же логином, но с разорванным соединением

						// TODO: перенести порты и сокеты в клиента с таким же логином
						// TODO: Старый клиент дисконнектнуть, сбросив порты и сокеты
					}
					else {
						// Уже подключен клиент с таким логином
						// Сбрасываем соединение
						shutdown(clientSocket, SD_BOTH);
						closesocket(clientSocket);
					}
				}
			}
			else {
				// Ошибка, сбросить соединение
				shutdown(clientSocket, SD_BOTH);
				closesocket(clientSocket);
			}
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