#pragma once

enum class CleanerMode {
	OnlyDisconnect,
	AgressiveMode
};

// Ñèíãëòîí êëèíåðà
class Cleaner {
public:
	static Cleaner& getInstance() {
		static Cleaner instance;
		return instance;
	}

	void startCleaner();
	void closeCleaner();

	void printCommandsList();
	void printMode();
	void changeMode();

	void cleanInactiveClients(bool ext = false);
	void inactiveClientsCleaner();
private:
	bool started = false;

	CleanerMode mode;

	std::thread cleaner;

	// Çàùèòà îò êîïèðîâàíèÿ
	Cleaner()                    {}
	Cleaner(const Cleaner&)      {}
	Cleaner& operator=(Cleaner&) {}
};
