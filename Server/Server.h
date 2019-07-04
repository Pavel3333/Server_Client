#pragma once

#include "stdafx.h"

#define NET_BUFFER_SIZE 512

enum class [[nodiscard]] SERVER_STATE : uint8_t{
	OK = 0,
	INIT_WINSOCK,
	GET_ADDR,
	CREATE_SOCKET,
	BIND,
	LISTEN,
	CONNECT,
	RECEIVE,
	SEND,
	SHUTDOWN,
	CLOSE_SOCKET
};

struct Packet {
	char* data;
	uint16_t size;
};

class Server {
public:
	Server(uint16_t);
	~Server();

	int error_code;

	SERVER_STATE state;

	uint16_t port;

	int bytesSent;

	std::vector<Packet*> receivedPackets;

	bool startServer();
	bool sendData(const char*, int);
	bool receiveData();
	bool closeServer();
	bool handleRequests();
private:
	char port_str[7];

	SOCKET connectSocket;

	WSAData wsaData;

	struct addrinfo  socketDescTemp;
	struct addrinfo* socketDesc;
};