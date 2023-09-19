#include "RingQueue.h"
#include <Windows.h>
//#include "MPacket.h"
#define DEFAULTSIZE 10000


RingQueue::RingQueue()
{
	buffer = (char*)malloc(sizeof(char) * DEFAULTSIZE);
	ReadPointer = buffer;
	WritePointer = buffer;
	FirstBufferPoint = buffer;
	EndBufferPoint = buffer + DEFAULTSIZE - 1;
	PacketBufferPointer = buffer;
}

RingQueue::RingQueue(int size)
{
	buffer = (char*)malloc(sizeof(char) * (size + 1));
	ReadPointer = buffer;
	WritePointer = buffer;
	FirstBufferPoint = buffer;
	EndBufferPoint = buffer + size - 1;
	PacketBufferPointer = buffer;
}

RingQueue::~RingQueue()
{
	free(buffer);
}
/*
	if size = 11 -> 사용가능 버퍼 10

	char * burffer ->	1	2	3	4	5	6	7	8	9	10	11(unuse)
*/

int RingQueue::GetUseSize()
{
	char* wPointer = WritePointer;
	char* rPointer = ReadPointer;
	if(wPointer >= rPointer)
		return wPointer - rPointer;
	return (wPointer - FirstBufferPoint) + (EndBufferPoint - rPointer);
}

int RingQueue::GetFreeSize()
{
	char* wPointer = WritePointer;
	char* rPointer = ReadPointer;

	if (wPointer >= rPointer)
		return (rPointer - FirstBufferPoint) + (EndBufferPoint - wPointer);
	return rPointer - wPointer - 1;
}

int RingQueue::GetDirectEnqueueSize()
{
	char* wPointer = WritePointer;
	char* rPointer = ReadPointer;
	if (wPointer >= rPointer)
		return EndBufferPoint - wPointer;
	return rPointer - wPointer - 1;
}

int RingQueue::GetDirectDequeueSize()
{
	if (WritePointer >= ReadPointer)
		return WritePointer - ReadPointer;
	return (EndBufferPoint - ReadPointer);
	
}

int RingQueue::GetDirectPacketDequeueSize()
{
	if (WritePointer >= PacketBufferPointer)
		return WritePointer - PacketBufferPointer;
	return (EndBufferPoint - PacketBufferPointer);
}

int RingQueue::MoveWritePointer(int size)
{
	WritePointer += size;
	if (WritePointer >= EndBufferPoint)
	{
		int OverPointer = WritePointer - EndBufferPoint;
		WritePointer = FirstBufferPoint + OverPointer;
	}
	return 0;
}

int RingQueue::MoveReadPointer(int size)
{
	ReadPointer += size;
	if (ReadPointer >= EndBufferPoint)
	{
		int OverPointer = ReadPointer - EndBufferPoint;
		ReadPointer = FirstBufferPoint + OverPointer;
	}
	return 0;
}
int RingQueue::MovePacketPointer(int size)
{
	PacketBufferPointer += size;
	if (PacketBufferPointer >= EndBufferPoint)
	{
		int OverPointer = PacketBufferPointer - EndBufferPoint;
		PacketBufferPointer = FirstBufferPoint + OverPointer;
	}
	
	return 0;
}
int RingQueue::Enqueue(const char* buf, int size)
{
	if (GetFreeSize() < size) return 0;
	//크기가 충분할때
	if (GetDirectEnqueueSize() >= size)
	{
		memcpy_s(WritePointer, size, buf, size);
		MoveWritePointer(size);
		return size;
	}

	const char* CopyPointer = buf;
	int EnqueueSize = GetDirectEnqueueSize();
	memcpy_s(WritePointer, EnqueueSize, CopyPointer, EnqueueSize);
	CopyPointer += EnqueueSize;
	MoveWritePointer(EnqueueSize);

	int RemzinSize = size - EnqueueSize;
	memcpy_s(WritePointer, RemzinSize, CopyPointer, RemzinSize);
	MoveWritePointer(RemzinSize);

	return size;
}

int RingQueue::Dequeue(char* buf, int size)
{
	if (GetUseSize() < size) return 0;
	//크기가 충분할때
	if (GetDirectDequeueSize() >= size)
	{
		memcpy_s(buf, size, ReadPointer, size);
		MoveReadPointer(size);
		return size;
	}

	char* CopyPointer = buf;
	int DequeueSize = GetDirectDequeueSize();
	memcpy_s(CopyPointer, DequeueSize, ReadPointer, DequeueSize);
	CopyPointer += DequeueSize;
	MoveReadPointer(DequeueSize);

	int RemzinSize = size - DequeueSize;
	memcpy_s(CopyPointer, RemzinSize, ReadPointer, RemzinSize);
	MoveReadPointer(RemzinSize);
	/*
	MPacket* mPacket = (MPacket*)buf;
	mPacket->Len;
	*/
	return size;
}
int RingQueue::PacketDequeue(char* buf, int size)
{
	if (GetUseSize() < size) return 0;
	//크기가 충분할때
	if (GetDirectPacketDequeueSize() >= size)
	{
		memcpy_s(buf, size, PacketBufferPointer, size);
		MovePacketPointer(size);
		return size;
	}

	char* CopyPointer = buf;
	int DequeueSize = GetDirectPacketDequeueSize();
	memcpy_s(CopyPointer, DequeueSize, PacketBufferPointer, DequeueSize);
	CopyPointer += DequeueSize;
	MovePacketPointer(DequeueSize);

	int RemzinSize = size - DequeueSize;
	memcpy_s(CopyPointer, RemzinSize, PacketBufferPointer, RemzinSize);
	MovePacketPointer(RemzinSize);
	return size;
}
int RingQueue::Peek(char* buf, int size)
{
	if (GetUseSize() < size) return 0;
	//크기가 충분할때
	if (GetDirectDequeueSize() >= size)
	{
		memcpy_s(buf, size, ReadPointer, size);
		//MoveReadPointer(size);
		return size;
	}
	char* origniPointer = ReadPointer;
	char* CopyPointer = buf;
	int DequeueSize = GetDirectDequeueSize();
	memcpy_s(CopyPointer, DequeueSize, ReadPointer, DequeueSize);
	CopyPointer += DequeueSize;
	MoveReadPointer(DequeueSize);

	int RemzinSize = size - DequeueSize;
	memcpy_s(CopyPointer, RemzinSize, ReadPointer, RemzinSize);
	ReadPointer = origniPointer;
	return size;
} 


