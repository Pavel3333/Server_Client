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

	log_raw_colored(ConsoleColor::InfoHighlighted, "You can use these commands to manage the client:");
	log_raw_colored(ConsoleColor::Info,            "  \"send\"  -> Send the packet to server");
	log_raw_colored(ConsoleColor::Danger,          "  \"close\" -> Close the client");

	while (client.isRunning()) { // ����� ������ �� ��������� ������
		std::string cmd;

		std::cin >> cmd;

		if (cmd == "close") { // �������� �������
			if (client.disconnect())
				return 2;
		}
		else if (cmd == "send") {
			log_raw_colored(ConsoleColor::Info, "Please type the data you want to send");

			std::cin.ignore();
			std::getline(std::cin, cmd);

			client.sendPacket(packetFactory.create(cmd.data(), cmd.size(), false));
		}
	}

	return 0;
}

int main()
{
	// Set thread description
	setThreadDesc(L"main");

	if (int err = start())
		log_colored(ConsoleColor::DangerHighlighted, "Client creating failed - error: %d", err);

	int v;
	std::cin >> v; // ����� �� ����������� ����

	return 0;
}
