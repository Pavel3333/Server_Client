#pragma once
#include <vector>
#include <queue>
#include <thread>

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
	static Client& getInstance() {
		static Client instance;
		return instance;
	}

    ClientError init(std::string_view login, std::string_view pass, PCSTR IP, uint16_t authPort, uint16_t dataPort);
	void disconnect();

	void sendPacket(PacketPtr packet) { mainPackets.push(packet); }

	// Getters
	bool isRunning() const {
        return started && authSocket.status == 2 && dataSocket.status == 2; 
    }

	uint16_t getID() const { return ID; }
    std::string_view getToken() { return token; }

	// Other methods
	void printCommandsList() const;

	// Instances
	std::vector<PacketPtr> receivedPackets;
	std::vector<PacketPtr> sendedPackets;

	std::queue<PacketPtr>  mainPackets;
	std::vector<PacketPtr> syncPackets;
private:
    ClientError initSocket(Socket& sock, uint16_t port, IPPROTO protocol);
    ClientError connect2server(Socket& sock, uint16_t port);

    ClientError authorize(std::string_view login, std::string_view pass);

	ClientError handlePacketIn(bool closeAfterTimeout);
	ClientError handlePacketOut(PacketPtr packet);

	void receiverThread();
	void senderThread();

	void createThreads();

	ClientError receiveData(PacketPtr& dest, bool closeAfterTimeout);
	ClientError sendData(PacketPtr packet);

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