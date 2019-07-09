#pragma once

#include "Common.h"

#include "Client.h"

enum class SERVER_STATE {
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

class Server {
public:
	Server(USHORT port);
	~Server();

	std::vector<std::unique_ptr<Client>> clients;

	int startServer();
	int closeServer();
	int handleRequests();
private:
	void setState(SERVER_STATE state);

	SERVER_STATE state;

	uint16_t port;

	int error_code;

	char port_str[7];

	WSADATA wsData;

	SOCKET connectSocket;

	struct addrinfo* socketDesc;
};
