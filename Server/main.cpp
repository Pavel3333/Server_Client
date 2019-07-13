#include "pch.h"
#include "Server.h"

#include <string>

constexpr uint16_t READ_PORT  = 27010;
constexpr uint16_t WRITE_PORT = 27011;


int start()
{
	Server server { READ_PORT, WRITE_PORT };

	if (server.startServer())
		return 1;

	log_raw_colored(ConsoleColor::InfoHighlighted, "You can use these commands to manage the server:");
	log_raw_colored(ConsoleColor::Info,            "  \"close\" -> Close the server");
	log_raw_colored(ConsoleColor::Info,            "  \"send\"  -> Send the packet to client");

	while (server.isRunning()) { // Прием команд из командной строки
		std::string cmd;

		std::cin >> cmd;

		if      (cmd == "close") { // Закрытие сервера
			if (server.closeServer())
				return 2;
		}
		else if (cmd == "send") {
			log_raw_colored(ConsoleColor::Info, "Please print the client IP");

			std::cin >> cmd;

			IN_ADDR IP_struct;

			inet_pton(AF_INET, cmd.data(), &IP_struct);

			auto client_it = server.clientPool.find(IP_struct.s_addr);

			if (client_it == end(server.clientPool))
				log_raw_colored(ConsoleColor::WarningHighlighted, "Client not found!");// Клиент не найден
			else {
				log_raw_colored(ConsoleColor::Info, "Please type the word");

				std::cin >> cmd;

				ConnectedClient& client = *(client_it->second);

				client.sendData(packetFactory.create(cmd.data(), cmd.size(), false));
			}
		}
	}

	return 0;
}

int main()
{
	// Set thread description
	setThreadDesc(L"main");

	if (int err = start())
		log_colored(ConsoleColor::DangerHighlighted, "Server creating failed - error: %d", err);

	std::cin.get(); //Чтобы не закрывалось окно

	return 0;
}