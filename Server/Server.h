#pragma once
#include <map>
#include "Common.h"
#include "ConnectedClient.h"


enum class ServerState {
	InitWinSock,
	CreateReadSocket,
	CreateWriteSocket,
	Bind,
	Listen,
	Connect,
	CloseSockets
};


class Server {
public:
	Server(uint16_t readPort, uint16_t writePort);
	~Server();

	std::map<uint32_t, ConnectedClientPtr> clientPool;

	bool isRunning() { return this->started; }

	int startServer();
	int closeServer();

private:
	SOCKET initSocket(uint16_t port);

	int initSockets();

	void handleNewClients(bool isReadSocket);
	void setState(ServerState state);

	std::thread firstHandshakesHandler;
	std::thread secondHandshakesHandler;

	std::mutex handshakes_mutex;

	bool started;

	ServerState state;

	uint16_t readPort;
	uint16_t writePort;

	SOCKET listeningReadSocket;
	SOCKET listeningWriteSocket;
};
