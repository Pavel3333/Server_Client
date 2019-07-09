#pragma once

#include "Common.h"

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

	int startServer();
	int sendData();
	int receiveData(SOCKET clientSocket);
	int closeServer();
	int handleRequests();
private:
	void setState(SERVER_STATE state);

	SERVER_STATE state;

	uint16_t port;

	int bytesSent;
	int error_code;

	std::vector<std::unique_ptr<Packet>> receivedPackets;
	std::vector<std::unique_ptr<Packet>> sendedPackets;

	char port_str[7];

	WSADATA wsData;

	SOCKET connectSocket;
	SOCKET clientSocket;

	struct addrinfo* socketDesc;
};
