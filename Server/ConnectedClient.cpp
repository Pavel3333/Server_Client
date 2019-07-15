#include "pch.h"
#include <array>
#include <functional>
#include "ConnectedClient.h"


ConnectedClient::ConnectedClient(uint16_t ID, sockaddr_in clientDesc, int clientLen)
	: readSocket(INVALID_SOCKET)
	, writeSocket(INVALID_SOCKET)
	, ID(ID)
	, IP(clientDesc.sin_addr.s_addr)
	, started(false)
{
	// Collecting data about client IP, port and host

	inet_ntop(AF_INET, &(clientDesc.sin_addr), IP_str, 16);                          // get IP addr string
	getnameinfo((sockaddr*)&clientDesc, clientLen, host, NI_MAXHOST, NULL, NULL, 0); // get host

	log_colored(ConsoleColor::InfoHighlighted, "Client %d (IP: %s, host: %s) connected!", ID, IP_str, host);
}

ConnectedClient::~ConnectedClient() {
	disconnect();
}


// Первое рукопожатие с соединенным клиентом
int ConnectedClient::first_handshake(SOCKET socket)
{
	// Присвоить сокет на запись
	setState(ClientState::FirstHandshake);

	writeSocket = socket;

	return 0;
}

// Второе рукопожатие с соединенным клиентом
int ConnectedClient::second_handshake(SOCKET socket)
{
	// Присвоить сокет на чтение
	// Отправить Hello пакет и ожидать ответ
	// Создать потоки-обработчики
	setState(ClientState::SecondHandshake);

	readSocket = socket;
	started = true;

	// TODO: Отправить Hello пакет и ожидать ответ

	createThreads();

	return 0;
}


void ConnectedClient::getInfo(bool ext)
{
	log_colored(ConsoleColor::InfoHighlighted, "Client %d {", ID);

	log_colored(ConsoleColor::InfoHighlighted, "  IP:   %s", IP_str);
	log_colored(ConsoleColor::InfoHighlighted, "  host: %s", host);

	if (ext) {
		log_colored(ConsoleColor::InfoHighlighted, "  received:      %d", receivedPackets.size());
		log_colored(ConsoleColor::InfoHighlighted, "  sended:        %d", sendedPackets.size());
		log_colored(ConsoleColor::InfoHighlighted, "  in main queue: %d", mainPackets.size());
		log_colored(ConsoleColor::InfoHighlighted, "  in sync queue: %d", syncPackets.size());
	}

	log_raw_colored(ConsoleColor::InfoHighlighted, "}");
}

void ConnectedClient::disconnect() {
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

	log_colored(ConsoleColor::InfoHighlighted, "Connected client %d was stopped", ID);
}


// Обработать пакет ACK
int ConnectedClient::ack_handler(PacketPtr packet)
{
	log_raw(std::string_view(packet->data, packet->size));
	return 0;
}

// Обработать любой входящий пакет
int ConnectedClient::any_packet_handler(PacketPtr packet)
{
	log_raw_colored(ConsoleColor::Info, std::string_view(packet->data, packet->size));

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

	sendPacket(packetFactory.create(resp.data(), resp.size(), false));*/

	return 0;
}


// Обработка входящего пакета
int ConnectedClient::handlePacketIn(std::function<int(PacketPtr)> handler, bool closeAfterTimeout) {
	PacketPtr packet;

	int err = receiveData(packet, closeAfterTimeout);
	if (err)
		return err; // Произошла ошибка

	if (!packet) return -1;

	// Обработка пришедшего пакета
	return handler(packet);
}

// Обработка исходящего пакета
int ConnectedClient::handlePacketOut(PacketPtr packet) {
	if (sendData(packet))
		return 1;

	if (packet->needACK) {                                                         // Если нужно подтверждение отправленного пакета
		int err = handlePacketIn(                                                  // Попробовать принять подтверждение
			std::bind(&ConnectedClient::ack_handler, this, std::placeholders::_1),
			true                                                                   // Таймаут 3 секунды
		);

		if (err)
			return 2;
	}

	return 0;
}


// Поток обработки входящих пакетов
void ConnectedClient::receiverThread() {
	// Задать имя потоку
	setThreadDesc(L"Receiver");

	int err = 0;

	// Ожидание любых входящих пакетов
	// Таймаут не нужен
	while (isRunning()) {
		err = handlePacketIn(
			std::bind(&ConnectedClient::any_packet_handler, this, std::placeholders::_1),
			false
		);

		if (err) {
			if (err > 0) break;    // Критическая ошибка или соединение сброшено
			else         continue; // Неудачный пакет, продолжить прием        
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
	log_colored(ConsoleColor::InfoHighlighted, "Receiver thread closed");
}

// Поток отправки пакетов
void ConnectedClient::senderThread()
{
	// Задать имя потоку
	setThreadDesc(L"Sender");

	while (isRunning()) {
		// Обработать основные пакеты
		while (!mainPackets.empty()) {
			PacketPtr packet = mainPackets.back();

			if (handlePacketOut(packet)) {
				log_colored(ConsoleColor::Warning, "Packet %d not confirmed, adding to sync queue", packet->ID);
				syncPackets.push_back(packet);
			}

			mainPackets.pop();
		}

		// Обработать пакеты, не подтвержденные сервером
		auto packetIt = syncPackets.begin();
		while (packetIt != syncPackets.end()) {
			if (handlePacketOut(*packetIt)) {
				log_colored(ConsoleColor::Warning, "Sync packet %d not confirmed", (*packetIt)->ID);
				packetIt++;
			}
			else packetIt = syncPackets.erase(packetIt);
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
	log_colored(ConsoleColor::InfoHighlighted, "Sender thread closed");
}

// Создание потоков
void ConnectedClient::createThreads()
{
	receiver = std::thread(&ConnectedClient::receiverThread, this);
	sender   = std::thread(&ConnectedClient::senderThread,   this);
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

		if      (respSize > 0) {
			// Записываем данные от клиента
			dest = packetFactory.create(respBuff.data(), respSize, false);

			// Добавить пакет
			receivedPackets.push_back(dest);

			break;
		}
		else if (!respSize) {
			// Соединение сброшено
			log_raw_colored(ConsoleColor::InfoHighlighted, "Connection closed");
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
				log_raw_colored(ConsoleColor::WarningHighlighted, "The size of received packet is larger than the buffer size!");
				return -2;
			}
			else if (err == WSAECONNRESET || err == WSAECONNABORTED) {
				// Соединение сброшено
				log_raw_colored(ConsoleColor::InfoHighlighted, "Connection closed");
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
int ConnectedClient::sendData(PacketPtr packet) {
	setState(ClientState::Send);

	if (send(writeSocket, packet->data, packet->size, 0) == SOCKET_ERROR) {
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

#define PRINT_STATE(X) case ClientState:: X: \
	state_desc = #X; \
	break;

	switch (state) {
		PRINT_STATE(FirstHandshake);
		PRINT_STATE(SecondHandshake);
		PRINT_STATE(Send);
		PRINT_STATE(Receive);
		PRINT_STATE(Shutdown);
		PRINT_STATE(CloseSocket);
	default:
		log_colored(ConsoleColor::WarningHighlighted, "Unknown state: %d", (int)state);
		return;
}
#undef PRINT_STATE

	log_colored(ConsoleColor::Info, "State changed to: %s", state_desc);
#endif

	this->state = state;
}



std::ostream& operator<< (std::ostream& os, ConnectedClient& client)
{
	os << "IP: \"" << client.getIP_str() << "\" => {" << endl
	<< "  ID                    : " << client.getID() << endl
	<< "  running               : " << client.isRunning() << endl
	<< "  received packets count: " << client.receivedPackets.size() << endl
	<< "  sended packets count  : " << client.sendedPackets.size() << endl
	<< "  main packets count    : " << client.mainPackets.size() << endl
	<< "  sync packets count    : " << client.syncPackets.size() << endl << endl;

	os << "  received packets: {" << endl;

	for (auto packet : client.receivedPackets) {
		os << "    {" << endl
			<< "    ID     : " << packet->ID << endl
			<< "    size   : " << packet->size << endl
			<< "    needACK: " << packet->needACK << endl
			<< "    data   : ";

		os.write(packet->data, packet->size);

		os << endl
			<< "    }" << endl;
	}

	os << "  }" << endl
	<< "  sended packets  : {" << endl;

	for (auto packet : client.sendedPackets) {
		os << "    {" << endl
			<< "    ID     : " << packet->ID << endl
			<< "    size   : " << packet->size << endl
			<< "    needACK: " << packet->needACK << endl
			<< "    data   : ";

		os.write(packet->data, packet->size);

		os << endl
			<< "    }" << endl;
	}

	os << "  }" << endl
	<< "  sync packets    : {" << endl;

	for (const auto packet : client.syncPackets) {
		os << "    {" << endl
			<< "    ID     : " << packet->ID << endl
			<< "    size   : " << packet->size << endl
			<< "    needACK: " << packet->needACK << endl
			<< "    data   : ";

		os.write(packet->data, packet->size);

		os << endl
			<< "    }" << endl;
	}

	os << "  }" << endl
	<< '}' << endl;
	return os;
}
