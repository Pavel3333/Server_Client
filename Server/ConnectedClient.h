#pragma once
#include <thread>
#include <memory>
#include <queue>
#include <functional>
#include "Common.h"


enum class ClientState : uint8_t {
	FirstHandshake,
	SecondHandshake,
	HelloSending,
	HelloReceiving,
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

	void resetSocketsAndPorts();

	void first_handshake(SOCKET socket, uint16_t port);
	int second_handshake(SOCKET socket, uint16_t port);

	void createThreads();

	// Getters
	bool isRunning() const {
		return started && readSocket  != INVALID_SOCKET
			           && writeSocket != INVALID_SOCKET; }

	bool isDisconnected() const { return disconnected; }

	uint16_t getID() const { return ID; }

	std::string_view getLogin()     const { return login; }
	uint32_t         getLoginHash() const { return loginHash; }

	uint32_t    getIP_u32() const { return IP; }
	const char* getIP_str() const { return IP_str; }

	int    getPort(bool isReadPort)     const { return isReadPort   ? readPort   : writePort; }
	SOCKET getSocket(bool isReadSocket) const { return isReadSocket ? readSocket : writeSocket; }

	void getInfo(bool ext = false);

	// Setters
	void setPort(bool isReadPort, int port)          { if (isReadPort)   readPort   = port;   else writePort   = port; }
	void setSocket(bool isReadSocket, SOCKET socket) { if (isReadSocket) readSocket = socket; else writeSocket = socket; }

	void sendPacket(PacketPtr packet) { mainPackets.push(packet); }
	bool disconnect();


private:
	sockaddr_in clientDesc;
	SOCKET readSocket;
	SOCKET writeSocket;

	int ack_handler(PacketPtr packet);
	int any_packet_handler(PacketPtr packet);

	int handlePacketIn(std::function<int(PacketPtr)>handler, bool closeAfterTimeout);
	int handlePacketOut(PacketPtr packet);

	void receiverThread();
	void senderThread();

	int receiveData(PacketPtr& dest, bool closeAfterTimeout);
	int sendData(PacketPtr packet);

	void setState(ClientState state);

	std::thread receiver;
	std::thread sender;

	bool started;
	bool disconnected;

	uint16_t ID;
	uint32_t IP;

	int readPort;
	int writePort;

	std::string login;
	uint32_t loginHash;

	ClientState state;

	char IP_str[16];
	char host[NI_MAXHOST];
};

typedef std::shared_ptr<ConnectedClient> ConnectedClientPtr;

std::ostream& operator<< (std::ostream& os, ConnectedClient& client);

// Синглтон фабрики подключенных к серверу клиентов
class ConnectedClientFactory
{
public:
	static ConnectedClientPtr create(sockaddr_in clientDesc, int clientLen)
	{
		return make_shared<ConnectedClient>(getID(), clientDesc, clientLen);
	}
private:
	static std::atomic_uint getID() { // Возможно, нужно предварительно обнулять ID?
		static std::atomic_uint ID;

		return ID++;
	}

	// Защита от копирования
	ConnectedClientFactory() {}
	ConnectedClientFactory(const ConnectedClientFactory&) {};
	ConnectedClientFactory& operator=(ConnectedClientFactory&) {};
};