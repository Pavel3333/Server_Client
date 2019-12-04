#include "pch.h"
#include <array>
#include "Client.h"

int Client::init(std::string_view login, std::string_view pass, PCSTR IP, uint16_t authPort, uint16_t dataPort)
{
    int err;

	// Init class members
	this->authPort = authPort;
	this->dataPort = dataPort;
	this->ID = 0;
	this->IP = IP;
	this->started = false;

	// Initialize Winsock
	setState(ClientState::InitWinSock);

	WSADATA wsaData;

	err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != NO_ERROR) {
		wsa_print_err();
		return 1;
	}

    // Create an authorization socket
    setState(ClientState::CreateAuthSocket);

    err = connect2server(authSocket, authPort, IPPROTO_TCP);
    if (_ERROR(err))
        return 2;

    started = true;

    LOG::colored(CC_SuccessHL, "The client is authorizing on the %d port", authPort);

    err = authorize(login, pass);
	if (err) return err;

    // Create an authorization socket
    setState(ClientState::CreateDataSocket);

    err = connect2server(dataSocket, dataPort, IPPROTO_TCP); // TODO: make UDP in the ClientUDP
    if (_ERROR(err))
        return 3;

    //createThreads();

	return 0;
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


ERR Client::connect2server(Socket& sock, uint16_t port, IPPROTO protocol)
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

	// Connect to server
    err = sock.connect(IP, port);
	if (_ERROR(err)) {
        switch (WSAGetLastError()) {
        case WSAETIMEDOUT:
            // Таймаут
            LOG::raw_colored(CC_WarningHL, "Unable to connect to the server: timeout");
            break;
        default:
            // Критическая ошибка
            wsa_print_err();
        }
		return err;
	}

	return E_OK;
}

int Client::authorize(std::string_view login, std::string_view pass)
{
    int err;

	// Authorization
	setState(ClientState::Auth);

    ClientAuthPacket clientAuthRaw { login, pass };

    PacketPtr clientAuth = PacketFactory::create_from_struct(clientAuthRaw, true);
    if (sendData(clientAuth))
        return 1;

    PacketPtr serverAuth;
	err = receiveData(serverAuth, false);
	if (err > 0)
		// Критическая ошибка или соединение сброшено
		return 1;

	// Packet handling

	if (!serverAuth)
		// Empty packet
		return 2;
	else if (serverAuth->getDataSize() != sizeof(ServerAuthPacket))
		// Packet size is incorrect
		return 3;

	auto serverAuthRaw = reinterpret_cast<const ServerAuthPacket*>(
        serverAuth->getData());

    if (_ERROR(serverAuthRaw->errorCode)) {
        LOG::colored(CC_DangerHL, "Auth error: server returned %d", serverAuthRaw->errorCode);
        return 4;
    }
    else if (WARNING(serverAuthRaw->errorCode))
        LOG::colored(CC_WarningHL, "Auth warning %d", serverAuthRaw->errorCode);

    if (serverAuthRaw->tokenSize >= TOKEN_MAX_SIZE)
        return 5;

    token = std::string_view(serverAuthRaw->token, serverAuthRaw->tokenSize);

	LOG::colored(CC_SuccessHL, "Client authorized successfully! Client token: %.*s", token.size(), token.data());

	return 0;
}


// Обработать пакет ACK
int Client::ack_handler(PacketPtr packet)
{
	LOG::raw_colored(CC_InfoHL, std::string_view(packet->getData(), packet->getDataSize()));
	return 0;
}

// Обработать любой входящий пакет
int Client::any_packet_handler(PacketPtr packet)
{
	LOG::raw_colored(CC_InfoHL, std::string_view(packet->getData(), packet->getDataSize()));
	return 0;
}


// Обработка входящего пакета
int Client::handlePacketIn(std::function<int(PacketPtr)> handler, bool closeAfterTimeout)
{
	PacketPtr packet;

	int err = receiveData(packet, closeAfterTimeout);
	if (err)
		return err; // Произошла ошибка

	if (!packet) return -1;

	// Обработка пришедшего пакета
	return handler(packet);
}

// Обработка исходящего пакета
int Client::handlePacketOut(PacketPtr packet)
{
	if (sendData(packet))
		return 1;

	if (packet->isNeedACK()) {                                                     // Если нужно подтверждение отправленного пакета
		int err = handlePacketIn(                                                  // Попробовать принять подтверждение
			std::bind(&Client::ack_handler, this, std::placeholders::_1),
			true                                                                   // Таймаут 3 секунды
		);

		if (err)
			return 2;
	}

	return 0;
}


// Поток обработки входящих пакетов
void Client::receiverThread()
{
	// Задать имя потоку
	setThreadDesc(L"[Receiver]");

	int err = 0;

	// Ожидание любых входящих пакетов
	// Таймаут не нужен
	while (isRunning()) {
		err = handlePacketIn(
			std::bind(&Client::any_packet_handler, this, std::placeholders::_1),
			false
		);

		if (err) {
			if (err > 0) break;    // Критическая ошибка или соединение сброшено
			else         continue; // Неудачный пакет, продолжить прием        
		}
	}

	// Сбрасываем соединение (сокет для чтения)
	setState(ClientState::Shutdown);

    err = dataSocket.shutdown(SD_RECEIVE);
	if (!SUCCESS(err))
		wsa_print_err();

	// Закрытие сокета (чтение)
	setState(ClientState::CloseSocket);

    err = dataSocket.close();
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

    // TODO: закрывать сокет нужно только после того, как сокет будет закрыт на чтение
	// Закрытие сокета (запись)
	setState(ClientState::CloseSocket);

    err = dataSocket.close();
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
int Client::receiveData(PacketPtr& dest, bool closeAfterTimeout)
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
			return 1;
		}
		else {
			int err = WSAGetLastError();

			if      (err == WSAETIMEDOUT) {
				// Таймаут
				if (closeAfterTimeout) return -1;
				else                   continue;
			}
			else if (err == WSAEMSGSIZE) {
				// Размер пакета превысил размер буфера
				// Вывести предупреждение
				LOG::raw_colored(CC_WarningHL, "The size of received packet is larger than the buffer size!");
				return -2;
			}
			else if (err == WSAECONNRESET || err == WSAECONNABORTED) {
				// Соединение сброшено
				LOG::raw_colored(CC_InfoHL, "Connection closed");
				return 2;
			}
			else {
				// Критическая ошибка
				wsa_print_err();
				return 3;
			}
		}
	}

	return 0;
}

// Отправка данных
int Client::sendData(PacketPtr packet)
{
	setState(ClientState::Send);

	if (packet->send(dataSocket) == SOCKET_ERROR) {
		wsa_print_err();
		return 1;
	}

	// Добавить пакет
	sendedPackets.push_back(packet);

	return 0;
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