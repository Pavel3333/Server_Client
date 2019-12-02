#pragma once
#include <map>
#include <mutex>
#include <thread>
#include <string>
#include "Common.h"
#include "ConnectedClient.h"

enum class ServerState {
	InitWinSock,
	CreateReadSocket,
	CreateWriteSocket,
	Bind,
	Connect,
	CloseSockets
};

typedef std::map<uint16_t, ConnectedClientPtr> ClientPool;

// Синглтон сервера
class Server {
public:
	static Server& getInstance() {
		static Server instance;
		return instance;
	}

	int  startServer(uint16_t readPort, uint16_t writePort);
	void closeServer();

	// Getters
	bool isRunning() const { return started; }

	size_t getActiveClientsCount();

	ConnectedClientPtr findClient(bool lockMutex, bool onlyActive, std::function<bool(ConnectedClientPtr)> handler);

	ConnectedClientPtr getClientByID(bool lockMutex, bool onlyActive, uint32_t ID);
	ConnectedClientPtr getClientByIP(bool lockMutex, bool onlyActive, uint32_t IP, int port = -1, bool isReadPort = false);
	ConnectedClientPtr getClientByLogin(bool lockMutex, bool onlyActive, uint32_t loginHash, int16_t clientID = -1);

	// Other methods
	void printCommandsList() const;
	void printClientsList(bool ext = false);

	void send(ConnectedClientPtr client = nullptr, PacketPtr packet = nullptr);
	void sendAll(PacketPtr packet = nullptr);

	void save();

    size_t processClientsByPair(bool onlyActive, std::function<int(ConnectedClient&)> handler);

	// Instances

	ClientPool clientPool;
	std::mutex clients_mutex;
private:
	SOCKET initSocket(uint16_t port);

	int initSockets();

	void processIncomeConnection(bool isReadSocket);

	void setState(ServerState state);

	std::thread firstHandshakesHandler;
	std::thread secondHandshakesHandler;

	bool started = false;

	ServerState state;

	uint16_t readPort;
	uint16_t writePort;

	SOCKET listeningReadSocket;
	SOCKET listeningWriteSocket;

	// Защита от копирования
	Server()                   {}
	Server(const Server&)      {}
	Server& operator=(Server&) {}
};