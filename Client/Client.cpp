#include "pch.h"
#include <array>
#include "Client.h"

ERR Client::init(std::string_view login, std::string_view pass, PCSTR IP, uint16_t authPort, uint16_t dataPort)
{
    ERR err;

	// Init class members
	this->authPort = authPort;
	this->dataPort = dataPort;
	this->ID = 0;
	this->IP = IP;
	this->started = false;

	// Initialize Winsock
	setState(ClientState::InitWinSock);

	WSADATA wsaData;

	int init_winsock = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (init_winsock != NO_ERROR) {
		wsa_print_err();
		return E_INIT_WINSOCK;
	}

    // Create an authorization socket
    setState(ClientState::CreateAuthSocket);

    err = initSocket(authSocket, authPort, IPPROTO_TCP);
    if (_ERROR(err))
        return err;

    err = connect2server(authSocket, authPort);
    if (!SUCCESS(err))
        return err;

    started = true;

    LOG::colored(CC_SuccessHL, "The client is authorizing on the %d port", authPort);

    err = authorize(login, pass);
	if (_ERROR(err))
        return err;

    // Create an authorization socket
    setState(ClientState::CreateDataSocket);

    err = initSocket(dataSocket, dataPort, IPPROTO_TCP); // TODO: make UDP in the ClientUDP
    if (_ERROR(err))
        return err;

    createThreads();

	return E_OK;
}

void Client::disconnect()
{
	if (!started)
		return;

	started = false;

	// Closing handler threads

	if (receiver.joinable())
		receiver.join();

	if (sender.joinable())
		sender.join();

    // Closing sockets
    setState(ClientState::CloseSocket);

    dataSocket.close();
    authSocket.close();

	// Clear all data

	receivedPackets.clear();
	sendedPackets.clear();
	syncPackets.clear();

	while (!mainPackets.empty())
		mainPackets.pop();

	WSACleanup();

	LOG::raw_colored(CC_InfoHL, "The client was stopped");
}

void Client::printCommandsList() const
{
	LOG::raw_colored(CC_InfoHL, "You can use these commands to manage the client:");
	LOG::raw_colored(CC_Info,   "  \"send\"     => Send the packet to server");
	LOG::raw_colored(CC_Info,   "  \"commands\" => Print all available commands");
	LOG::raw_colored(CC_Danger, "  \"close\"    => Close the client");
}


ERR Client::initSocket(Socket& sock, uint16_t port, IPPROTO protocol)
{
    ERR err = sock.init(IP, port, protocol);
    if (!SUCCESS(err))
        wsa_print_err();
    if (_ERROR(err))
        return err;

    if (protocol == IPPROTO_TCP) {
        // Set socket options
        setState(ClientState::SetOpts);

        err = sock.setTimeout(TIMEOUT, SO_SNDTIMEO);
        if (_ERROR(err))
            return err;
        else if (WARNING(err))
            LOG::colored(CC_WarningHL, "Cannot to set send timeout");

        err = sock.setTimeout(TIMEOUT, SO_RCVTIMEO);
        if (_ERROR(err))
            return err;
        else if (WARNING(err))
            LOG::colored(CC_WarningHL, "Cannot to set receive timeout");
    }

	return E_OK;
}

ERR Client::connect2server(Socket& sock, uint16_t port) {
    
    ERR err = sock.connect(IP, port);
    if (_ERROR(err)) {
        wsa_print_err();
        return err;
    }
    else if (WARNING(err)) {
        if (err == W_TIMEOUT)
            LOG::raw_colored(CC_WarningHL, "Unable to connect to the server: timeout");
        else
            LOG::colored(CC_WarningHL, "Unable to connect to the server: warning code %d", err);
    }

    return err;
}

ERR Client::authorize(std::string_view login, std::string_view pass)
{
    int err;

	// Authorization
	setState(ClientState::Auth);

    ClientAuthHeader clientAuthHeader {
        static_cast<uint8_t>(login.size()),
        static_cast<uint8_t>(pass.size())
    };

    PacketPtr clientAuth = PacketFactory::create_from_struct(DT_AUTH_CLIENT, clientAuthHeader, true);
    clientAuth->writeData(login);
    clientAuth->writeData(pass);

    if (sendData(clientAuth))
        return E_AUTH_CLIENT_SEND;

    PacketPtr serverAuth;
	err = receiveData(serverAuth, false);
	if (_ERROR(err))
		// Критическая ошибка или соединение сброшено
		return E_AUTH_SERVER_RECV;

	// Packet handling

	if (!serverAuth)
		// Empty packet
		return E_AUTH_SERVER_EMPTY;
	else if (serverAuth->getDataSize() < sizeof(ServerAuthHeader))
		// Packet size is incorrect
		return E_AUTH_SERVER_SIZE;

	auto serverAuthHeader = reinterpret_cast<const ServerAuthHeader*>(
        serverAuth->getData());

    if (_ERROR(serverAuthHeader->errorCode)) {
        LOG::colored(CC_DangerHL, "Auth error: server returned %d", serverAuthHeader->errorCode);
        return E_AUTH_SERVER_GOTERR;
    }
    else if (WARNING(serverAuthHeader->errorCode))
        LOG::colored(CC_WarningHL, "Auth warning %d", serverAuthHeader->errorCode);

    std::string_view token = serverAuth->readData(serverAuthHeader->tokenSize);

	LOG::colored(CC_SuccessHL, "Client authorized successfully! Client token: %.*s", token.size(), token.data());

	return E_OK;
}


// Обработка входящего пакета
ERR Client::handlePacketIn(bool closeAfterTimeout)
{
	PacketPtr packet;

	ERR err = receiveData(packet, closeAfterTimeout);
	if (_ERROR(err))
		return err; // Произошла ошибка

	if (!packet)
        return W_HAMDLE_IN_PACKET_EMPTY;

	// Обработка пришедшего пакета
    return Handler::handle_packet(packet);
}

// Обработка исходящего пакета
ERR Client::handlePacketOut(PacketPtr packet)
{
    ERR err = sendData(packet);
	if (_ERROR(err))
		return E_HANDLE_OUT_SEND;

	if (packet->isNeedACK()) {                                                 // Если нужно подтверждение отправленного пакета
        err = handlePacketIn(true);                                            // Попробовать принять подтверждение

		if (_ERROR(err))
			return err;
	}

	return E_OK;
}


// Поток обработки входящих пакетов
void Client::receiverThread()
{
	// Задать имя потоку
	setThreadDesc(L"[Receiver]");

	ERR err = E_OK;

	// Ожидание любых входящих пакетов
	// Таймаут не нужен
	while (isRunning()) {
		err = handlePacketIn(false);

	    if      (_ERROR(err))  break;    // Критическая ошибка или соединение сброшено
		else if (WARNING(err)) continue; // Неудачный пакет, продолжить прием        
	}

    if (!_ERROR(err))
        wsa_print_err();

	// Сбрасываем соединение (сокет для чтения)
	setState(ClientState::Shutdown);

    err = dataSocket.shutdown(SD_RECEIVE);
	if (!SUCCESS(err))
		wsa_print_err();

	// Завершаем поток
	LOG::colored(CC_InfoHL, "Receiver thread closed");
}

// Поток отправки пакетов
void Client::senderThread()
{
	// Задать имя потоку
	setThreadDesc(L"[Sender]");

	while (isRunning()) {
		// Обработать основные пакеты
		while (!mainPackets.empty()) {
			PacketPtr packet = mainPackets.back();

			if (handlePacketOut(packet)) {
				LOG::colored(CC_Warning, "Packet %d not confirmed, adding to sync queue", packet->getID());
				syncPackets.push_back(packet);
			}

			mainPackets.pop();
		}

		// Обработать пакеты, не подтвержденные сервером
		auto packetIt = syncPackets.begin();
		while (packetIt != syncPackets.end()) {
			if (handlePacketOut(*packetIt)) {
				LOG::colored(CC_Warning, "Sync packet %d not confirmed", (*packetIt)->getID());
				packetIt++;
			}
			else packetIt = syncPackets.erase(packetIt);
		}

		Sleep(100);
	}

	// Сбрасываем соединение (сокет для записи)
	setState(ClientState::Shutdown);

    ERR err = dataSocket.shutdown(SD_SEND);
    if (!SUCCESS(err))
        wsa_print_err();

	// Завершаем поток
	LOG::colored(CC_InfoHL, "Sender thread closed");
}

// Создание потоков
void Client::createThreads()
{
	receiver = std::thread(&Client::receiverThread, this);
	sender   = std::thread(&Client::senderThread,   this);
}


// Принятие данных
ERR Client::receiveData(PacketPtr& dest, bool closeAfterTimeout)
{
	setState(ClientState::Receive);

	std::array<char, NET_BUFFER_SIZE> respBuff;

	while (isRunning()) {
		// Цикл принятия сообщений.  Может завершиться:
		// - после критической ошибки,
		// - после закрытия соединения,
		// - после таймаута (см. closeAfterTimeout)

		int respSize = recv(dataSocket.getSocket(), respBuff.data(), NET_BUFFER_SIZE, 0);
		if      (respSize > 0) {
			// Записываем данные от сервера
			dest = PacketFactory::create(respBuff.data(), respSize);

			// Добавить пакет
			receivedPackets.push_back(dest);

			break;
		}
		else if (!respSize) {
			// Соединение сброшено
			LOG::raw_colored(CC_InfoHL, "Connection closed");
			return E_CONN_CLOSED;
		}
		else switch(WSAGetLastError()) {
        case WSAETIMEDOUT:
            // Таймаут
            if (closeAfterTimeout) return W_TIMEOUT;
            else                   continue;
        case WSAEMSGSIZE:
            // Размер пакета превысил размер буфера
            // Вывести предупреждение
            LOG::raw_colored(CC_WarningHL, "The size of received packet is larger than the buffer size!");
            return W_MSG_SIZE;
        case WSAECONNRESET:
        case WSAECONNABORTED:
            // Соединение сброшено
            LOG::raw_colored(CC_InfoHL, "Connection closed");
            return E_CONN_CLOSED;
        default:
            // Критическая ошибка
            wsa_print_err();
            return E_RECV;
		}
	}

	return E_OK;
}

// Отправка данных
ERR Client::sendData(PacketPtr packet)
{
	setState(ClientState::Send);

	if (packet->send(dataSocket) == SOCKET_ERROR) {
		wsa_print_err();
		return E_SEND;
	}

	// Добавить пакет
	sendedPackets.push_back(packet);

	return E_OK;
}


void Client::setState(ClientState state)
{
#ifdef _DEBUG
	const char* state_desc;

#define PRINT_STATE(X) case ClientState::X: \
	state_desc = #X;                        \
	break;

	switch (state) {
		PRINT_STATE(InitWinSock);
		PRINT_STATE(SetOpts);
		PRINT_STATE(CreateAuthSocket);
		PRINT_STATE(Auth);
		PRINT_STATE(CreateDataSocket);
		PRINT_STATE(Send);
		PRINT_STATE(Receive);
		PRINT_STATE(Shutdown);
		PRINT_STATE(CloseSocket);
	default:
		LOG::colored(CC_WarningHL, "Unknown state: %d", (int)state);
		return;
	}
#undef PRINT_STATE

	LOG::colored(CC_Info, "State changed to: %s", state_desc);
#endif

	this->state = state;
}