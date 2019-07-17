#include "pch.h"
#include <string>
#include "Client.h"


constexpr uint16_t    READ_PORT  = 27010;
constexpr uint16_t    WRITE_PORT = 27011;
constexpr const char* SERVER_IP  = "95.72.11.66";

int start() {
	std::string cmd;
	bool err = true;

	while (err) {
		log_raw("Please type the login:");

		std::cin >> cmd;
		std::cin.ignore();

		if (cmd.size() > LOGIN_MAX_SIZE)
			log_colored(ConsoleColor::DangerHighlighted, "Login size must be less than %d!", LOGIN_MAX_SIZE);
		else
			err = false;
	}

	Client client { cmd, SERVER_IP, READ_PORT, WRITE_PORT };

	if (client.init())
		return 1;

	client.printCommandsList();

	while (client.isRunning()) { // Прием команд из командной строки
		std::cin >> cmd;
		std::cin.ignore();

		if      (cmd == "send") { // Отправка данных серверу
			log_raw_colored(ConsoleColor::Info, "Please type the data you want to send");

			std::getline(std::cin, cmd);

			client.sendPacket(PacketFactory::create(cmd.data(), cmd.size(), false));
		}
		else if (cmd == "commands") { // Вывод всех доступных команд
			client.printCommandsList();
		}
		else if (cmd == "close") { // Закрытие клиента
			client.disconnect();
		}
	}

	if (!client.isRunning())
		client.disconnect();

	return 0;
}

int main()
{
	// Задать имя потоку
	setThreadDesc(L"[main]");

	if (int err = start())
		log_colored(ConsoleColor::DangerHighlighted, "Client creating failed - error: %d", err);

	int v;
	std::cin >> v; // Чтобы не закрывалось окно

	return 0;
}
