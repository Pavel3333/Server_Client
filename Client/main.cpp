#include "pch.h"
#include <string>
#include "Client.h"


constexpr uint16_t    READ_PORT  = 27010;
constexpr uint16_t    WRITE_PORT = 27011;
constexpr const char* SERVER_IP  = "192.168.0.12";

int start() {
	std::string cmd;
	bool err = true;

	while (err) {
		LOG::raw("Please type the login:");

		std::cin >> cmd;
		std::cin.ignore();

		if (cmd.size() > LOGIN_MAX_SIZE)
			LOG::colored(ConsoleColor::DangerHighlighted, "Login size must be less than %d!", LOGIN_MAX_SIZE);
		else
			err = false;
	}

	uint8_t ctr_reconnect = 1;

	while (ctr_reconnect <= 5) {
		LOG::colored(ConsoleColor::InfoHighlighted, "%d connect to the server...", ctr_reconnect);

		if (Client::getInstance().init(cmd, SERVER_IP, READ_PORT, WRITE_PORT)) {
			ctr_reconnect++;
			continue;
		}

		Client::getInstance().printCommandsList();

		while (Client::getInstance().isRunning()) { // Прием команд из командной строки
			std::cin >> cmd;
			std::cin.ignore();

			if (cmd == "send") { // Отправка данных серверу
				LOG::raw_colored(ConsoleColor::Info, "Please type the data you want to send");

				std::getline(std::cin, cmd);

				Client::getInstance().sendPacket(PacketFactory::create(cmd.data(), cmd.size(), false));
			}
			else if (cmd == "commands") { // Вывод всех доступных команд
				Client::getInstance().printCommandsList();
			}
			else if (cmd == "close") { // Закрытие клиента
				Client::getInstance().disconnect();
				return 0;
			}
		}

		if (!Client::getInstance().isRunning()) {
			Client::getInstance().disconnect();

			ctr_reconnect++;
		}
	}

	if (ctr_reconnect > 5)
		LOG::raw_colored(ConsoleColor::DangerHighlighted, "Too many connection attempts. Contact the developers");

	return 0;
}

int main()
{
	// Задать имя потоку
	setThreadDesc(L"[main]");

	if (int err = start())
		LOG::colored(ConsoleColor::DangerHighlighted, "Client creating failed - error: %d", err);

	LOG::raw_colored(ConsoleColor::InfoHighlighted, "Press any button to end execution of client");

	int v;
	std::cin >> v; // Чтобы не закрывалось окно

	return 0;
}
