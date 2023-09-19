#pragma once



class RingQueue
{
public:
	RingQueue();
	RingQueue(int size);
	~RingQueue();
	int GetUseSize();
	int GetFreeSize();
	int GetDirectEnqueueSize();
	int GetDirectDequeueSize();
	int GetDirectPacketDequeueSize();

	int MoveWritePointer(int size); //MoveRear
	int MoveReadPointer(int size); //MoveFront

	int MovePacketPointer(int size);

	int Enqueue(const char* buf, int size);
	int Dequeue(char* buf, int size);

	int PacketDequeue(char* buf, int size);
	void SetPacketPointer() {PacketBufferPointer = ReadPointer;}
	int Peek(char* buf, int size);
	char* GetFirstPointer() { return FirstBufferPoint; }
	char* GetWritePointer() { return WritePointer; }
	char* GetReadPointer() { return ReadPointer; }
	void Clear() { WritePointer = buffer; ReadPointer = buffer; PacketBufferPointer = buffer; }
private:
	
public: 
	char* buffer;

private:
	char* WritePointer; //Rear
	char* ReadPointer; //Front
	char* FirstBufferPoint;
	char* EndBufferPoint;
	char* PacketBufferPointer;
};