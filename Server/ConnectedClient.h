#pragma once

#include "include/Common.h"

#include <thread>
#include <memory>
#include <functional>

enum class ClientState : uint8_t {
	FirstHandshake,
	SecondHandshake,
	Send,
	Receive,
	Shutdown,
	CloseSocket
};

class ConnectedClient {
public:
	ConnectedClient(uint16_t ID, sockaddr_in clientDesc, int clientLen);
	~ConnectedClient();

	std::vector<PacketPtr> receivedPackets;
	std::vector<PacketPtr> sendedPackets;

	std::queue<PacketPtr> mainPackets;
	std::vector<PacketPtr> syncPackets;

	int first_handshake(SOCKET socket);
	int second_handshake(SOCKET socket);

	bool     isRunning() { return started && readSocket != INVALID_SOCKET && writeSocket != INVALID_SOCKET; }
	uint16_t getID()     { return ID; }
	uint32_t getIP_u32() { return IP; }
	char*    getIP_str() { return IP_str; }

	void getInfo(bool ext = false);
	void sendPacket(PacketPtr packet) { mainPackets.push(packet); }
	void disconnect();

	sockaddr_in clientDesc;

	SOCKET readSocket;
	SOCKET writeSocket;
private:
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

	bool started;
	uint16_t ID;
	uint32_t IP;

	ClientState state;

	char IP_str[16];
	char host[NI_MAXHOST];
};

typedef std::shared_ptr<ConnectedClient> ConnectedClientPtr;
