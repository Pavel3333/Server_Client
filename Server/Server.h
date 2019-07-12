#pragma once
#include <map>
#include "Common.h"
#include "ConnectedClient.h"


enum class ServerState {
	Ok = 0, // bad state, need to remove
	InitWinSock,
	GetAddr, // unused now
	CreateSocket,
	Bind,
	Listen,
	Connect,
	FirstHandshake,
	SecondHandshake,
	Shutdown,
	CloseSocket
};


class Server {
public:
	Server(USHORT port);
	~Server();

	std::map<uint32_t, ConnectedClientPtr> clientPool;

	int startServer();
	int closeServer();

private:
	int first_handshake(ConnectedClient& client, SOCKET socket);
	int second_handshake(ConnectedClient& client, SOCKET socket);

	void handleRequests();
	void setState(ServerState state);

	std::thread handler;

	bool server_started;
	ServerState state;

	uint16_t port;

	SOCKET connectSocket;

	struct addrinfo* socketDesc;
};
