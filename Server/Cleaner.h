#pragma once


enum class CleanerMode {
    OnlyDisconnect,
    AgressiveMode
};

// Синглтон клинера
class Cleaner {
private:
    bool started = false;

    CleanerMode mode;

    std::thread cleaner;

    // Защита от копирования
    Cleaner() {}
    Cleaner(const Cleaner&) {}
    Cleaner& operator=(Cleaner&) {}

    void startCleanerImpl();
    void closeCleanerImpl();
    void printCommandsListImpl() /* const */ ;
    void printModeImpl() /* const */ ;
    void changeModeImpl();
    void cleanInactiveClientsImpl(bool ext = false);
    void inactiveClientsCleanerImpl();

public:
    static Cleaner& getInstance()
    {
        static Cleaner instance;
        return instance;
    }

    static void startCleaner()
    {
        getInstance().startCleanerImpl();
    }

    static void closeCleaner()
    {
        getInstance().closeCleanerImpl();
    }

    static void printCommandsList() /* const */
    {
        getInstance().printCommandsListImpl();
    }

    static void printMode() /* const */
    {
        getInstance().printModeImpl();
    }

    static void changeMode()
    {
        getInstance().changeModeImpl();
    }

    static void cleanInactiveClients(bool ext = false)
    {
        getInstance().cleanInactiveClientsImpl(ext);
    }

    static void inactiveClientsCleaner()
    {
        getInstance().inactiveClientsCleanerImpl();
    }
};
