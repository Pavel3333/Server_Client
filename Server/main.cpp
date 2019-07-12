#include "pch.h"
#include "Server.h"

#include <string>

constexpr uint16_t DEFAULT_PORT = 27010;


int start()
{
	Server server { DEFAULT_PORT };

	if (server.startServer())
		return 1;

	log_raw("You can use these commands to manage the server:\n"
		    "  \"close\" -> Close the server");

	while (server.started) { // Прием команд из командной строки
		std::string cmd;

		std::cin >> cmd;

		if (cmd == "close") { // Закрытие сервера
			if (server.closeServer())
				return 2;
		}
	}

	return 0;
}

int main()
{
	if (int err = start())
		log("Server creating failed - error: %d", err);

	std::cin.get(); //Чтобы не закрывалось окно

	return 0;
}