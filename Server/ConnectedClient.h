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
	ConnectedClient(SOCKET clientSocket, PCSTR IP, USHORT port);
	~ConnectedClient();

	int error_code;

	std::vector<std::shared_ptr<Packet>> receivedPackets;
	std::vector<std::shared_ptr<Packet>> sendedPackets;

	std::queue<std::shared_ptr<Packet>> mainPackets;
	std::vector<std::shared_ptr<Packet>> syncPackets;

	int createThreads();

	int receiveData(std::shared_ptr<Packet> dest);
	int sendData(std::shared_ptr<Packet> packet);

	int disconnect();
private:
	int handlePacketIn(std::function<int(std::shared_ptr<Packet>)>handler);
	int handlePacketOut(std::shared_ptr<Packet> packet);

	void receiverThread();
	void senderThread();

	void setState(CLIENT_STATE state);

	std::thread receiver;
	std::thread sender;

	CLIENT_STATE state;

	PCSTR IP;

	uint16_t port;

	bool client_started;

	WSADATA wsaData;

	SOCKET clientSocket;

	struct sockaddr_in socketDesc;
};