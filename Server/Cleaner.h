#pragma once

enum class CleanerMode {
	OnlyDisconnect,
	AgressiveMode
};

// �������� �������
class Cleaner {
public:
	static Cleaner& getInstance() {
		static Cleaner instance;
		return instance;
	}

	void startCleaner();
	void closeCleaner();

	void printCommandsList();
	void printCleanerMode();

	void changeCleanerMode(CleanerMode dest) { mode = dest; }

	void cleanInactiveClients(bool ext = false);
	void inactiveClientsCleaner();
private:
	bool started = false;

	CleanerMode mode;

	std::thread cleaner;

	// ������ �� �����������
	Cleaner()                    {}
	Cleaner(const Cleaner&)      {}
	Cleaner& operator=(Cleaner&) {}
};