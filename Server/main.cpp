#include "pch.h"
#include "Server.h"

#include <string>
#include <fstream>

constexpr uint16_t READ_PORT  = 27011;
constexpr uint16_t WRITE_PORT = 27010;

void saveData(ConnectedClient& client) {
	std::ofstream fil("all_server_data.txt", std::ios::app);
	fil.setf(std::ios::boolalpha); // Вывод true/false
	fil << client;
	fil.close();
}

int start()
{
	Server server { READ_PORT, WRITE_PORT };

	if (server.startServer())
		return 1;

	server.printCommandsList();

	while (server.isRunning()) { // Прием команд из командной строки
		std::string cmd;

		std::cin >> cmd;
		std::cin.ignore();

		if      (cmd == "list") { // Получаем данные всех активных клиентов
			server.processClientsByPair(
				true,
				[](ConnectedClient& client) -> int { client.getInfo(); return 0; }
			);
		}
		else if (cmd == "list_detailed") { // Получаем расширенные данные всех активных клиентов
			server.processClientsByPair(
				true,
				[](ConnectedClient& client) -> int { client.getInfo(true); return 0; }
			);
		}
		else if (cmd == "send") { // Послать пакет определенному клиенту
			log_raw_colored(ConsoleColor::Info, "Please type the client ID or IP");

			std::cin >> cmd;
			std::cin.ignore();

			ConnectedClientConstIter client_it = server.getClientByID(true, std::stoi(cmd));
			if (client_it == end(server.clientPool)) {
				IN_ADDR IP_struct;
				inet_pton(AF_INET, cmd.data(), &IP_struct);

				client_it = server.getClientByIP(true, IP_struct.s_addr);
				if (client_it == end(server.clientPool)) {
					log_raw_colored(ConsoleColor::WarningHighlighted, "Client not found!"); // Клиент не найден
					continue;
				}
			}

			ConnectedClient& client = *(client_it->second);

			log_raw_colored(ConsoleColor::Info, "Please type the data you want to send");

			std::getline(std::cin, cmd);

			client.sendPacket(PacketFactory::create(cmd.data(), cmd.size(), false));

			log_raw_colored(ConsoleColor::SuccessHighlighted, "Data was successfully sended!");
		}
		else if (cmd == "send_all") { // Посылаем пакеты активным клиентам
			log_raw_colored(ConsoleColor::Info, "Please type the data you want to send");

			std::getline(std::cin, cmd);

			server.processClientsByPair(
				true,
				[cmd](ConnectedClient& client) -> int 
				{ client.sendPacket(PacketFactory::create(cmd.data(), cmd.size(), false)); return 0; }
			);

			log_raw_colored(ConsoleColor::SuccessHighlighted, "Data was successfully sended!");
		}
		else if (cmd == "save") { // Сохранение данных всех клиентов
			server.processClientsByPair(
				false,
				[](ConnectedClient& client) -> int { saveData(client); return 0; }
			);

			log_colored(ConsoleColor::SuccessHighlighted, "Data was successfully saved!");
		}
		else if (cmd == "enable_cleaner") { // Включение клинера
			server.startCleaner();
		}
		else if (cmd == "disable_cleaner") { // Выключение клинера
			server.closeCleaner();
		}
		else if (cmd == "clean") { // Очистка неактивных клиентов
			server.cleanInactiveClients(true);
		}
		else if (cmd == "commands") { // Вывод всех доступных команд
			server.printCommandsList();
		}
		else if (cmd == "close") { // Закрытие сервера
			server.closeServer();
		}
	}

	if (!server.isRunning())
		server.closeServer();

	return 0;
}

int main()
{
	// Задать имя потоку
	setThreadDesc(L"[main]");

	if (int err = start())
		log_colored(ConsoleColor::DangerHighlighted, "Server creating failed - error: %d", err);

	int v;
	std::cin >> v; // Чтобы не закрывалось окно

	return 0;
}