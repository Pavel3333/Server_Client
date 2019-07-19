#include "pch.h"
#include "Server.h"
#include "Cleaner.h"

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
	if (Server::getInstance().startServer(READ_PORT, WRITE_PORT) > 0) // Произошла ошибка
		return 1;

	Server::getInstance().printCommandsList();
	Cleaner::getInstance().printCommandsList();

	while (Server::getInstance().isRunning()) { // Прием команд из командной строки
		std::string cmd;

		std::cin >> cmd;
		std::cin.ignore();

		if      (cmd == "list") { // Получаем данные всех активных клиентов
			Server::getInstance().processClientsByPair(
				true,
				[](ConnectedClient& client) -> int { client.getInfo(); return 0; }
			);
		}
		else if (cmd == "list_detailed") { // Получаем расширенные данные всех активных клиентов
			Server::getInstance().processClientsByPair(
				true,
				[](ConnectedClient& client) -> int { client.getInfo(true); return 0; }
			);
		}
		else if (cmd == "send") { // Послать пакет определенному клиенту
			log_raw_colored(ConsoleColor::Info, "Please type the client ID or IP");

			std::cin >> cmd;
			std::cin.ignore();

			Server::getInstance().clients_mutex.lock();

			ConnectedClientPtr found_client = Server::getInstance().getClientByID(false, true, std::stoi(cmd));
			if (!found_client) {
				IN_ADDR IP_struct;
				inet_pton(AF_INET, cmd.data(), &IP_struct);

				found_client = Server::getInstance().getClientByIP(false, true, IP_struct.s_addr);
				if (!found_client) {
					log_raw_colored(ConsoleColor::WarningHighlighted, "Client not found!"); // Клиент не найден
					continue;
				}
			}

			log_raw_colored(ConsoleColor::Info, "Please type the data you want to send");

			std::getline(std::cin, cmd);

			found_client->sendPacket(PacketFactory::create(cmd.data(), cmd.size(), false));

			Server::getInstance().clients_mutex.unlock();

			log_raw_colored(ConsoleColor::SuccessHighlighted, "Data was successfully sended!");
		}
		else if (cmd == "send_all") { // Посылаем пакеты активным клиентам
			log_raw_colored(ConsoleColor::Info, "Please type the data you want to send");

			std::getline(std::cin, cmd);

			Server::getInstance().processClientsByPair(
				true,
				[cmd](ConnectedClient& client) -> int 
				{ client.sendPacket(PacketFactory::create(cmd.data(), cmd.size(), false)); return 0; }
			);

			log_raw_colored(ConsoleColor::SuccessHighlighted, "Data was successfully sended!");
		}
		else if (cmd == "save") { // Сохранение данных всех клиентов
			Server::getInstance().processClientsByPair(
				false,
				[](ConnectedClient& client) -> int { saveData(client); return 0; }
			);

			log_colored(ConsoleColor::SuccessHighlighted, "Data was successfully saved!");
		}
		else if (cmd == "clean") { // Очистка неактивных клиентов
			Cleaner::getInstance().cleanInactiveClients(true);
		}
		else if (cmd == "commands") { // Вывод всех доступных команд
			Server::getInstance().printCommandsList();
			Cleaner::getInstance().printCommandsList();
		}
		else if (cmd == "close") { // Закрытие сервера
			Server::getInstance().closeServer();
		}
		else if (cmd == "enable_cleaner") { // Включение клинера
			Cleaner::getInstance().startCleaner();
		}
		else if (cmd == "get_cleaner_mode") { // Вывести режим работы клинера
			Cleaner::getInstance().printMode();
		}
		else if (cmd == "change_cleaner_mode") { // Сменить режим работы клинера
			Cleaner::getInstance().changeMode();
		}
		else if (cmd == "disable_cleaner") { // Выключение клинера
			Cleaner::getInstance().closeCleaner();
		}
	}

	if (!Server::getInstance().isRunning())
		Server::getInstance().closeServer();

	return 0;
}

int main()
{
	// Задать имя потоку
	setThreadDesc(L"[main]");

	if (int err = start())
		log_colored(ConsoleColor::DangerHighlighted, "Server creating failed - error: %d", err);

	log_raw_colored(ConsoleColor::InfoHighlighted, "Press any button to end execution of server");

	int v;
	std::cin >> v; // Чтобы не закрывалось окно

	return 0;
}