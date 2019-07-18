#pragma once
#include <atomic>


class Packet;
typedef std::shared_ptr<Packet> PacketPtr;


class Packet : public std::vector<char> {
public:
	Packet(uint32_t ID, std::string_view, bool needACK);
	Packet(std::string_view);

	uint32_t getID() const {
		return header->ID;
	}

	bool isNeedACK() const {
		return header->needACK;
	}

	const char* getData() const {
		return this->data() + sizeof(packet_header);
	}

	uint32_t getDataSize() const {
		return header->fullsize - sizeof(packet_header);
	}

	int send(SOCKET s) {
		return ::send(s, data(), static_cast<int>(size()), 0);
	}

private:
#pragma pack(push,1)
	struct packet_header {
		uint32_t fullsize;
		uint32_t ID;
		bool needACK;
	} *header;

	// check packing
	static_assert(sizeof(packet_header) == 9);
#pragma pack(pop)
};


std::ostream& operator<< (std::ostream& os, Packet& packet);


// Packet Factory Singleton
class PacketFactory {
public:
	static PacketPtr create(const char* data, size_t size, bool needACK) {
		using std::make_shared;
		using std::string_view;

		return make_shared<Packet>(getID(), string_view(data, size), needACK);
	}

	static PacketPtr create(const char* data, size_t size) {
		using std::make_shared;
		using std::string_view;

		return make_shared<Packet>(string_view(data, size));
	}

private:
	static std::atomic_uint getID() {
		static std::atomic_uint ID;
		return ID++;
	}

	// copy guard
	PacketFactory() {}
	PacketFactory(const PacketFactory&) {}
	PacketFactory& operator=(PacketFactory&) {}
};


// Hello-packets

#pragma pack(push,1)
struct ServerHelloPacket {
	uint32_t clientID;
};

struct ClientHelloPacket {
	ClientHelloPacket() {
		serverHelloPacketID = ~0;
		login[0] = '\0';
	}

	ClientHelloPacket(uint32_t ID, std::string_view login) {
		serverHelloPacketID = ID;
		loginSize = (uint32_t)min(login.size(), LOGIN_MAX_SIZE - 1);
		memcpy(this->login, login.data(), loginSize);
		this->login[loginSize] = '\0';
	}

	uint32_t serverHelloPacketID;
	uint32_t loginSize;
	char     login[LOGIN_MAX_SIZE];
};

// check packing
static_assert(sizeof(ServerHelloPacket) == 4);
static_assert(sizeof(ClientHelloPacket) == 8 + LOGIN_MAX_SIZE);
#pragma pack(pop)
