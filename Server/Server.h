#pragma once

#include "Common.h"

#include "ConnectedClient.h"

enum class SERVER_STATE {
	OK = 0,
	INIT_WINSOCK,
	GET_ADDR,
	CREATE_SOCKET,
	BIND,
	LISTEN,
	CONNECT,
	SHUTDOWN,
	CLOSE_SOCKET
};

class Server {
public:
	Server(USHORT port);
	~Server();

	int error_code;

	std::vector<std::shared_ptr<ConnectedClient>> clientPool;

	int startServer();
	int closeServer();
	int handleRequests();
private:
	void setState(SERVER_STATE state);

	SERVER_STATE state;

	uint16_t port;

	bool server_started;

	char port_str[7];

	SOCKET connectSocket;

	struct addrinfo* socketDesc;
};
