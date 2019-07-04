#include "stdafx.h"

#include "Server.h"

#define NET_BUFFER_SIZE 512
#define DEFAULT_PORT    27015

int main() {
	Server* server = new Server(DEFAULT_PORT);

	if (server->startServer())                                        goto failed;
	if (server->handleRequests())                                     goto failed;
	if (server->closeServer())                                        goto failed;

failed:
	delete server;
	server = nullptr;

	return 0;
}