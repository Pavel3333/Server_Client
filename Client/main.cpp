#include "pch.h"

#include "Client.h"

#include <string>

constexpr uint16_t    READ_PORT  = 27010;
constexpr uint16_t    WRITE_PORT = 27011;
constexpr const char* SERVER_IP  = "127.0.0.1";


int start() {
	Client client { SERVER_IP, READ_PORT, WRITE_PORT };

	if (client.init())
		return 1;

	log_raw_colored(ConsoleColor::InfoHighlighted, "You can use these commands to manage the client:");
	log_raw_colored(ConsoleColor::Info, "  \"close\" -> Close the client");

	while (client.isRunning()) { // ����� ������ �� ��������� ������
		std::string cmd;

		std::cin >> cmd;

		if (cmd == "close") { // �������� �������
			if (client.disconnect())
				return 2;
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
	std::cin >> v; //����� �� ����������� ����

	return 0;
}
