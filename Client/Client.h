#pragma once

#include "pch.h"

constexpr uint16_t NET_BUFFER_SIZE = 8192;

enum class ERROR_TYPE : uint8_t {
	OK = 0,        // Без ошибок
	WARNING,       // Предупреждение
	SOFT_ERROR,    // Ошибка
	CRITICAL_ERROR // Критическая ошибка
};

enum class CLIENT_STATE : uint8_t {
	OK = 0,
	INIT_WINSOCK,
	CREATE_SOCKET,
	CONNECT,
	SEND,
	SHUTDOWN,
	RECEIVE,
	CLOSE_SOCKET
};

struct Packet {
	Packet(const char* data, size_t size = 0);
	~Packet();
	char* data;
	size_t size;
};

class Client {
public:
	Client(PCSTR, USHORT);
	~Client();

	int error_code;

	std::vector<std::unique_ptr<Packet>> receivedPackets;
	std::vector<std::unique_ptr<Packet>> sendedPackets;

	int connect2server();
	int sendData();
	int receiveData();
	int disconnect();
private:
	void setState(CLIENT_STATE state);

	CLIENT_STATE state;

	PCSTR IP;

	uint16_t port;

	bool client_started;

	WSADATA wsaData;

	SOCKET connectSocket;

	struct sockaddr_in socketDesc;
};