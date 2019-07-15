#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include "Common.h"


enum class ERROR_TYPE {
	OK = 0,        // Без ошибок
	WARNING,       // Предупреждение
	SOFT_ERROR,    // Ошибка
	CRITICAL_ERROR // Критическая ошибка
};

enum class ClientState {
	InitWinSock,
	CreateReadSocket,
	CreateWriteSocket,
	SetOpts,
	Send,
	Shutdown,
	Receive,
	CloseSocket
};

class Client {
public:
	Client(PCSTR IP, uint16_t readPort, uint16_t writePort);
	~Client();

	std::vector<PacketPtr> receivedPackets;
	std::vector<PacketPtr> sendedPackets;

	std::queue<PacketPtr> mainPackets;
	std::vector<PacketPtr> syncPackets;

	int init();
	void disconnect();

	bool isRunning() const {
		return started && readSocket != INVALID_SOCKET
			           && writeSocket != INVALID_SOCKET; }

	void sendPacket(PacketPtr packet) { mainPackets.push(packet); }

	void printCommandsList() const;
private:
	SOCKET connect2server(uint16_t port);

	int handshake();

	int ack_handler(PacketPtr packet);
	int any_packet_handler(PacketPtr packet);

	int handlePacketIn(std::function<int(PacketPtr)>handler, bool closeAfterTimeout);
	int handlePacketOut(PacketPtr packet);

	void receiverThread();
	void senderThread();

	void createThreads();

	int receiveData(PacketPtr& dest, bool closeAfterTimeout);
	int sendData(PacketPtr packet);

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