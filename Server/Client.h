#pragma once

#include "Common.h"

#include <queue>

enum class CLIENT_STATE : uint8_t {
	OK = 0,
	CREATE_SOCKET,
	CONNECT,
	SEND,
	SHUTDOWN,
	RECEIVE,
	CLOSE_SOCKET
};

class Client {
public:
	Client(SOCKET, PCSTR, USHORT);
	~Client();

	std::vector<std::unique_ptr<Packet>> receivedPackets;
	std::vector<std::unique_ptr<Packet>> sendedPackets;

	std::queue<std::unique_ptr<Packet>> packetsToSend;

	int connect2client();
	int sendData();
	int receiveData();
	int disconnect();
private:
	void setState(CLIENT_STATE state);

	int error_code;

	CLIENT_STATE state;

	PCSTR IP;

	uint16_t port;

	WSADATA wsaData;

	SOCKET clientSocket;

	struct sockaddr_in socketDesc;
};