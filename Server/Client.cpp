#include "pch.h"

#include "Client.h"

Client::Client(SOCKET clientSocket, PCSTR IP, USHORT port)
	: clientSocket(clientSocket)
	, IP(IP)
	, port(port)
	, client_started(true)
{
	setState(CLIENT_STATE::OK);

	// TODO: поток для клиента
}

Client::~Client() {
	if(client_started) disconnect();

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
}

int Client::sendData() {
	// Send an initial buffer
	setState(CLIENT_STATE::SEND);

	std::string req;

	cout << "Type what you want to send to client: " << endl << '>';
	std::getline(std::cin, req);

	auto packet = std::make_unique<Packet>(req.c_str(), req.size());

	if (send(clientSocket, packet->data, packet->size, 0) == SOCKET_ERROR) return 1;

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
	}
	while (respSize > 0);

	setState(CLIENT_STATE::OK);
	return 0;
}

int Client::disconnect() {
	if (!client_started) return 0;

	// Shutdown the connection since no more data will be sent
	setState(CLIENT_STATE::SHUTDOWN);

	if (shutdown(clientSocket, SD_BOTH) == SOCKET_ERROR) return 1;

	// Close the socket
	setState(CLIENT_STATE::CLOSE_SOCKET);

	if (closesocket(clientSocket) == SOCKET_ERROR) return 1;

	cout << "The client was stopped" << endl;

	client_started = false;

	setState(CLIENT_STATE::OK);
	return 0;
}

void Client::setState(CLIENT_STATE state)
{
#ifdef _DEBUG
	const char state_desc[32];

#define PRINT_STATE(X) case SERVER_STATE:: X: \
	state_desc = #X; \
	break;

	switch (state) {
		PRINT_STATE(OK);
		PRINT_STATE(SEND);
		PRINT_STATE(RECEIVE);
		PRINT_STATE(SHUTDOWN);
		PRINT_STATE(CLOSE_SOCKET);
	default:
		std::cout << "Unknown state: " << (int)state << std::endl;
		return;
}
#undef PRINT_STATE

	std::cout << "State changed to: " << state_desc << std::endl;
#endif

	this->state = state;
}