#pragma once
#include "MPacket.h"
//#include <synchapi.h>
//#include <Windows.h>
static const int MaxSize = 30000;

struct JobNode
{
	MPacket* Job;
	JobNode* next;
	JobNode* pre;
};

struct stJob
{
	__int64 SessionID;
	JobNode head;
	JobNode tail;

	stJob()
	{
		tail.pre = &head;
		head.next = &tail;
	}
};

class JobQueue
{
public:
	JobQueue();
	JobQueue(DWORD size);
	~JobQueue();

	bool JobEnQueue(stJob* Job);
	stJob* JobDeQueue();
	
private:
	stJob* JobQ[MaxSize];
	int WriteIndex = 0;
	int ReadIndex = 0;
	int Size = 0;
	SRWLOCK srw;
};

void InsertJob(stJob* Job, MPacket* mPacket);