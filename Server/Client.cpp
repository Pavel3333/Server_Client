#include "pch.h"

#include "Client.h"

Client::Client(SOCKET clientSocket, PCSTR IP, USHORT port)
	: clientSocket(clientSocket)
	, IP(IP)
	, port(port)
{
	setState(CLIENT_STATE::OK);
}

Client::~Client() {
	error_code = WSAGetLastError();

	// Вывод сообщения об ошибке

	if (state != CLIENT_STATE::OK) cout << "state " << (int)state << " - error: " << error_code << endl;
	else if (!receivedPackets.empty()) {
#ifdef _DEBUG
		cout << "All received data:" << endl;
		size_t i = 1;

		for (auto& it : receivedPackets) cout << i++ << ':' << endl << "  size: " << it->size << endl << "  data: " << it->data << endl;
#endif

		receivedPackets.clear();
	}

	if (state > CLIENT_STATE::CREATE_SOCKET) closesocket(clientSocket);

	WSACleanup();
}


int Client::connect2client() {
	// Create a SOCKET for connecting to client (TCP/IP protocol)
	setState(CLIENT_STATE::CREATE_SOCKET);

	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) return 1;

	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.

	socketDesc.sin_family = AF_INET;
	socketDesc.sin_port = htons(port);
	inet_pton(AF_INET, IP, &socketDesc.sin_addr.s_addr);

	// Connect to server.
	setState(CLIENT_STATE::CONNECT);

	if (connect(clientSocket, (SOCKADDR*)&socketDesc, sizeof(socketDesc)) == SOCKET_ERROR) return 1;

	char addr_str[16];

	if (!inet_ntop(AF_INET, &(socketDesc.sin_addr), addr_str, 32)) cout << "Cannot to get addr string of server IP" << endl;
	else                                                           cout << "The client was connected to the server " << addr_str << ':' << ntohs(socketDesc.sin_port) << endl;

	setState(CLIENT_STATE::OK);
	return 0;
}

int Client::sendData() {
	// Send an initial buffer
	setState(CLIENT_STATE::SEND);

	std::string req;

	cout << "Type what you want to send to client: " << endl << '>';
	std::getline(std::cin, req);

	auto packet = std::make_unique<Packet>(req.c_str(), req.size());

	int bytesSent = send(clientSocket, packet->data, packet->size, 0);
	if (bytesSent == SOCKET_ERROR) return 1;

	sendedPackets.push_back(std::move(packet));

	setState(CLIENT_STATE::OK);
	return 0;
}

int Client::receiveData() {
	// Receive until the peer closes the connection
	setState(CLIENT_STATE::RECEIVE);

	char respBuff[NET_BUFFER_SIZE];
	int respSize;

	do {
		respSize = recv(clientSocket, respBuff, NET_BUFFER_SIZE, 0);
		if (respSize > 0) { //Записываем данные от клиента (TODO: писать туда и ID клиента)
			receivedPackets.push_back(std::make_unique<Packet>(respBuff, respSize));

			if (sendData()) cout << "SEND - error: " << WSAGetLastError() << endl;
		}
		else if (respSize == 0) cout << "Connection closed" << endl;
		else
			return 1;
	} while (respSize > 0);

	setState(CLIENT_STATE::OK);
	return 0;
}

int Client::disconnect() {
	// Shutdown the connection since no more data will be sent
	setState(CLIENT_STATE::SHUTDOWN);

	if (shutdown(clientSocket, SD_BOTH) == SOCKET_ERROR) return 1;

	// Close the socket
	setState(CLIENT_STATE::CLOSE_SOCKET);

	if (closesocket(clientSocket) == SOCKET_ERROR) return 1;

	cout << "The client was stopped" << endl;

	setState(CLIENT_STATE::OK);
	return 0;
}

void Client::setState(CLIENT_STATE state)
{
#ifdef _DEBUG
#define PRINT_STATE(X) case CLIENT_STATE::X:           \
	std::cout << "State changed to: " #X << std::endl; \
	break;

	switch (state) {
		PRINT_STATE(OK);
		PRINT_STATE(INIT_WINSOCK);
		PRINT_STATE(CREATE_SOCKET);
		PRINT_STATE(CONNECT);
		PRINT_STATE(SEND);
		PRINT_STATE(SHUTDOWN);
		PRINT_STATE(RECEIVE);
		PRINT_STATE(CLOSE_SOCKET);
	default:
		std::cout << "Unknown state: " << (int)state << std::endl;
		return;
	}
#undef PRINT_STATE
#endif

	this->state = state;
}