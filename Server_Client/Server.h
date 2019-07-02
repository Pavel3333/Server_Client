#pragma once

#include "stdafx.h"

enum[[nodiscard]] SERVER_STATE : uint8_t{
	SUCCESS = 0,
	INIT_WINSOCK,
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
	SOCKET connectSocket;

	WSAData wsaData;

	struct sockaddr_in socketDesc;
};