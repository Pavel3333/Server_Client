#pragma once
#include <atomic>

#include "Error.h"
#include "Socket.h"


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

	int send(Socket& sock) {
		return ::send(sock.getSocket(), data(), static_cast<int>(size()), 0);
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

// TODO: struct DataPacket - пакет, у которого в заголовке token (для идентификации отправителя)


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

	template <class T> 
	static PacketPtr create_from_struct(T &packet, bool needACK) {
		return create(
			reinterpret_cast<const char*>(&packet), sizeof(packet), needACK);
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


// Auth packets

#pragma pack(push,1)
struct ServerAuthPacket {
    SERVER_ERR errorCode;
    uint8_t tokenSize : TOKEN_BITCNT;
    char    token[TOKEN_MAX_SIZE];
};

struct ClientAuthPacket {
    ClientAuthPacket() {
		loginSize =  0;
		passSize  =  0;
	}

    ClientAuthPacket(std::string_view login, std::string_view pass) {
        loginSize = static_cast<uint8_t>(min(login.size(), LOGIN_MAX_SIZE));
		passSize  = static_cast<uint8_t>(min(pass.size(),  PWD_MAX_SIZE));
		memcpy(this->login,             login.data(), loginSize);
		memcpy(this->login + loginSize, pass.data(),  passSize);
	}

    uint8_t  loginSize : LOGIN_BITCNT;
    uint8_t  passSize  : PWD_BITCNT;
	char     login[LOGIN_MAX_SIZE];
    char     pass [PWD_MAX_SIZE];
};

// check packing
static_assert(sizeof(ServerAuthPacket) == 5 + TOKEN_MAX_SIZE);
static_assert(sizeof(ClientAuthPacket) == 2 + LOGIN_MAX_SIZE + PWD_MAX_SIZE);
#pragma pack(pop)