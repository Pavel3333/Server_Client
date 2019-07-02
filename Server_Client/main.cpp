#include "stdafx.h"

#include "Client.h"

#define NET_BUFFER_SIZE 512
#define DEFAULT_PORT    27015
#define SERVER_IP       "127.0.0.1"

int main() {
	std::string_view data_to_send = "Send me please";

	char received_data[NET_BUFFER_SIZE];

	Client* client = new Client(SERVER_IP, DEFAULT_PORT);

	if (client->connect2server())                                     goto client_failed;
	if (client->sendData(data_to_send.data(), data_to_send.length())) goto client_failed;
	if (client->receiveData(received_data, NET_BUFFER_SIZE))          goto client_failed;
	if (client->disconnect())                                         goto client_failed;

	return 0;

client_failed:
	delete client;
	client = nullptr;
}