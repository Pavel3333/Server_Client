#include "pch.h"
#include "Server.h"

#include <string>

constexpr uint16_t DEFAULT_PORT = 27010;


int start()
{
	Server server { DEFAULT_PORT };

	if (server.startServer())
		return 1;

	while (server.started) { // Прием команд из командной строки
		std::string cmd;

		std::cin >> cmd;

		if     (cmd == "close") { // Закрытие сервера
			if (server.closeServer())
				return 2;
		}

		Sleep(1000);
	}

	return 0;
}

int main()
{
	if (int err = start())
		cout << "Server creating failed - error: " << err << endl;

	std::cin.get(); //Чтобы не закрывалось окно

	return 0;
}