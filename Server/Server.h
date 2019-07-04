#pragma once

#include "stdafx.h"

enum class [[nodiscard]] SERVER_STATE : uint8_t{
	OK = 0,
	INIT_WINSOCK,
	GET_ADDR,
	CREATE_SOCKET,
	BIND,
	LISTEN,
	CONNECT,
	RECEIVE,
	SHUTDOWN_REC,
	SEND,
	SHUTDOWN_SEND,
	CLOSE_SOCKET
};

class Server {
public:
	Server(uint16_t);
	~Server();

	int error_code;

	SERVER_STATE state;

	uint16_t port;

	bool startServer();
	bool closeServer();
	bool handleRequests();
private:
	char port_str[7];

	SOCKET connectSocket;

	WSAData wsaData;

	struct addrinfo  socketDescTemp;
	struct addrinfo* socketDesc;
};