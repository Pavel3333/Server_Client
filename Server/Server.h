#pragma once

#include "Common.h"

#include "ConnectedClient.h"

typedef std::shared_ptr<ConnectedClient> ConnectedClientPtr;

enum class SERVER_STATE {
	OK = 0,
	INIT_WINSOCK,
	GET_ADDR,
	CREATE_SOCKET,
	BIND,
	LISTEN,
	CONNECT,
	HANDSHAKE_1,
	HANDSHAKE_2,
	SHUTDOWN,
	CLOSE_SOCKET
};

class Server {
public:
	Server(USHORT port);
	~Server();

	std::vector<ConnectedClientPtr> clientPool;

	int startServer();
	int closeServer();
private:
	int handshake_1(ConnectedClientPtr client, SOCKET socket);
	int handshake_2(ConnectedClientPtr client, SOCKET socket);

	void handleRequests();
	void setState(SERVER_STATE state);

	std::thread handler;

	int error_code;

	SERVER_STATE state;

	uint16_t port;

	bool server_started;

	char port_str[7];

	SOCKET connectSocket;

	struct addrinfo* socketDesc;
};
