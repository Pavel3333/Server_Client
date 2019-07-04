#include "stdafx.h"

#include "Server.h"

#define NET_BUFFER_SIZE 512
#define DEFAULT_PORT    27015

bool start() {
	auto server = std::make_unique<Server>(DEFAULT_PORT);

	if (server->startServer())    return true;
	if (server->handleRequests()) return true;
	if (server->closeServer())    return true;

	return false;
}

int main() {
	if (!start()) printf("Server created successfully!\n");
	else          printf("Server creating failed\n");

	return 0;
}