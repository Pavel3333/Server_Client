#pragma once

#include "Common.h"

#include <thread>
#include <memory>
#include <functional>

enum class ClientState : uint8_t {
	FirstHandshake,
	SecondHandshake,
	Send,
	Receive,
	Shutdown,
	CloseSockets
};

class ConnectedClient {
public:
	ConnectedClient(uint16_t ID, sockaddr_in clientDesc, int clientLen);
	~ConnectedClient();

	std::vector<PacketPtr> receivedPackets;
	std::vector<PacketPtr> sendedPackets;

	std::queue<PacketPtr> mainPackets;
	std::vector<PacketPtr> syncPackets;

	bool isRunning();

	uint16_t getID();

	uint16_t getIP_u16();
	char*    getIP_str();

	int first_handshake(SOCKET socket);
	int second_handshake(SOCKET socket);

	void createThreads();

	int receiveData(PacketPtr& dest);
	int sendData(PacketPtr packet);

	int disconnect();

	sockaddr_in clientDesc;

	SOCKET readSocket;
	SOCKET writeSocket;
private:
	int ack_handler(PacketPtr packet);
	int any_packet_handler(PacketPtr packet);

	int handlePacketIn(std::function<int(PacketPtr)>handler);
	int handlePacketOut(PacketPtr packet);

	void receiverThread();
	void senderThread();

	void setState(ClientState state);

	std::thread receiver;
	std::thread sender;

	uint16_t ID;
	uint32_t IP;
	bool started;

	ClientState state;

	char IP_str[16];
	char host[NI_MAXHOST];

	uint16_t port;
};

typedef std::shared_ptr<ConnectedClient> ConnectedClientPtr;
