#include "pch.h"

#include "Server.h"

constexpr uint16_t DEFAULT_PORT = 27010;


int start()
{
	auto server = std::make_unique<Server>(DEFAULT_PORT);

	if (server->startServer())    return 1;
	if (server->closeServer())    return 2;

	return 0;
}

int main()
{
	if (int err = start()) cout << "Server creating failed - error: " << err << endl;
	else                   cout << "Server created successfully!" << endl;

	int v;
	std::cin >> v; //Чтобы не закрывалось окно

	return 0;
}