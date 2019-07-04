#include "stdafx.h"
#include <memory>
#include <array>
#include "Client.h"

#define NET_BUFFER_SIZE 512
#define DEFAULT_PORT    27015
#define SERVER_IP       "127.0.0.1"

bool start() {
	std::string_view                  data_to_send = "Send me please";
	std::array<char, NET_BUFFER_SIZE> received_data;

	auto client = std::make_unique<Client>(SERVER_IP, DEFAULT_PORT);

	if (client->connect2server())                                        return true;
	if (client->sendData(data_to_send.data(), data_to_send.size()))      return true;
	if (client->receiveData(received_data.data(), received_data.size())) return true;
	if (client->disconnect())                                            return true;

	return false;
}

int main()
{
	if (!start()) printf("Client created successfully!\n");
	else          printf("Client creating failed\n");

	std::getchar(); // ����� �� ����������� ����

	return 0;
}