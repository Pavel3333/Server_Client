#include "pch.h"
#include "Server.h"

#include <string>
#include <fstream>

constexpr uint16_t READ_PORT  = 27011;
constexpr uint16_t WRITE_PORT = 27010;

void saveData(ConnectedClient& client) {
	std::ofstream fil("all_server_data.txt", std::ios::app);
	fil.setf(std::ios::boolalpha); // Вывод true/false

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

	fil.close();
}

void printCommandsList() {
	log_raw_colored(ConsoleColor::InfoHighlighted, "You can use these commands to manage the server:");
	log_raw_colored(ConsoleColor::Info,            "  \"list\"          => Print list of all active clients");
	log_raw_colored(ConsoleColor::Info,            "  \"list_detailed\" => Print list of all active clients with extra info");
	log_raw_colored(ConsoleColor::Info,            "  \"send\"          => Send the packet to client");
	log_raw_colored(ConsoleColor::Info,            "  \"send_all\"      => Send the packet to all clients");
	log_raw_colored(ConsoleColor::Info,            "  \"save\"          => Save all data into the file");
	log_raw_colored(ConsoleColor::Info,            "  \"clean\"         => Clean all inactive users");
	log_raw_colored(ConsoleColor::Danger,          "  \"close\"         => Close the server");
}

int start()
{
	Server server { READ_PORT, WRITE_PORT };

	if (server.startServer())
		return 1;

	printCommandsList();

	while (server.isRunning()) { // Прием команд из командной строки
		std::string cmd;

		std::cin >> cmd;
		std::cin.ignore();

		if      (cmd == "list") {
			server.processClients(
				true, // Получаем данные всех активных клиентов
				[](ConnectedClient& client) -> int { client.getInfo(); return 0; }
			);
		}
		else if (cmd == "list_detailed") {
			server.processClients(
				true, // Получаем данные всех активных клиентов
				[](ConnectedClient& client) -> int { client.getInfo(true); return 0; }
			);
		}
		else if (cmd == "send") {
			log_raw_colored(ConsoleColor::Info, "Please type the client IP");

			std::cin >> cmd;
			std::cin.ignore();

			IN_ADDR IP_struct;

			inet_pton(AF_INET, cmd.data(), &IP_struct);

			server.clients_mutex.lock();

			auto client_it = server.clientPool.find(IP_struct.s_addr);

			if      (client_it == end(server.clientPool))
				log_raw_colored(ConsoleColor::WarningHighlighted, "Client not found!"); // Клиент не найден
			else {
				ConnectedClient& client = *(client_it->second);

				if (client.isRunning()) {
					log_raw_colored(ConsoleColor::Info, "Please type the data you want to send");

					std::getline(std::cin, cmd);

					client.sendPacket(packetFactory.create(cmd.data(), cmd.size(), false));
				}
				else
					log_raw_colored(ConsoleColor::WarningHighlighted, "Client is not active!"); // Клиент неактивен
			}

			server.clients_mutex.unlock();
		}
		else if (cmd == "send_all") {
			log_raw_colored(ConsoleColor::Info, "Please type the data you want to send");

			std::getline(std::cin, cmd);

			server.processClients(
				true, // Посылаем пакеты только активным клиентам
				[cmd](ConnectedClient& client) -> int 
				{ client.sendPacket(packetFactory.create(cmd.data(), cmd.size(), false)); return 0; }
			);
		}
		else if (cmd == "save") {
			server.processClients(
				false, // Сохраняем данные всех клиентов, не только активных
				[](ConnectedClient& client) -> int { saveData(client); return 0; }
			);

			log_colored(ConsoleColor::SuccessHighlighted, "Data successfully saved!");
		}
		else if (cmd == "clean") {
			server.cleanInactiveClients();
		}
		else if (cmd == "close") { // Закрытие сервера
			if (server.closeServer())
				log_raw_colored(ConsoleColor::DangerHighlighted, "Error while closing the server");
		}
	}

	return 0;
}

int main()
{
	// Задать имя потоку
	setThreadDesc(L"main");

	if (int err = start())
		log_colored(ConsoleColor::DangerHighlighted, "Server creating failed - error: %d", err);

	int v;
	std::cin >> v; // Чтобы не закрывалось окно

	return 0;
}