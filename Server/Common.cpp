#include "pch.h"
#include <string_view>
#include "Common.h"

std::mutex msg_mutex;

void log_raw_nonl(const char* str) {
	msg_mutex.lock();
	cout << str;
	msg_mutex.unlock();
}

void log_raw(const char* str) {
	msg_mutex.lock();
	cout << str << endl;
	msg_mutex.unlock();
}


void log_nonl(const char* fmt, ...) {
	msg_mutex.lock();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	msg_mutex.unlock();
}

void log(const char* fmt, ...) {
	msg_mutex.lock();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	cout << endl;
	msg_mutex.unlock();
}

Packet::Packet(const char* data, size_t size, bool needACK)
	: needACK(needACK)
	, size(size)
{
	this->data = new char[size];
	memcpy(this->data, data, size);

#ifdef _DEBUG
	log("Packet: %d bytes, data: %s", size, std::string_view(data, size));
#endif
}

Packet::~Packet() { delete[] this->data; }


//std::ostream& operator<< (std::ostream& os, const Packet& packet)
//{
//	os << "Packet: " << packet.size << ", " << "data: " << std::string_view(packet.data, packet.size);
//	return os;
//}


void __wsa_print_err(const char* file, int line)
{
	msg_mutex.lock();
	int err = WSAGetLastError();

	char err_msg[256] = "";
	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
		err_msg, sizeof(err_msg), NULL);

	printf("%s:%d - WSA Error %d:\n%s", file, line, err, err_msg);
	msg_mutex.unlock();
}
