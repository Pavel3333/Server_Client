#include "pch.h"
#include "Server.h"
#include "Cleaner.h"

constexpr uint16_t AUTH_PORT = 27010;
constexpr uint16_t DATA_PORT = 27011;

ServerError start()
{
    int err;

    err = Server::getInstance().startServer(AUTH_PORT);
	if (_ERROR(err))
		return SE_START_SERVER;

	Server::getInstance().printCommandsList();
	Cleaner::getInstance().printCommandsList();

	while (Server::getInstance().isRunning()) { // Прием команд из командной строки
		std::string cmd;

		std::cin >> cmd;
		std::cin.ignore();

		if      (cmd == "list") { // Получаем данные всех активных клиентов
			Server::getInstance().printClientsList(false);
		}
		else if (cmd == "list_detailed") { // Получаем расширенные данные всех активных клиентов
			Server::getInstance().printClientsList(true);
		}
		else if (cmd == "send") { // Послать пакет определенному клиенту
			Server::getInstance().send();
		}
		else if (cmd == "send_all") { // Посылаем пакеты активным клиентам
			Server::getInstance().sendAll();
		}
		else if (cmd == "save") { // Сохранение данных всех клиентов
			Server::getInstance().save();
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

	return SE_OK;
}

int main()
{
    ServerError err = SE_OK;

	// Задать имя потоку
	setThreadDesc(L"[main]");

    err = start();
	if (_ERROR(err))
		LOG::colored(CC_DangerHL, "Server creating failed - error: %d", err);

	LOG::raw_colored(CC_InfoHL, "Press any button to end execution of server");

	int v;
	std::cin >> v; // Чтобы не закрывалось окно

	return 0;
}