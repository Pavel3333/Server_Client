#pragma once

#include "stdafx.h"

enum[[nodiscard]] ERROR_TYPE : uint8_t{
	OK = 0,        // Без ошибок
	WARNING,       // Предупреждение
	SOFT_ERROR,    // Ошибка
	CRITICAL_ERROR // Критическая ошибка
};

enum[[nodiscard]] CLIENT_STATE : uint8_t {
	SUCCESS = 0,
	INIT_WINSOCK,
	CREATE_SOCKET,
	CONNECT,
	SEND,
	SHUTDOWN,
	RECEIVE,
	CLOSE_SOCKET
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
	int bytesRec;

	bool connect2server();
	bool sendData(const char*, int);
	bool receiveData(char*, int);
	bool disconnect();
private:
	WSADATA wsaData;

	SOCKET connectSocket;

	struct sockaddr_in socketDesc;
};