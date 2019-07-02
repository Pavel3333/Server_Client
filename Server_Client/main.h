#pragma once

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>

#include <iostream>

#define NET_BUFFER_SIZE 512
#define DEFAULT_PORT    27015
#define SERVER_IP       "127.0.0.1"

enum [[nodiscard]] ERROR_TYPE : uint8_t {
	OK = 0,        // Без ошибок
	WARNING,       // Предупреждение
	SOFT_ERROR,    // Ошибка
	CRITICAL_ERROR // Критическая ошибка
};

enum [[nodiscard]] CLIENT_STATE : uint8_t {
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
	Client();
	~Client();

	int error_code;

	CLIENT_STATE state;

	int bytesSent;
	int bytesRec;

	bool connect2Server();
	bool sendData(const char*, int);
	bool receiveData(char*, int);
	bool disconnect();
private:
	WSADATA wsaData;

	SOCKET ConnectSocket;

	struct sockaddr_in clientService;
};