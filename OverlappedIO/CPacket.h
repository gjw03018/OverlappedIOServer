#pragma once



class CPacket
{
public:
	CPacket();
	CPacket(int size);
	virtual ~CPacket();
	
	void Release();//패킷 파괴
	virtual void Clear();//패킷 청소
	
	int GetBufferSize() { return BufferSize; }
	int GetDataSize() { return WritePointer - ReadPointer; }

	char* GetBufferPtr() { return buffer; }

	char* GetReadBuffPtr() { return ReadPointer; }
	char* GetWriteBuffPtr() { return WritePointer; }

	int MoveWritePos(int size);
	int MoveReadPos(int size);

	int GetData(char* Dest, int size);
	int PutData(char* Src, int size);
public:
	//연산자 오버로딩
	CPacket& operator = (CPacket& clSrcPacket);
	//INPUT
	CPacket& operator << (unsigned char value);
	CPacket& operator << (char value);
	CPacket& operator << (unsigned short value);
	CPacket& operator << (short value);
	CPacket& operator << (unsigned int value);
	CPacket& operator << (int value);
	CPacket& operator << (long value);
	CPacket& operator << (float value);
	CPacket& operator << (double value);
	CPacket& operator << (__int64 value);

	//OUTPUT
	CPacket& operator >> (unsigned char &value);
	CPacket& operator >> (char &value);
	CPacket& operator >> (unsigned short &value);
	CPacket& operator >> (short &value);
	CPacket& operator >> (unsigned int &value);
	CPacket& operator >> (int &value);
	CPacket& operator >> (long &value);
	CPacket& operator >> (float &value);
	CPacket& operator >> (double &value);
	CPacket& operator >> (__int64 &value);
protected:
	char* buffer;
	char* WritePointer;
	char* ReadPointer;
	
	int BufferSize;
	int UseSize;
private:
	char* EndPointer;
};

