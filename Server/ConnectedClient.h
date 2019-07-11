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
	ConnectedClient(uint16_t ID, PCSTR IP, USHORT port);
	~ConnectedClient();

	int error_code;

	std::vector<PacketPtr> receivedPackets;
	std::vector<PacketPtr> sendedPackets;

	std::queue<PacketPtr> mainPackets;
	std::vector<PacketPtr> syncPackets;

	int handshake();

	int receiveData(PacketPtr dest);
	int sendData(PacketPtr packet);

	int disconnect();
private:
	int handlePacketIn(std::function<int(PacketPtr)>handler);
	int handlePacketOut(PacketPtr packet);

	void receiverThread();
	void senderThread();

	void setState(CLIENT_STATE state);

	std::thread receiver;
	std::thread sender;

	CLIENT_STATE state;

	PCSTR IP;

	uint16_t port;
	uint16_t ID;

	bool client_started;

	WSADATA wsaData;

	SOCKET readSocket;
	SOCKET writeSocket;

	struct sockaddr_in socketDesc;
};