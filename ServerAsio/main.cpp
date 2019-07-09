#include <iostream>
#include <string_view>
#include <thread>
#include <asio/write.hpp>
#include <asio/ip/tcp.hpp>

using asio::ip::tcp;

#define PORT 27010
#define MAX_LEN 1024

void session(tcp::socket sock)
{
	std::cout << "Client connected " << sock.remote_endpoint() << std::endl;

	while (true) {
		char data[MAX_LEN];

		std::error_code error;
		size_t length = sock.read_some(asio::buffer(data), error);
		if (error == asio::stream_errc::eof) {
			std::cout << "Connection closed cleanly by peer " << sock.remote_endpoint() << std::endl;
			break;
		} else if (error) {
			std::cout << "Error with client " << sock.remote_endpoint() << std::endl;
			break;
		}

		std::cout << "Incoming message from client " << sock.remote_endpoint()  << ": " << std::string_view(data, length) << std::endl;
		asio::write(sock, asio::buffer(data, length));
	}
}

void server(asio::io_context& io_context, unsigned short port)
{
	tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), port));
	std::cout << "Wait for first client..." << std::endl;
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
