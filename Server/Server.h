#pragma once
#include <vector>

constexpr uint16_t NET_BUFFER_SIZE = 8192;


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

struct Packet {
	Packet(char* data, size_t size);
	~Packet();
	char* data;
	size_t size;
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

	uint16_t port;

	int bytesSent;

	std::vector<std::unique_ptr<Packet>> receivedPackets;

	bool startServer();
	bool sendData(std::string_view);
	bool receiveData();
	bool closeServer();
	bool handleRequests();
private:
	char port_str[7];

	WSADATA wsData;
	SOCKET connectSocket;
};
