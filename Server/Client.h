#pragma once

#include "Common.h"

#include <queue>

enum class CLIENT_STATE : uint8_t {
	OK = 0,
	SEND,
	RECEIVE,
	SHUTDOWN,
	CLOSE_SOCKET
};

class Client {
public:
	Client(SOCKET, PCSTR, USHORT);
	~Client();

	int error_code;

	std::vector<std::unique_ptr<Packet>> receivedPackets;
	std::vector<std::unique_ptr<Packet>> sendedPackets;

	std::queue<std::unique_ptr<Packet>> packetsToSend;

	int sendData();
	int receiveData();
	int disconnect();
private:
	void setState(CLIENT_STATE state);

	CLIENT_STATE state;

	PCSTR IP;

	uint16_t port;

	bool client_started;

	WSADATA wsaData;

	SOCKET clientSocket;

	struct sockaddr_in socketDesc;
};