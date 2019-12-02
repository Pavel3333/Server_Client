#include "pch.h"
#include <thread>
#include "Common.h"
#include "Cleaner.h"
#include "Server.h"

// Запуск клинера
void Cleaner::startCleaner()
{
	if (started)
		return;

	// Init class members
	this->mode = CleanerMode::OnlyDisconnect;

	cleaner = std::thread(&Cleaner::inactiveClientsCleaner, this);

	started = true;

	mode = CleanerMode::OnlyDisconnect;

	LOG::raw_colored(ConsoleColor::SuccessHighlighted, "Cleaner enabled!");
	printMode();
	printCommandsList();
}

// Отключение клинера
void Cleaner::closeCleaner()
{
	if (!started)
		return;

	started = false;

	if (cleaner.joinable())
		cleaner.join();

	LOG::raw_colored(ConsoleColor::SuccessHighlighted, "Cleaner disabled!");
	printCommandsList();
}


// Напечатать команды клинера
void Cleaner::printCommandsList() {
	LOG::raw_colored(ConsoleColor::InfoHighlighted, "Commands for managing the cleaner:");
	if (!started)
		LOG::raw_colored(ConsoleColor::Info,        "  \"enable_cleaner\"      => Enable inactive clients cleaner");
	else {
		LOG::raw_colored(ConsoleColor::Info,        "  \"get_cleaner_mode\"    => Print cleaner mode");
		LOG::raw_colored(ConsoleColor::Info,        "  \"change_cleaner_mode\" => Change cleaner mode");
		LOG::raw_colored(ConsoleColor::Warning,     "  \"disable_cleaner\"     => Disable inactive clients cleaner");
	}
}

// Напечатать режим работы клинера
void Cleaner::printMode() {
	const char* modeDesc = "-";

	switch(mode) {
		case CleanerMode::OnlyDisconnect:
			modeDesc = "Only disconnect";
			break;
		case CleanerMode::AgressiveMode:
			modeDesc = "Agressive mode";
			break;
		default:
			modeDesc = "Invalid";
	}

	LOG::colored(ConsoleColor::InfoHighlighted, "Cleaner mode: %s", mode);
}

void Cleaner::changeMode() {
	uint8_t cmd;
	
	LOG::raw_colored(ConsoleColor::InfoHighlighted, "Type the desired mode:");
	LOG::colored(ConsoleColor::Info,                "  %d - Only disconnect", CleanerMode::OnlyDisconnect);
	LOG::colored(ConsoleColor::Info,                "  %d - Agressive mode",  CleanerMode::AgressiveMode);

	std::cin >> cmd;

	switch(cmd) {
		case (uint8_t)(CleanerMode::OnlyDisconnect):
		case (uint8_t)(CleanerMode::AgressiveMode):
			mode = (CleanerMode)cmd;
			LOG::raw_colored(ConsoleColor::SuccessHighlighted, "Cleaner mode changed successfully!");
			break;
		default:
			LOG::raw_colored(ConsoleColor::WarningHighlighted, "Invalid mode was typed");
	}
}


// Очистка неактивных клиентов
void Cleaner::cleanInactiveClients(bool ext)
{
	size_t cleaned = 0;

	Server::getInstance().clients_mutex.lock();

	auto client_it = Server::getInstance().clientPool.begin();
	auto end       = Server::getInstance().clientPool.end();
	while (client_it != end) {
		ConnectedClient& client = *(client_it->second);

		if (!client.isRunning() && !client.isDisconnected()) {
			if (client.disconnect()) {
				cleaned++;
				if (mode == CleanerMode::AgressiveMode) {
					client_it = Server::getInstance().clientPool.erase(client_it);
					continue;
				}
			}
		}

		client_it++;
	}

	Server::getInstance().clients_mutex.unlock();

	if (cleaned) {
		const char* verb = "Disconnected";
		if (mode == CleanerMode::AgressiveMode)
			verb = "Cleaned";
		
		LOG::colored(ConsoleColor::InfoHighlighted, "%s %d inactive clients", verb, cleaned);
	}
	else if (ext)
		LOG::raw_colored(ConsoleColor::InfoHighlighted, "All clients are active");
}

// Каждые 5 секунд очищать неактивных клиентов
void Cleaner::inactiveClientsCleaner() {
	// Задать имя потоку
	setThreadDesc(L"[Server][Cleaner]");

	while (Server::getInstance().isRunning() && started) {
		cleanInactiveClients();

		Sleep(5000);
	}
}
