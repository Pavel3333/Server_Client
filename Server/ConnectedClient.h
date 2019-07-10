#pragma once

#include "Common.h"

#include <queue>
#include <thread>

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

	std::vector<std::unique_ptr<Packet>> receivedPackets;
	std::vector<std::unique_ptr<Packet>> sendedPackets;

	std::queue<std::unique_ptr<Packet>> mainPackets;
	std::vector<std::unique_ptr<Packet>> syncPackets;

	int sendData();
	int receiveData();
	int disconnect();
private:
	int  handlePacket(std::unique_ptr<Packet> packet);
	void handlerThread();

	void setState(CLIENT_STATE state);

	std::thread handler;

	CLIENT_STATE state;

	PCSTR IP;

	uint16_t port;

	bool client_started;

	WSADATA wsaData;

	SOCKET clientSocket;

	struct sockaddr_in socketDesc;
};