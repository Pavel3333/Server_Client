#pragma once

#include "stdafx.h"

#define NET_BUFFER_SIZE 8192

enum class [[nodiscard]] SERVER_STATE : uint8_t{
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
	Server(uint16_t);
	~Server();

	int error_code;

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

	SOCKET connectSocket;
	SOCKET clientSocket;

	WSAData wsaData;

	struct addrinfo  socketDescTemp;
	struct addrinfo* socketDesc;
};