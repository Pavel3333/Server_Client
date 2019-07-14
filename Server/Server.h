#pragma once
#include <map>
#include "include/Common.h"
#include "ConnectedClient.h"


enum class ServerState {
	InitWinSock,
	CreateReadSocket,
	CreateWriteSocket,
	Bind,
	SetOpts,
	Listen,
	Waiting,
	Connect,
	CloseSockets
};


class Server {
public:
	Server(uint16_t readPort, uint16_t writePort);
	~Server();

	std::map<uint32_t, ConnectedClientPtr> clientPool;

	int  startServer();
	void closeServer();

	void startCleaner();
	void closeCleaner();

	bool isRunning() { return started; }

	void   printCommandsList();
	size_t getActiveClientsCount();
	void   cleanInactiveClients();
	int    processClients(bool onlyActive, std::function<int(ConnectedClient&)> handler);

	std::mutex clients_mutex;

private:
	SOCKET initSocket(uint16_t port);

	int initSockets();

	void inactiveClientsCleaner();

	void processIncomeConnection(bool isReadSocket);

	void setState(ServerState state);

	std::thread cleaner;

	std::thread firstHandshakesHandler;
	std::thread secondHandshakesHandler;

	bool started;
	bool cleanerStarted;

	ServerState state;

	uint16_t readPort;
	uint16_t writePort;

	SOCKET listeningReadSocket;
	SOCKET listeningWriteSocket;
};
