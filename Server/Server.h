#pragma once
#include <vector>

constexpr uint16_t NET_BUFFER_SIZE = 512;


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
	int sendData(std::string_view);
	int receiveData(SOCKET clientSocket);
	int closeServer();
	int handleRequests();

private:
	void setState(SERVER_STATE state);

	std::vector<std::string_view> receivedPackets;
	SERVER_STATE state;
	USHORT port;

	WSADATA wsData;
	SOCKET connectSocket;
};
