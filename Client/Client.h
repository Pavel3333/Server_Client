#pragma once

#include "stdafx.h"

#define NET_BUFFER_SIZE 8192

enum class [[nodiscard]] ERROR_TYPE : uint8_t{
	OK = 0,        // Без ошибок
	WARNING,       // Предупреждение
	SOFT_ERROR,    // Ошибка
	CRITICAL_ERROR // Критическая ошибка
};

enum class [[nodiscard]] CLIENT_STATE : uint8_t {
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
	Packet(char* data, size_t size);
	~Packet();
	char* data;
	size_t size;
};

class Client {
public:
	Client(const char*, uint16_t);
	~Client();

	int error_code;

	CLIENT_STATE state;

	const char* IP;

	uint16_t port;

	int bytesSent;

	std::vector<std::unique_ptr<Packet>> receivedPackets;

	bool connect2server();
	bool sendData(std::string_view);
	bool receiveData();
	bool disconnect();
private:
	WSADATA wsaData;

	SOCKET connectSocket;

	struct sockaddr_in socketDesc;
};