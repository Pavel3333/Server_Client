#include "pch.h"
#include <array>
#include <fstream>
#include <functional>
#include "ConnectedClient.h"


ConnectedClient::ConnectedClient(uint16_t ID, sockaddr_in clientDesc, int clientLen)
    : readSocket(INVALID_SOCKET)
    , writeSocket(INVALID_SOCKET)
    , ID(ID)
    , IP(clientDesc.sin_addr.s_addr)
    , readPort(-1)
    , writePort(-1)
    , started(false)
    , disconnected(true)
{
    // Collecting data about client IP, port and host

    inet_ntop(AF_INET, &(clientDesc.sin_addr), IP_str, 16); // get IP addr string
    getnameinfo((sockaddr*)&clientDesc, clientLen, host, NI_MAXHOST, NULL, NULL, 0); // get host

    LOG::colored(CC_InfoHL, "Client %d (IP: %s, host: %s) connected!", ID, IP_str, host);
}

ConnectedClient::~ConnectedClient()
{
    disconnect();
}

// Создание потоков
void ConnectedClient::createThreads()
{
    started = true;
    disconnected = false;

    receiver = std::thread(&ConnectedClient::receiverThread, this);
    sender = std::thread(&ConnectedClient::senderThread, this);
}

// Сброс портов и сокетов
void ConnectedClient::resetSocketsAndPorts()
{
    readSocket = INVALID_SOCKET;
    writeSocket = INVALID_SOCKET;
    readPort = -1;
    writePort = -1;
}

// Вывести информацию о клиенте
void ConnectedClient::printInfo(bool ext)
{
    LOG::colored(CC_InfoHL, "Client %d {", ID);

    const char* status = isRunning() ? "connected" : "disconnected";

    LOG::colored(CC_InfoHL, "  status: %s", status);
    LOG::colored(CC_InfoHL, "  login : %.*s", login.size(), login.data());
    LOG::colored(CC_InfoHL, "  IP    : %s", IP_str);
    LOG::colored(CC_InfoHL, "  host  : %s", host);

    if (ext) {
        LOG::colored(CC_InfoHL, "  received:      %d", receivedPackets.size());
        LOG::colored(CC_InfoHL, "  sended:        %d", sendedPackets.size());
        LOG::colored(CC_InfoHL, "  in main queue: %d", mainPackets.size());
        LOG::colored(CC_InfoHL, "  in sync queue: %d", syncPackets.size());
    }

    LOG::raw_colored(CC_InfoHL, "}");
}

bool ConnectedClient::disconnect()
{
    if (!started || disconnected)
        return false;

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

    LOG::colored(CC_InfoHL, "Connected client %d was stopped", ID);

    disconnected = true;

    return true;
}

// Сохранение данных клиента
void ConnectedClient::saveData()
{
    std::ofstream fil("all_server_data.txt", std::ios::app);
    fil.setf(std::ios::boolalpha); // Вывод true/false
    fil << *this;
    fil.close();
}

// Обработать пакет ACK
int ConnectedClient::ack_handler(PacketPtr packet)
{
    LOG::raw_colored(CC_InfoHL, std::string_view(packet->getData(), packet->getDataSize()));
    return 0;
}

// Обработать любой входящий пакет
int ConnectedClient::any_packet_handler(PacketPtr packet)
{
    LOG::raw_colored(CC_InfoHL, std::string_view(packet->getData(), packet->getDataSize()));

    /*std::string_view resp =
		"HTTP / 1.1 200 OK\r\n"
		"Date : Wed, 11 Jul 2019 11 : 20 : 59 GMT\r\n"
		"Server : Apache\r\n"
		"X - Powered - By : C++ / 17\r\n"
		"Last - Modified : Wed, 11 Jul 2019 11 : 20 : 59 GMT\r\n"
		"Content - Language : ru\r\n"
		"Content - Type : text / html; charset = utf - 8\r\n"
		"Content - Length: 16\r\n"
		"Connection : close\r\n"
		"\r\n"
		"You are welcome!";

	sendPacket(PacketFactory::create(resp.data(), resp.size(), false));*/

    return 0;
}

// Обработка входящего пакета
int ConnectedClient::handlePacketIn(std::function<int(PacketPtr)> handler, bool closeAfterTimeout)
{
    PacketPtr packet;

    int err = receiveData(packet, closeAfterTimeout);
    if (_ERROR(err))
        return err; // Произошла ошибка

    if (!packet)
        return -1;

    // Обработка пришедшего пакета
    return handler(packet);
}

// Обработка исходящего пакета
int ConnectedClient::handlePacketOut(PacketPtr packet)
{
    if (sendData(packet))
        return 1;

    if (packet->isNeedACK()) { // Если нужно подтверждение отправленного пакета
        int err = handlePacketIn( // Попробовать принять подтверждение
            std::bind(&ConnectedClient::ack_handler, this, std::placeholders::_1),
            true // Таймаут 3 секунды
        );

        if (err)
            return 2;
    }

    return 0;
}

// Поток обработки входящих пакетов
void ConnectedClient::receiverThread()
{
    // Задать имя потоку
    setThreadDesc(L"[Client %d][Receiver]", ID);

    int err = 0;

    // Ожидание любых входящих пакетов
    // Таймаут не нужен
    while (isRunning()) {
        err = handlePacketIn(
            std::bind(&ConnectedClient::any_packet_handler, this, std::placeholders::_1),
            false);

        if (err) {
            if (_ERROR(err))
                break; // Критическая ошибка или соединение сброшено
            else
                continue; // Неудачный пакет, продолжить прием
        }
    }

    // Сбрасываем соединение (сокет для чтения)
    setState(ClientState::Shutdown);

    if (readSocket != INVALID_SOCKET)
        if (shutdown(readSocket, SD_BOTH) == SOCKET_ERROR)
            wsa_print_err();

    // Закрытие сокета (чтение)
    setState(ClientState::CloseSocket);

    if (readSocket != INVALID_SOCKET)
        if (closesocket(readSocket) == SOCKET_ERROR)
            wsa_print_err();

    readSocket = INVALID_SOCKET;

    // Завершаем поток
    LOG::colored(CC_InfoHL, "Receiver thread closed");
}

// Поток отправки пакетов
void ConnectedClient::senderThread()
{
    // Задать имя потоку
    setThreadDesc(L"[Client %d][Sender]", ID);

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
            } else
                packetIt = syncPackets.erase(packetIt);
        }

        Sleep(100);
    }

    // Сбрасываем соединение (сокет для записи)
    setState(ClientState::Shutdown);

    if (writeSocket != INVALID_SOCKET)
        if (shutdown(writeSocket, SD_BOTH) == SOCKET_ERROR)
            wsa_print_err();

    // Закрытие сокета (запись)
    setState(ClientState::CloseSocket);

    if (writeSocket != INVALID_SOCKET)
        if (closesocket(writeSocket) == SOCKET_ERROR)
            wsa_print_err();

    writeSocket = INVALID_SOCKET;

    // Завершаем поток
    LOG::colored(CC_InfoHL, "Sender thread closed");
}

// Принятие данных
int ConnectedClient::receiveData(PacketPtr& dest, bool closeAfterTimeout)
{
    setState(ClientState::Receive);

    std::array<char, NET_BUFFER_SIZE> respBuff;

    while (isRunning()) {
        // Цикл принятия сообщений.  Может завершиться:
        // - после критической ошибки,
        // - после закрытия соединения,
        // - после таймаута (см. closeAfterTimeout)

        int respSize = recv(readSocket, respBuff.data(), NET_BUFFER_SIZE, 0);
        if (respSize > 0) {
            // Записываем данные от клиента
            dest = PacketFactory::create(respBuff.data(), respSize);

            // Добавить пакет
            receivedPackets.push_back(dest);

            break;
        } else if (!respSize) {
            // Соединение сброшено
            LOG::raw_colored(CC_InfoHL, "Connection closed");
            return 1;
        } else {
            int err = WSAGetLastError();

            if (err == WSAETIMEDOUT) {
                // Таймаут
                if (closeAfterTimeout)
                    return -1;
                else
                    continue;
            } else if (err == WSAEMSGSIZE) {
                // Размер пакета превысил размер буфера
                // Вывести предупреждение
                LOG::raw_colored(CC_WarningHL, "The size of received packet is larger than the buffer size!");
                return -2;
            } else if (err == WSAECONNRESET || err == WSAECONNABORTED) {
                // Соединение сброшено
                LOG::raw_colored(CC_InfoHL, "Connection closed");
                return 2;
            } else {
                // Критическая ошибка
                wsa_print_err();
                return 3;
            }
        }
    }

    return 0;
}

// Отправка данных
int ConnectedClient::sendData(PacketPtr packet)
{
    setState(ClientState::Send);

    if (packet->send(writeSocket) == SOCKET_ERROR) {
        wsa_print_err();
        return 1;
    }

    // Добавить пакет
    sendedPackets.push_back(packet);

    return 0;
}

void ConnectedClient::setState(ClientState state)
{
#ifdef _DEBUG
    const char* state_desc;

#define PRINT_STATE(X)   \
    case ClientState::X: \
        state_desc = #X; \
        break;

    switch (state) {
        PRINT_STATE(FirstHandshake);
        PRINT_STATE(SecondHandshake);
        PRINT_STATE(HelloSending);
        PRINT_STATE(HelloReceiving);
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

std::ostream& operator<<(std::ostream& os, ConnectedClient& client)
{
    os << "IP: \"" << client.getIP_str() << "\" => {" << endl
       << "  ID                    : " << client.getID() << endl
       << "  login                 : " << client.getLogin() << endl
       << "  running               : " << client.isRunning() << endl
       << "  received packets count: " << client.receivedPackets.size() << endl
       << "  sended packets count  : " << client.sendedPackets.size() << endl
       << "  main packets count    : " << client.mainPackets.size() << endl
       << "  sync packets count    : " << client.syncPackets.size() << endl
       << endl;

    if (!client.receivedPackets.empty()) {
        os << "  received packets: {" << endl;

        for (auto packet : client.receivedPackets)
            os << *packet;

        os << "  }" << endl;
    }

    if (!client.sendedPackets.empty()) {
        os << "  sended packets  : {" << endl;

        for (auto packet : client.sendedPackets)
            os << *packet;

        os << "  }" << endl;
    }

    if (!client.mainPackets.empty()) {
        // Main packets is a queue so we can't read
        // its elements without copying the queue
        // and destroying the copied queue
        std::queue<PacketPtr> mainPacketsCopy = client.mainPackets;

        os << "  main packets    : {" << endl;

        while (!mainPacketsCopy.empty()) {
            os << *(mainPacketsCopy.front());
            mainPacketsCopy.pop();
        }

        os << "  }" << endl;
    }

    if (!client.syncPackets.empty()) {
        os << "  sync packets    : {" << endl;

        for (const auto packet : client.syncPackets)
            os << *packet;

        os << "  }" << endl;
    }

    os << '}' << endl;

    return os;
}
