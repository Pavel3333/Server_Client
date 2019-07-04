#include "stdafx.h"

#include "Client.h"

#define NET_BUFFER_SIZE 512
#define DEFAULT_PORT    27015
#define SERVER_IP       "127.0.0.1"

int main() {
	std::string_view data_to_send = "Send me please";

	char received_data[NET_BUFFER_SIZE];

	Client* client = new Client(SERVER_IP, DEFAULT_PORT);

	if (client->connect2server())                                     goto failed;
	if (client->sendData(data_to_send.data(), data_to_send.length())) goto failed;
	if (client->receiveData(received_data, NET_BUFFER_SIZE))          goto failed;
	if (client->disconnect())                                         goto failed;

	std::getchar(); // Чтобы не закрывалось окно

failed:
	delete client;
	client = nullptr;

	return 0;
}