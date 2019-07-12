#pragma once

#include "Common.h"

#include <queue>
#include <thread>
#include <memory>
#include <functional>

enum class CLIENT_STATE : uint8_t {
	OK = 0,
	SEND,
	RECEIVE,
	SHUTDOWN,
	CLOSE_SOCKET
};

class ConnectedClient {
public:
	ConnectedClient(uint16_t ID, sockaddr_in clientDesc, int clientLen);
	~ConnectedClient();

	uint16_t ID;
	bool started;

	std::vector<PacketPtr> receivedPackets;
	std::vector<PacketPtr> sendedPackets;

	std::queue<PacketPtr> mainPackets;
	std::vector<PacketPtr> syncPackets;

	void createThreads();

	int receiveData(PacketPtr dest);
	int sendData(PacketPtr packet);

	int disconnect();

	sockaddr_in clientDesc;

	SOCKET readSocket;
	SOCKET writeSocket;
private:
	int handle1(PacketPtr packet);
	int handle2(PacketPtr packet);

	int handlePacketIn(std::function<int(PacketPtr)>handler);
	int handlePacketOut(PacketPtr packet);

	void receiverThread();
	void senderThread();

	void setState(CLIENT_STATE state);

	std::thread receiver;
	std::thread sender;

	int error_code;

	CLIENT_STATE state;

	char IP_str[16];
	char host[NI_MAXHOST];

	unsigned long IP;

	uint16_t port;
};