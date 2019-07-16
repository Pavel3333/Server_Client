#pragma once
#include <map>
#include <mutex>
#include <thread>
#include "ConnectedClient.h"
#include "Common.h"

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

enum class CleanerMode {
	OnlyDisconnect,
	AgressiveMode
};

typedef std::map<uint16_t, ConnectedClientPtr> ClientPool;

typedef ClientPool::iterator       ConnectedClientIter;
typedef ClientPool::const_iterator ConnectedClientConstIter;

class Server {
public:
	Server(uint16_t readPort, uint16_t writePort);
	~Server();

	ClientPool clientPool;

	int  startServer();
	void closeServer();

	void startCleaner();
	void printCleanerCommands();
	void printCleanerMode();
	void cleanInactiveClients(bool ext = false);
	void closeCleaner();

	bool isRunning() { return started; }

	void   printCommandsList();
	size_t getActiveClientsCount();

	int processClientsByPair(bool onlyActive, std::function<int(ConnectedClient&)> handler);

	ConnectedClientConstIter findClientIter(bool onlyActive, std::function<bool(ConnectedClient&)> handler);

	ConnectedClientConstIter getClientByID(bool onlyActive, uint32_t ID);
	ConnectedClientConstIter getClientByIP(bool onlyActive, uint32_t IP, int port = -1, bool isReadPort = false);

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
	CleanerMode cleanerMode;

	uint16_t readPort;
	uint16_t writePort;

	SOCKET listeningReadSocket;
	SOCKET listeningWriteSocket;
};
