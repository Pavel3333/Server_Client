#include "stdafx.h"

#include "Client.h"

int main() {
	std::string_view data_to_send = "Send me please";

	char received_data[NET_BUFFER_SIZE];

	Client* client = new Client();

	if (client->connect2server())                                     goto client_failed;
	if (client->sendData(data_to_send.data(), data_to_send.length())) goto client_failed;
	if (client->receiveData(received_data, NET_BUFFER_SIZE))          goto client_failed;
	if (client->disconnect())                                         goto client_failed;

	return 0;

client_failed:
	delete client;
	client = nullptr;
}