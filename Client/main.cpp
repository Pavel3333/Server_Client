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

	uint8_t ctr_reconnect = 1;

	while (ctr_reconnect <= 5) {
		if (Client::getInstance().init(cmd, SERVER_IP, READ_PORT, WRITE_PORT))
			return 1;

		Client::getInstance().printCommandsList();

		log_colored(ConsoleColor::InfoHighlighted, "%d connect to the server...", ctr_reconnect);

		while (Client::getInstance().isRunning()) { // ����� ������ �� ��������� ������
			std::cin >> cmd;
			std::cin.ignore();

			if (cmd == "send") { // �������� ������ �������
				log_raw_colored(ConsoleColor::Info, "Please type the data you want to send");

				std::getline(std::cin, cmd);

				Client::getInstance().sendPacket(PacketFactory::create(cmd.data(), cmd.size(), false));
			}
			else if (cmd == "commands") { // ����� ���� ��������� ������
				Client::getInstance().printCommandsList();
			}
			else if (cmd == "close") { // �������� �������
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
		log_raw_colored(ConsoleColor::DangerHighlighted, "Too many connection attempts. Contact the developers");

	return 0;
}

int main()
{
	// ������ ��� ������
	setThreadDesc(L"[main]");

	if (int err = start())
		log_colored(ConsoleColor::DangerHighlighted, "Client creating failed - error: %d", err);

	log_raw_colored(ConsoleColor::InfoHighlighted, "Press any button to end execution of client");

	int v;
	std::cin >> v; // ����� �� ����������� ����

	return 0;
}
