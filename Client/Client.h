#pragma once
#include <queue>
#include <thread>
#include <vector>

#include "Common.h"
#include "Handler.h"


enum class ClientState {
    InitWinSock,
    SetOpts,
    CreateAuthSocket,
    Auth,
    CreateDataSocket,
    Send,
    Receive,
    Shutdown,
    CloseSocket
};

class Client {
public:
    static Client& getInstance()
    {
        static Client instance;
        return instance;
    }

    ERR init(std::string_view login, std::string_view pass, PCSTR IP, uint16_t authPort, uint16_t dataPort);
    void disconnect();

    void sendPacket(PacketPtr packet) { mainPackets.push(packet); }

    // Getters
    bool isRunning() const
    {
        return started && authSocket.status == 2 && dataSocket.status == 2;
    }

    uint16_t getID() const { return ID; }
    std::string_view getToken() { return token; }

    // Other methods
    void printCommandsList() const;

    // Instances
    std::vector<PacketPtr> receivedPackets;
    std::vector<PacketPtr> sendedPackets;

    std::queue<PacketPtr> mainPackets;
    std::vector<PacketPtr> syncPackets;

private:
    ERR initSocket(Socket& sock, uint16_t port, IPPROTO protocol);
    ERR connect2server(Socket& sock, uint16_t port);

    ERR authorize(std::string_view login, std::string_view pass);

    ERR handlePacketIn(bool closeAfterTimeout);
    ERR handlePacketOut(PacketPtr packet);

    void receiverThread();
    void senderThread();

    void createThreads();

    ERR receiveData(PacketPtr& dest, bool closeAfterTimeout);
    ERR sendData(PacketPtr packet);

    void setState(ClientState state);

    std::thread receiver;
    std::thread sender;

    ClientState state;

    uint16_t ID;

    PCSTR IP;

    std::string token;

    uint16_t authPort;
    uint16_t dataPort;

    bool started;

    Socket authSocket;
    Socket dataSocket;
};