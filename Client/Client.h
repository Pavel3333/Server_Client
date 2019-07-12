#pragma once

#include "pch.h"

#include <queue>
#include <thread>
#include <functional>

constexpr uint16_t NET_BUFFER_SIZE = 8192;

enum class ERROR_TYPE : uint8_t {
	OK = 0,        // Без ошибок
	WARNING,       // Предупреждение
	SOFT_ERROR,    // Ошибка
	CRITICAL_ERROR // Критическая ошибка
};

enum class ClientState : uint8_t {
	InitWinSock,
	CreateReadSocket,
	CreateWriteSocket,
	Send,
	Shutdown,
	Receive,
	CloseSockets
};

struct Packet {
	Packet(const char* data, size_t size, bool needACK);
	~Packet();
	char* data;
	size_t size;
	bool needACK;
};

typedef std::shared_ptr<Packet> PacketPtr;

std::ostream& operator<< (std::ostream& os, const Packet& val);

// Print WSA errors
void __wsa_print_err(const char* file, uint16_t line);

#define wsa_print_err() __wsa_print_err(__FILE__, __LINE__)

class Client {
public:
	Client(PCSTR IP, uint16_t readPort, uint16_t writePort);
	~Client();

	std::vector<PacketPtr> receivedPackets;
	std::vector<PacketPtr> sendedPackets;

	std::queue<PacketPtr> mainPackets;
	std::vector<PacketPtr> syncPackets;

	int init();
	int connect2server(uint16_t port);
	int handshake();
	int sendData(PacketPtr packet);
	int receiveData(PacketPtr dest);
	int disconnect();
private:
	void createThreads();

	int ack_handler(PacketPtr packet);
	int any_packet_handler(PacketPtr packet);

	int handlePacketIn(std::function<int(PacketPtr)>handler);
	int handlePacketOut(PacketPtr packet);

	void receiverThread();
	void senderThread();

	void setState(ClientState state);

	std::thread receiver;
	std::thread sender;

	ClientState state;

	PCSTR IP;

	uint16_t readPort;
	uint16_t writePort;

	bool started;

	SOCKET readSocket;
	SOCKET writeSocket;
};