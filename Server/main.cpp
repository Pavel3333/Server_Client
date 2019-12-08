#include "pch.h"
#include "Cleaner.h"
#include "Server.h"

const uint16_t AUTH_PORT = 27010;
const uint16_t DATA_PORT = 27011;


ERR start()
{
    int err;

    err = Server::startServer(AUTH_PORT);
    if (_ERROR(err))
        return ERR::E_START_SERVER;

    Server::printCommandsList();
    Cleaner::printCommandsList();

    while (Server::isRunning()) { // Прием команд из командной строки
        std::string cmd;

        std::cin >> cmd;
        std::cin.ignore();

        if (cmd == "list") { // Получаем данные всех активных клиентов
            Server::printClientsList(false);
        } else if (cmd == "list_detailed") { // Получаем расширенные данные всех активных клиентов
            Server::printClientsList(true);
        } else if (cmd == "send") { // Послать пакет определенному клиенту
            Server::send();
        } else if (cmd == "send_all") { // Посылаем пакеты активным клиентам
            Server::sendAll();
        } else if (cmd == "save") { // Сохранение данных всех клиентов
            Server::save();
        } else if (cmd == "clean") { // Очистка неактивных клиентов
            Cleaner::cleanInactiveClients(true);
        } else if (cmd == "commands") { // Вывод всех доступных команд
            Server::printCommandsList();
            Cleaner::printCommandsList();
        } else if (cmd == "close") { // Закрытие сервера
            Server::closeServer();
        } else if (cmd == "enable_cleaner") { // Включение клинера
            Cleaner::startCleaner();
        } else if (cmd == "get_cleaner_mode") { // Вывести режим работы клинера
            Cleaner::printMode();
        } else if (cmd == "change_cleaner_mode") { // Сменить режим работы клинера
            Cleaner::changeMode();
        } else if (cmd == "disable_cleaner") { // Выключение клинера
            Cleaner::closeCleaner();
        }
    }

    if (!Server::isRunning())
        Server::closeServer();

    return E_OK;
}

int main()
{
    ERR err = E_OK;

    // Задать имя потоку
    setThreadDesc(L"[main]");

    err = start();
    if (_ERROR(err))
        LOG::colored(CC_DangerHL, "Server creating failed - error: %d", err);

    LOG::raw_colored(CC_InfoHL, "Press any button to end execution of server");

    (void)std::cin.get(); // Чтобы не закрывалось окно

    return 0;
}