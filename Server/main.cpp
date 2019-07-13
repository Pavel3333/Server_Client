#include "pch.h"
#include "Server.h"

#include <string>
#include <fstream>

constexpr uint16_t READ_PORT  = 27011;
constexpr uint16_t WRITE_PORT = 27010;


int start()
{
	Server server { READ_PORT, WRITE_PORT };

	if (server.startServer())
		return 1;

	log_raw_colored(ConsoleColor::InfoHighlighted, "You can use these commands to manage the server:");
	log_raw_colored(ConsoleColor::Info,            "  \"send\"  -> Send the packet to client");
	log_raw_colored(ConsoleColor::Info,            "  \"save\"  -> Save all data into the file");
	log_raw_colored(ConsoleColor::Danger,          "  \"close\" -> Close the server");

	while (server.isRunning()) { // Прием команд из командной строки
		std::string cmd;

		std::cin >> cmd;
		std::cin.ignore();

		if      (cmd == "close") { // Закрытие сервера
			if (server.closeServer())
				return 2;
		}
		else if (cmd == "send") {
			log_raw_colored(ConsoleColor::Info, "Please type the client IP");

			std::cin >> cmd;
			std::cin.ignore();

			IN_ADDR IP_struct;

			inet_pton(AF_INET, cmd.data(), &IP_struct);

			auto client_it = server.clientPool.find(IP_struct.s_addr);

			if (client_it == end(server.clientPool))
				log_raw_colored(ConsoleColor::WarningHighlighted, "Client not found!"); // Клиент не найден
			else {
				log_raw_colored(ConsoleColor::Info, "Please type the data you want to send");

				std::getline(std::cin, cmd);

				ConnectedClient& client = *(client_it->second);

				client.sendPacket(packetFactory.create(cmd.data(), cmd.size(), false));
			}
		}
		else if (cmd == "save") {
			std::ofstream fil("all_server_data.txt");
			fil.setf(std::ios::boolalpha); // Вывод true/false

			for (auto clientIt : server.clientPool) {
				ConnectedClient& client = *(clientIt.second);

				fil << "IP: \""                     << client.getIP_str() << "\" => {" << endl
					<< "  ID                    : " << client.getID()                << endl
					<< "  running               : " << client.isRunning()            << endl
					<< "  received packets count: " << client.receivedPackets.size() << endl
					<< "  sended packets count  : " << client.sendedPackets.size()   << endl
					<< "  main packets count    : " << client.mainPackets.size()     << endl
					<< "  sync packets count    : " << client.syncPackets.size()     << endl << endl;

				fil	<< "  received packets: {" << endl;

				for (auto packet : client.receivedPackets) {
					fil << "    {" << endl
						<< "    ID     : " << packet->ID << endl
						<< "    size   : " << packet->size << endl
						<< "    needACK: " << packet->needACK << endl
						<< "    data   : ";

					fil.write(packet->data, packet->size);

					fil << endl
						<< "    }" << endl;
				}

				fil << "  }" << endl
					<< "  sended packets  : {" << endl;

				for (auto packet : client.sendedPackets) {
					fil << "    {"                            << endl
					    << "    ID     : " << packet->ID      << endl
						<< "    size   : " << packet->size    << endl
						<< "    needACK: " << packet->needACK << endl
						<< "    data   : ";

					fil.write(packet->data, packet->size);

					fil << endl
						<< "    }" << endl;
				}

				fil << "  }" << endl
					<< "  sync packets    : {" << endl;

				for (auto packet: client.syncPackets) {
					fil << "    {"                            << endl
					    << "    ID     : " << packet->ID      << endl
						<< "    size   : " << packet->size    << endl
						<< "    needACK: " << packet->needACK << endl
						<< "    data   : ";

					fil.write(packet->data, packet->size);

					fil << endl
						<< "    }" << endl;
				}

				fil << "  }" << endl
					<< '}'   << endl;
			}

			fil.close();

			log_colored(ConsoleColor::SuccessHighlighted, "Data successfully saved!");
		}
	}

	return 0;
}

int main()
{
	// Set thread description
	setThreadDesc(L"main");

	if (int err = start())
		log_colored(ConsoleColor::DangerHighlighted, "Server creating failed - error: %d", err);

	int v;
	std::cin >> v; // Чтобы не закрывалось окно

	return 0;
}