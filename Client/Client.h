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

enum class CLIENT_STATE : uint8_t {
	OK = 0,
	INIT_WINSOCK,
	CREATE_READ_SOCKET,
	CREATE_WRITE_SOCKET,
	SEND,
	SHUTDOWN,
	RECEIVE,
	CLOSE_SOCKET
};

struct Packet {
	Packet(const char* data = nullptr, size_t size = 0, bool needACK = false);
	~Packet();
	char* data;
	size_t size;
	bool needACK;
};

typedef std::shared_ptr<Packet> PacketPtr;

class Client {
public:
	Client(PCSTR IP, uint16_t readPort, uint16_t writePort);
	~Client();

	int error_code;

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

	int handleACK(PacketPtr packet);
	int handleAll(PacketPtr packet);

	int handlePacketIn(std::function<int(PacketPtr)>handler);
	int handlePacketOut(PacketPtr packet);

	void receiverThread();
	void senderThread();

	void setState(CLIENT_STATE state);

	std::thread receiver;
	std::thread sender;

	CLIENT_STATE state;

	PCSTR IP;

	uint16_t readPort;
	uint16_t writePort;

	bool started;

	SOCKET readSocket;
	SOCKET writeSocket;
};