#pragma once

//#include "MPacket.h"
#include "JobQueue.h"
#include "CSector.h"

int Init();
void EndServer();
void OnEndServer();
unsigned __stdcall AceeptThread(void*);
unsigned __stdcall WorkerThread(void* arg);
unsigned __stdcall UpdateThread(void*);
unsigned __stdcall LogicThread(void*);
unsigned __stdcall Monitor(void*);
unsigned __stdcall KeyProcess(void*);

Session* FindSession(__int64 SessionID);
H_Session* FindH_Session(__int64 SessionID);
void ReleaseSession(__int64 SessionID);
void RemoveSession(__int64 SessionID);
void RemoveSession(__int64 SessionID,int& b);

int RecvPost(__int64 SessionID);
int SendPost(__int64 SessionID);
int SendPacket(__int64 SessionID, MPacket* mPacket);

void OnClientJoin(__int64 SessionID, DWORD sock);
void OnRecv(__int64 SessionID, MPacket* mPacket);
void OnClientLeave(__int64 SessionID);
void OnClientLeaveOnDisconnect(__int64 SessionID);

H_Player* FindH_Player(__int64 SessionID);

void SendUnitCast(__int64 SessionID, MPacket& cPacket);
void SendBroadCast(__int64 SessionID, MPacket& cPacket, bool Sendme = false);

void SendUnitSector(__int64 SessionID, MPacket& cPacket, SectorPos* pSector);


void AddDisconnectList(__int64 PlayerID);
void Dissconnect();

extern CRITICAL_SECTION g_PlayerLock;