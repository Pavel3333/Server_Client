#include "pch.h"
#include "Server.h"

constexpr uint16_t DEFAULT_PORT = 27010;


int start()
{
	Server server { DEFAULT_PORT };

	if (server.startServer())
		return 1;

	if (server.closeServer())
		return 2;

	return 0;
}

int main()
{
	int err = start();
	if (err)
		cout << "Server creating failed - error: " << err << endl;
	else
		cout << "Server created successfully!" << endl;

	std::cin.get(); //Чтобы не закрывалось окно

	return 0;
}