#include "pch.h"
#include <string>
#include "Client.h"


constexpr uint16_t    AUTH_PORT  = 27010;
constexpr uint16_t    DATA_PORT  = 27011;
constexpr const char* SERVER_IP  = "192.168.0.12";

ClientError start()
{
    std::string login, pass;

    while (true) {
        LOG::raw("Please type the login: ");

        std::cin >> login;
        std::cin.ignore();

        if (login.size() >= UINT8_MAX) {
            LOG::raw_colored(CC_WarningHL, "Too long login. Please retry");
            continue;
        }

        LOG::raw("Please type the password: ");

        std::cin >> pass;
        std::cin.ignore();

        if (pass.size() >= UINT8_MAX) {
            LOG::raw_colored(CC_WarningHL, "Too long password. Please retry");
            continue;
        }

        break;
    }

    std::string cmd;

	uint8_t ctr_reconnect = 1;

	while (ctr_reconnect <= 5) {
		LOG::colored(CC_InfoHL, "%d connect to the server...", ctr_reconnect);

		if (Client::getInstance().init(login, pass, SERVER_IP, AUTH_PORT, DATA_PORT)) {
			ctr_reconnect++;
			continue;
		}

		Client::getInstance().printCommandsList();

		while (Client::getInstance().isRunning()) { // Прием команд из командной строки
			std::cin >> cmd;
			std::cin.ignore();

			if (cmd == "send") { // Отправка данных серверу
				LOG::raw_colored(CC_Info, "Please type the data you want to send");

				std::getline(std::cin, cmd);

				Client::getInstance().sendPacket(
                    PacketFactory::create(
                        DT_MSG,
                        cmd.data(),
                        cmd.size(),
                        false
                    )
                );
			}
			else if (cmd == "commands") { // Вывод всех доступных команд
				Client::getInstance().printCommandsList();
			}
			else if (cmd == "close") { // Закрытие клиента
				Client::getInstance().disconnect();
				return CE_OK;
			}
		}

		if (!Client::getInstance().isRunning()) {
			Client::getInstance().disconnect();

			ctr_reconnect++;
		}
	}

	if (ctr_reconnect > 5)
		LOG::raw_colored(CC_DangerHL, "Too many connection attempts. Contact the developers");

	return CE_OK;
}

int main()
{
    ClientError err;

	// Задать имя потоку
	setThreadDesc(L"[main]");

    err = start();
	if (_ERROR(err))
		LOG::colored(CC_DangerHL, "Client creating failed - error: %d", err);

	LOG::raw_colored(CC_InfoHL, "Press any button to end execution of client");

	int v;
	std::cin >> v; // Чтобы не закрывалось окно

	return 0;
}
