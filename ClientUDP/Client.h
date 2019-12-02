#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include "Common.h"


enum class ClientState {
	InitWinSock,
	SetOpts,
	CreateReadSocket,
	CreateWriteSocket,
	HelloReceiving,
	HelloSending,
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

	int init(std::string_view login, PCSTR IP, uint16_t readPort, uint16_t writePort);
	void disconnect();

	void sendPacket(PacketPtr packet) { mainPackets.push(packet); }

	// Getters
	bool isRunning() const {
		return started && readSocket  != INVALID_SOCKET
			           && writeSocket != INVALID_SOCKET; }

	uint16_t         getID()    const { return ID; }
	std::string_view getLogin() const { return login; }

	// Other methods
	void printCommandsList() const;

	// Instances

	std::vector<PacketPtr> receivedPackets;
	std::vector<PacketPtr> sendedPackets;

	std::queue<PacketPtr>  mainPackets;
	std::vector<PacketPtr> syncPackets;
private:
	SOCKET connect2server(uint16_t port);

	int handshake();

	int ack_handler(PacketPtr packet);
	int any_packet_handler(PacketPtr packet);

	int handlePacketIn(std::function<int(PacketPtr)>handler, bool closeAfterTimeout);
	int handlePacketOut(PacketPtr packet);

	void receiverThread();
	void senderThread();

	void createThreads();

	int receiveData(PacketPtr& dest, bool closeAfterTimeout);
	int sendData(PacketPtr packet);

	void setState(ClientState state);

	std::thread receiver;
	std::thread sender;

	ClientState state;

	uint16_t ID;
	std::string login;

	PCSTR IP;

	uint16_t readPort;
	uint16_t writePort;

	bool started;

	SOCKET readSocket;
	SOCKET writeSocket;
};