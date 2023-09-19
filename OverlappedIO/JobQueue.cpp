
#include "JobQueue.h"
#include <stdio.h>
JobQueue::JobQueue()
{
	InitializeSRWLock(&srw);
}

JobQueue::JobQueue(DWORD size)
{
}

JobQueue::~JobQueue()
{
}


bool JobQueue::JobEnQueue(stJob* Job)
{
	AcquireSRWLockExclusive(&srw);
	if (Size >= MaxSize)
	{
		ReleaseSRWLockExclusive(&srw);
		printf("FULL JobQ\n");
		return false;
	}

	JobQ[WriteIndex] = Job;
	WriteIndex++;
	Size++;
	if (WriteIndex >= MaxSize)
		WriteIndex = 0;
	
	ReleaseSRWLockExclusive(&srw);

	return true;
}

stJob* JobQueue::JobDeQueue()
{
	AcquireSRWLockExclusive(&srw);
	if (Size <= 0)
	{
		ReleaseSRWLockExclusive(&srw);
		return nullptr;
	}
	int index = ReadIndex;
	
	ReadIndex++;
	Size--;
	if (ReadIndex >= MaxSize)
		ReadIndex = 0;

	ReleaseSRWLockExclusive(&srw);

	return JobQ[index];
}

void InsertJob(stJob* Job, JobNode* Node)
{

}

void InsertJob(stJob* Job, MPacket* mPacket)
{
	JobNode* jNode = new JobNode;
	jNode->Job = mPacket;
	jNode->next = nullptr;

	jNode->pre = Job->tail.pre;
	jNode->next = &Job->tail;

	Job->tail.pre->next = jNode;
	Job->tail.pre = jNode;

}
