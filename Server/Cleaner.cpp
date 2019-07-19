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

	log_raw_colored(ConsoleColor::SuccessHighlighted, "Cleaner enabled!");
	printCleanerMode();
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

	log_raw_colored(ConsoleColor::SuccessHighlighted, "Cleaner disabled!");
	printCommandsList();
}


// Напечатать команды клинера
void Cleaner::printCommandsList() {
	log_raw_colored(ConsoleColor::InfoHighlighted, "Commands for managing the cleaner:");
	if (!started)
		log_raw_colored(ConsoleColor::Info,        "  \"enable_cleaner\"      => Enable inactive clients cleaner");
	else {
		log_raw_colored(ConsoleColor::Info,        "  \"get_cleaner_mode\"    => Print cleaner mode");
		log_raw_colored(ConsoleColor::Info,        "  \"change_cleaner_mode\" => Change cleaner mode");
		log_raw_colored(ConsoleColor::Warning,     "  \"disable_cleaner\"     => Disable inactive clients cleaner");
	}
}

// Напечатать режим работы клинера
void Cleaner::printCleanerMode() {
	const char* modeDesc = "-";

	if      (mode == CleanerMode::OnlyDisconnect)
		modeDesc = "Only disconnect";
	else if (mode == CleanerMode::AgressiveMode)
		modeDesc = "Agressive mode";

	log_colored(ConsoleColor::InfoHighlighted, "Cleaner mode: %s", mode);
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
		
		log_colored(ConsoleColor::InfoHighlighted, "%s %d inactive clients", verb, cleaned);
	}
	else if (ext)
		log_raw_colored(ConsoleColor::InfoHighlighted, "All clients are active");
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