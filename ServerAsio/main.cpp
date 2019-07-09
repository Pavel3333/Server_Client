#include "pch.h"
#include <iostream>
#include <thread>

#define _WIN32_WINNT 0x0603
#include "asio.hpp"

using asio::ip::tcp;

#define PORT 27010
#define MAX_LEN 1024

void session(tcp::socket sock)
{
	while (true) {
		char data[MAX_LEN];

		std::error_code error;
		size_t length = sock.read_some(asio::buffer(data), error);
		if (error == asio::stream_errc::eof)
			break; // Connection closed cleanly by peer.
		else if (error)
			break;

		asio::write(sock, asio::buffer(data, length));
	}
}

void server(asio::io_context& io_context, unsigned short port)
{
	tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), port));
	while (true) {
		tcp::socket sock(io_context);
		a.accept(sock);
		std::thread(session, std::move(sock)).detach();
	}
}

int main(int argc, char* argv[])
{
	asio::io_context io_context;
	server(io_context, PORT);

	return 0;
}
