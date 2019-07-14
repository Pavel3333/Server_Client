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
	if(started)
		disconnect();

	receivedPackets.clear();
	sendedPackets.clear();
	syncPackets.clear();

	while (!mainPackets.empty())
		mainPackets.pop();
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

	// Shutdown the connection since no more data will be sent
	setState(ClientState::Shutdown);

	if (readSocket != INVALID_SOCKET)
		if (shutdown(readSocket, SD_BOTH) == SOCKET_ERROR)
			log_colored(ConsoleColor::DangerHighlighted, "Error while shutdowning read socket: %d", WSAGetLastError());

	if (writeSocket != INVALID_SOCKET)
		if (shutdown(writeSocket, SD_BOTH) == SOCKET_ERROR)
			log_colored(ConsoleColor::DangerHighlighted, "Error while shutdowning write socket: %d", WSAGetLastError());

	// Close the socket
	setState(ClientState::CloseSockets);

	if (readSocket != INVALID_SOCKET)
		if (closesocket(readSocket) == SOCKET_ERROR)
			log_colored(ConsoleColor::DangerHighlighted, "Error while closing read socket: %d", WSAGetLastError());

	if (writeSocket != INVALID_SOCKET)
		if (closesocket(writeSocket) == SOCKET_ERROR)
			log_colored(ConsoleColor::DangerHighlighted, "Error while closing write socket: %d", WSAGetLastError());

	log_colored(ConsoleColor::InfoHighlighted, "Connected client %d was stopped", ID);

	return;
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
	log_raw(std::string_view(packet->data, packet->size));
	return 0;
}


// Обработка входящего пакета
int ConnectedClient::handlePacketIn(std::function<int(PacketPtr)> handler, bool closeAfterTimeout) {
	PacketPtr packet;

	int err = receiveData(packet, closeAfterTimeout);
	if (err)
		return err; // Произошла ошибка

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

	// Ожидание любых входящих пакетов
	// Таймаут не нужен
	while (started) {
		int err = handlePacketIn(
			std::bind(&ConnectedClient::any_packet_handler, this, std::placeholders::_1), 
			false
		);

		if (err) {
			if (err > 0) break;    // Критическая ошибка или соединение сброшено
			else         continue; // Неудачный пакет, продолжить прием        
		}
	}

	// Закрываем поток
	log_colored(ConsoleColor::InfoHighlighted, "Closing receiver thread");

	disconnect();
}

// Поток отправки пакетов
void ConnectedClient::senderThread()
{
	// Задать имя потоку
	setThreadDesc(L"Sender");

	while (started) {
		// Обработать основные пакеты
		while (!mainPackets.empty()) {
			PacketPtr packet = mainPackets.back();

			if (handlePacketOut(packet)) {
				log_colored(ConsoleColor::Warning, "Packet %d not confirmed, adding to sync queue", packet->ID);
				syncPackets.push_back(packet);
			}
			
			mainPackets.pop();
		}

		// Обработать пакеты, не подтвержденные клиентами
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

	// Закрываем поток
	log_colored(ConsoleColor::InfoHighlighted, "Closing sender thread");
}

// Создание потоков
void ConnectedClient::createThreads()
{
	receiver = std::thread(&ConnectedClient::receiverThread, this);
	receiver.detach();

	sender = std::thread(&ConnectedClient::senderThread, this);
	sender.detach();
}


// Принятие данных
int ConnectedClient::receiveData(PacketPtr& dest, bool closeAfterTimeout)
{
	setState(ClientState::Receive);

	std::array<char, NET_BUFFER_SIZE> respBuff;

	while (started) {
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
			else if (err == WSAECONNRESET) {
				// Соединение сброшено
				log_raw_colored(ConsoleColor::InfoHighlighted, "Connection closed");
				return -3;
			}
			else {
				// Критическая ошибка
				wsa_print_err();
				return 2;
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
		PRINT_STATE(CloseSockets);
	default:
		log_colored(ConsoleColor::WarningHighlighted, "Unknown state: %d", (int)state);
		return;
}
#undef PRINT_STATE

	log_colored(ConsoleColor::Info, "State changed to: %s", state_desc);
#endif

	this->state = state;
}