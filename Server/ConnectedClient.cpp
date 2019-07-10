#include "pch.h"

#include "ConnectedClient.h"

ConnectedClient::ConnectedClient(SOCKET clientSocket, PCSTR IP, USHORT port)
	: clientSocket(clientSocket)
	, IP(IP)
	, port(port)
	, client_started(true)
{
	setState(CLIENT_STATE::OK);
}

ConnectedClient::~ConnectedClient() {
	if(client_started) disconnect();

	error_code = WSAGetLastError();

	// ����� ��������� �� ������

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


int ConnectedClient::createThread() { // Handler thread creating
	handler = std::thread(&ConnectedClient::handlerThread, this);
	handler.detach();
}

int ConnectedClient::handlePacket(std::unique_ptr<Packet> packet) { // ��������� ������ �� �������
	if (sendData(std::move(packet))) return 1;

	// ������� � ������ ���� ������������ �������
	sendedPackets.push_back(std::move(packet));

	if (packet->needConfirm) {
		Packet resp;

		if (receiveData(&resp)) return 2;

		// ������� � ������ ���� ��������� �������
		receivedPackets.push_back(std::make_unique<Packet>(resp));

		// ��������� ���������� ������
		cout << "Data received: " << resp.data << endl;
	}

	return 0;
}

void ConnectedClient::handlerThread() { // ����� ��������� �������
	while (client_started) {
		// ���������� �������� ������
		while (!mainPackets.empty()) {
			std::unique_ptr<Packet> packet = std::move(mainPackets.back());

			if (handlePacket(std::move(packet))) {
				cout << "Packet not confirmed, adding to sync queue" << endl; // TODO: ������ Packet ID
				syncPackets.push_back(std::move(packet));
			}
			
			mainPackets.pop();
		}

		// ���������� ������, �� �������������� ���������
		auto packet = syncPackets.begin();
		while (packet != syncPackets.end()) {
			if (handlePacket(std::move(*packet))) {
				cout << "Packet not confirmed" << endl; // TODO: ������ Packet ID
				packet++;
			}
			else packet = syncPackets.erase(packet);
		}

		Sleep(100);
	};

	// ��������� �����
	cout << "Closing handler thread " << handler.get_id() << endl;
}

int ConnectedClient::sendData(std::unique_ptr<Packet> packet) {
	// Send an initial buffer
	setState(CLIENT_STATE::SEND);

	if (!packet->data) { //������ ������
		std::string req;

		cout << "Type what you want to send to client: " << endl << '>';
		std::getline(std::cin, req);

		packet = std::make_unique<Packet>(req.c_str(), req.size());
	}

	if (send(clientSocket, packet->data, packet->size, 0) == SOCKET_ERROR) return 1;

	sendedPackets.push_back(std::move(packet));

	setState(CLIENT_STATE::OK);
	return 0;
}

int ConnectedClient::receiveData(Packet* dest) {
	// Receive until the peer closes the connection
	setState(CLIENT_STATE::RECEIVE);

	char respBuff[NET_BUFFER_SIZE];

	int respSize = recv(clientSocket, respBuff, NET_BUFFER_SIZE, 0);

	if (respSize > 0) { //���������� ������ �� ������� (TODO: ������ ���� � ID �������)
		*dest = Packet(respBuff, respSize);
	}
	else if (!respSize) {
		cout << "Connection closed" << endl;
		return 2;
	}
	else return 1;

	setState(CLIENT_STATE::OK);
	return 0;
}

int ConnectedClient::disconnect() {
	if (!client_started) return 0;

	// Shutdown the connection since no more data will be sent
	setState(CLIENT_STATE::SHUTDOWN);

	if (shutdown(clientSocket, SD_BOTH) == SOCKET_ERROR) cout << "Error while shutdowning connection" << endl;

	// Close the socket
	setState(CLIENT_STATE::CLOSE_SOCKET);

	if (closesocket(clientSocket) == SOCKET_ERROR) cout << "Error while closing socket" << endl;

	cout << "The client was stopped" << endl;

	client_started = false;

	setState(CLIENT_STATE::OK);
	return 0;
}

void ConnectedClient::setState(CLIENT_STATE state)
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