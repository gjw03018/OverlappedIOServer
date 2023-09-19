#pragma once
#include "CPacket.h"
#include "Define.h"

class MPacket : public CPacket
{
public:
	MPacket();
	~MPacket();
	void SetHeader(Header& header);
	virtual void Clear();

private:
	char* HeaderPtr;

};


