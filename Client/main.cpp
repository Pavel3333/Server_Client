#include "pch.h"

#include "Client.h"

constexpr uint16_t    DEFAULT_PORT = 27015;
constexpr const char* SERVER_IP    = "127.0.0.1";


int start() {
	auto client = std::make_unique<Client>(SERVER_IP, DEFAULT_PORT);

	if (client->connect2server()) return 1;
	if (client->sendData())       return 2;
	if (client->receiveData())    return 3;
	if (client->disconnect())     return 4;

	return 0;
}

int main()
{
	if (int err = start()) cout << "Client creating failed - error: " << err << endl;
	else                   cout << "Client created successfully!" << endl;

	int v;
	std::cin >> v; //Чтобы не закрывалось окно

	return 0;
}
