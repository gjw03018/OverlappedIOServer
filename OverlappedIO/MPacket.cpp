#include "MPacket.h"
#include <memory.h>

MPacket::MPacket()
{
	Header header = {0};
	*this << header.len;
	HeaderPtr = WritePointer;
}

MPacket::~MPacket()
{

}

void MPacket::SetHeader(Header& header)
{
	memcpy_s(buffer, sizeof(Header), (char*)&header, sizeof(Header));
}

void MPacket::Clear()
{
	ReadPointer = buffer;
	WritePointer = HeaderPtr;
	UseSize = sizeof(Header);
}

