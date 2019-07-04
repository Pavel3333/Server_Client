#include "stdafx.h"
#include <memory>
#include <array>
#include "Client.h"

#define NET_BUFFER_SIZE 512
#define DEFAULT_PORT    27015
#define SERVER_IP       "127.0.0.1"

int main()
{
	std::string_view data_to_send = "Send me please";

	std::array<char, NET_BUFFER_SIZE> received_data;

	auto server = std::make_unique<Server>(DEFAULT_PORT);
	auto client = std::make_unique<Client>(SERVER_IP, DEFAULT_PORT);

	if (server->startServer()) {
		// fail log here
		goto failed;
	}

	if (server->handleRequests()) {
		// fail log here
		goto failed;
	}

	if (client->connect2server()) {
		// fail log here
		goto failed;
	}

	if (client->sendData(data_to_send.data(), data_to_send.length())) {
		// fail log here
		goto failed;
	}

	if (client->receiveData(received_data.data(), received_data.size())) {
		// fail log here
		goto failed;
	}

	if (client->disconnect()) {
		// fail log here
		goto failed;
	}

	if (server->closeServer()) {
		// fail log here
		goto failed;
	}

	// success log here

	std::getchar(); // Чтобы не закрывалось окно
failed:
	return 0;
}
