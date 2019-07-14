#include "pch.h"

#include "Client.h"

#include <string>

constexpr uint16_t    READ_PORT  = 27010;
constexpr uint16_t    WRITE_PORT = 27011;
constexpr const char* SERVER_IP  = "95.72.11.66";

int start() {
	Client client { SERVER_IP, READ_PORT, WRITE_PORT };

	if (client.init())
		return 1;

	client.printCommandsList();

	while (client.isRunning()) { // ����� ������ �� ��������� ������
		std::string cmd;

		std::cin >> cmd;
		std::cin.ignore();

		if      (cmd == "send") { // �������� ������ �������
			log_raw_colored(ConsoleColor::Info, "Please type the data you want to send");

			std::getline(std::cin, cmd);

			client.sendPacket(packetFactory.create(cmd.data(), cmd.size(), false));
		}
		else if (cmd == "commands") { // ����� ���� ��������� ������
			client.printCommandsList();
		}
		else if (cmd == "close") { // �������� �������
			client.disconnect();
		}
	}

	if (!client.isRunning())
		client.disconnect();

	return 0;
}

int main()
{
	// ������ ��� ������
	setThreadDesc(L"main");

	if (int err = start())
		log_colored(ConsoleColor::DangerHighlighted, "Client creating failed - error: %d", err);

	int v;
	std::cin >> v; // ����� �� ����������� ����

	return 0;
}
