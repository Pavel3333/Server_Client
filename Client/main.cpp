#include "pch.h"

#include "Client.h"

constexpr uint16_t    READ_PORT  = 27010;
constexpr uint16_t    WRITE_PORT = 27011;
constexpr const char* SERVER_IP  = "127.0.0.1";


int start() {
	auto client = std::make_unique<Client>(SERVER_IP, READ_PORT, WRITE_PORT);

	if (client->init())       return 1;
	if (client->disconnect()) return 2;

	return 0;
}

int main()
{
	// Set thread description
	setThreadDesc(L"main");

	if (int err = start())
		log("Client creating failed - error: %d", err);

	int v;
	std::cin >> v; //Чтобы не закрывалось окно

	return 0;
}
