#include "pch.h"
#include "Server.h"

constexpr uint16_t DEFAULT_PORT = 27010;


int start()
{
	auto server = make_unique<Server>(DEFAULT_PORT);

	cout << "[server] startServer" << endl;
	if (server->startServer()) {
		return 1;
	}

	cout << "[server] handleRequests" << endl;
	if (server->handleRequests()) {
		return 2;
	}

	cout << "[server] closeServer" << endl;
	if (server->closeServer()) {
		return 3;
	}

	return 0;
}

int main()
{
	int err = start();

	if (!err) {
		cout << "Server created successfully!" << endl;
	}
	else {
		cout << "Server creating failed" << endl;
		return err;
	}

	// (void)std::cin.get();
	return 0;
}