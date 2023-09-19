#include "CPacket.h"
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
static const int DefualtSize = 1400;

CPacket::CPacket()
{
	buffer = (char*)malloc(DefualtSize);
	WritePointer = buffer;
	ReadPointer = buffer;
	BufferSize = DefualtSize;
	UseSize = 0;

	EndPointer = buffer + DefualtSize;
}

CPacket::CPacket(int size)
{
	buffer = (char*)malloc(size);
	WritePointer = buffer;
	ReadPointer = buffer;
	BufferSize = size;
	UseSize = 0;

	EndPointer = buffer + size;
}

CPacket::~CPacket()
{
	free(buffer);
}

void CPacket::Release()
{
}

void CPacket::Clear()
{
	WritePointer = buffer;
	ReadPointer = buffer;
	UseSize = 0;
}

int CPacket::MoveWritePos(int size)
{
	if ((WritePointer + size) > EndPointer)
		return 0;
	WritePointer += size;
	UseSize += size;
	return size;
}

int CPacket::MoveReadPos(int size)
{
	if ((ReadPointer + size) > WritePointer)
		return 0;
	ReadPointer += size;
	return size;
}

int CPacket::GetData(char* Dest, int size)
{
	memcpy(Dest, ReadPointer, size);
	return size;
}

int CPacket::PutData(char* Src, int size)
{
	if (size > BufferSize - UseSize)
		return 0;
	memcpy(WritePointer, Src, size);
	UseSize += size;
	return size;
}

//CPacket& CPacket::operator=(CPacket& clSrcPacket)
//{
//	// TODO: 여기에 return 문을 삽입합니다.
//}

CPacket& CPacket::operator<<(unsigned char value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(unsigned char));
	MoveWritePos(size);
	return *this;
}

CPacket& CPacket::operator<<(char value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof( char));
	MoveWritePos(size);
	return *this;
}

CPacket& CPacket::operator<<(unsigned short value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(unsigned short));
	MoveWritePos(size);
	return *this;
}

CPacket& CPacket::operator<<(short value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(short));
	MoveWritePos(size);
	return *this;
}

CPacket& CPacket::operator<<(unsigned int value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(unsigned int));
	MoveWritePos(size);
	return *this;
}

CPacket& CPacket::operator<<(int value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(int));
	MoveWritePos(size);
	return *this;
}

CPacket& CPacket::operator<<(long value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(long));
	MoveWritePos(size);
	return *this;
}

CPacket& CPacket::operator<<(float value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(float));
	MoveWritePos(size);
	return *this;
}

CPacket& CPacket::operator<<(double value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(double));
	MoveWritePos(size);
	return *this;
}

CPacket& CPacket::operator<<(__int64 value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(__int64));
	MoveWritePos(size);
	return *this;
}

//OUTPUT

CPacket& CPacket::operator>>(unsigned char& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(unsigned char));
	MoveReadPos(size);
	return *this;
}

CPacket& CPacket::operator>>(char& value)
{
	//TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof( char));
	MoveReadPos(size);
	return *this;
}

CPacket& CPacket::operator>>(unsigned short& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(short));
	MoveReadPos(size);
	return *this;
}

CPacket& CPacket::operator>>(short& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof( short));
	MoveReadPos(size);
	return *this;
}

CPacket& CPacket::operator>>(unsigned int& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(unsigned int));
	MoveReadPos(size);
	return *this;
}

CPacket& CPacket::operator>>(int& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof( int));
	MoveReadPos(size);
	return *this;
}

CPacket& CPacket::operator>>(long& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof( long));
	MoveReadPos(size);
	return *this;
}

CPacket& CPacket::operator>>(float& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(float ));
	MoveReadPos(size);
	return *this;
}

CPacket& CPacket::operator>>(double& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(double ));
	MoveReadPos(size);
	return *this;
}

CPacket& CPacket::operator>>(__int64& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(__int64 ));
	MoveReadPos(size);
	return *this;
}

