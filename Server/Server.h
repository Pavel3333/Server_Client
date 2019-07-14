#pragma once
#include <map>
#include "Common.h"
#include "ConnectedClient.h"


enum class ServerState {
	InitWinSock,
	CreateReadSocket,
	CreateWriteSocket,
	Bind,
	SetOpts,
	Listen,
	Connect,
	CloseSockets
};


class Server {
public:
	Server(uint16_t readPort, uint16_t writePort);
	~Server();

	std::map<uint32_t, ConnectedClientPtr> clientPool;

	int startServer();
	int closeServer();

	bool isRunning() { return this->started; }

	size_t getActiveClientsCount();
	void   cleanInactiveClients();

private:
	SOCKET initSocket(uint16_t port);

	int initSockets();

	void inactiveClientsCleaner();
	void handleNewClients(bool isReadSocket);
	void setState(ServerState state);

	std::thread cleaner;

	std::thread firstHandshakesHandler;
	std::thread secondHandshakesHandler;

	std::mutex clients_mutex;

	bool started;

	ServerState state;

	uint16_t readPort;
	uint16_t writePort;

	SOCKET listeningReadSocket;
	SOCKET listeningWriteSocket;
};
