#include "pch.h"

#include "Common.h"

Packet::Packet(const char* data, size_t size, bool needConfirm)
	: needConfirm(needConfirm)
{
	if (!size) size = strnlen_s(data, NET_BUFFER_SIZE);

	this->size = size;

	this->data = new char[size + 2];
	memcpy(this->data, data, size);
	this->data[size] = NULL; //NULL-terminator

#ifdef _DEBUG
	cout << "Packet: " << size << ", data: ";
#endif

	cout << this->data << endl;
}

Packet::~Packet() {
	delete this->data;
}