#pragma once

#include "Common.h"

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
	Client(PCSTR, USHORT);
	~Client();

	int error_code;

	CLIENT_STATE state;

	PCSTR IP;

	uint16_t port;

	int bytesSent;

	std::vector<std::unique_ptr<Packet>> receivedPackets;
	std::vector<std::unique_ptr<Packet>> sendedPackets;

	int connect2server();
	int sendData();
	int receiveData();
	int disconnect();
private:
	void setState(CLIENT_STATE state);

	WSADATA wsaData;

	SOCKET connectSocket;

	struct sockaddr_in socketDesc;
};