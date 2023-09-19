//#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm.lib")
#include <time.h>
#include "OverlapNetwork.h"
#include <process.h>
//#include <WinSock2.h>
//#include <stdio.h>
#include <unordered_map>
#include <conio.h>
#include "MsgProc.h"
#include <WS2tcpip.h>
//#include "CPacket.h"


HANDLE CICP;

SOCKET listen_sock;



//전역 자료구조 lock
SRWLOCK g_SessionLock;
CRITICAL_SECTION g_DisconnectLock;
CRITICAL_SECTION g_PlayerLock;
CRITICAL_SECTION g_LogicLock;
__int64 SessionID;
//std::unordered_map<__int64, Session*> SessionMap;
std::unordered_map<__int64, H_Session*> SessionMap;
//Player Map
std::unordered_map<__int64, H_Player*> PlayerMap;
std::list<__int64> DisconnectionList;


const int CreateThreadNum = 8;
const int RunnigThreadNum = 6;

//AccepThread
DWORD g_JobQueueIndex;

//UpdateThread 변수
JobQueue ThreadJobQueue;
HANDLE g_ExitThreadEvent;
HANDLE g_UpdateThreadEvent;

//Thread HANDLE
HANDLE gh_Accept;
HANDLE gh_Worker[CreateThreadNum];
HANDLE gh_Update;
HANDLE gh_Logic;
HANDLE gh_Monitor;
HANDLE gh_KeyProcess;

bool g_Monitor = false;



int D_Cnt;

int PlayerDisconnectCnt;
int SessionDisconnectCnt;

int Init()
{
	timeBeginPeriod(1);
	InitializeSRWLock(&g_SessionLock);
	InitializeCriticalSection(&g_PlayerLock);
	InitializeCriticalSection(&g_DisconnectLock);
	InitializeCriticalSection(&g_LogicLock);
	SetSector();

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return WSAGetLastError();
	}

	CICP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, RunnigThreadNum);
	if (CICP == NULL)
		return WSAGetLastError();

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);

	bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

	listen(listen_sock, SOMAXCONN_HINT(2000));
	char buf;
	int optval = 0;
	int optlen = sizeof(optval);
	int tmep = setsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
	linger _linger;
	_linger.l_onoff = 1;
	_linger.l_linger = 0;
	setsockopt(listen_sock, SOL_SOCKET, SO_LINGER, (char*)&_linger, sizeof(linger));
	int nValue = 1;
	setsockopt(listen_sock, SOL_SOCKET, TCP_NODELAY, (char*)&nValue, sizeof(nValue));
	


	HANDLE hTread;

	gh_Accept = (HANDLE)_beginthreadex(NULL, 0, AceeptThread, 0, 0, NULL);

	for(int i = 0; i < CreateThreadNum; i++)
	{
		gh_Worker[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, (HANDLE)CICP, 0, NULL);
	}


	gh_Update = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, 0, 0, NULL);
	g_UpdateThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	gh_Logic = (HANDLE)_beginthreadex(NULL, 0, LogicThread, 0, 0, NULL);
	g_ExitThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	gh_Monitor = (HANDLE)_beginthreadex(NULL, 0, Monitor, 0, 0, NULL);
	gh_KeyProcess = (HANDLE)_beginthreadex(NULL, 0, KeyProcess, 0, 0, NULL);
	printf("ServerON\n");
}
void EndServer()
{
	WaitForMultipleObjects(CreateThreadNum, gh_Worker, TRUE, INFINITE);

	WaitForSingleObject(gh_Accept, INFINITE);
	WaitForSingleObject(gh_Update, INFINITE);
	WaitForSingleObject(gh_Monitor, INFINITE);
	WaitForSingleObject(gh_KeyProcess, INFINITE);

	for (std::unordered_map<__int64, H_Session*>::iterator iter = SessionMap.begin();
		iter != SessionMap.end(); iter++)
	{
		H_Session* hSession = iter->second;
		Session* pSession = hSession->pSession;
		SessionMap.erase(iter->first);

		closesocket(pSession->sock);
		delete pSession->SendQ;
		delete pSession->RecvQ;
		delete pSession;
		delete hSession;
	}
	DeleteCriticalSection(&g_PlayerLock);
	DeleteCriticalSection(&g_DisconnectLock);
	DeleteCriticalSection(&g_LogicLock);
	OnEndServer();
	printf("=== End Server ===\n");
}
void OnEndServer()
{
	for (std::unordered_map<__int64, H_Player*>::iterator iter = PlayerMap.begin();
		iter != PlayerMap.end(); iter++)
	{
		H_Player* hPlayer = iter->second;
		Player* pPlayer = hPlayer->pPlayer;
		PlayerMap.erase(iter->first);

		delete pPlayer;
		delete hPlayer;
	}
}
DWORD AceeptCnt = 0;
bool b_AceeptCnt = true;
SOCKADDR_IN clientaddr;
int addrlen;
unsigned __stdcall AceeptThread(void*)
{
	int retval;
	
	printf("Aceept Thread Start\n");
	SOCKET client_sock;
	
	while (1)
	{
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		//DWORD start = timeGetTime();
		InterlockedIncrement(&AceeptCnt);
		if (client_sock == INVALID_SOCKET)
		{
			retval = GetLastError();
			printf("Accept Error %d\n", retval);
			break;
		}
		//printf(" -- socket %d\n", client_sock);
		OnClientJoin(++SessionID, client_sock);
		//DWORD end = timeGetTime();
		//printf("===Accept Time : %d===\n", end - start);

	}
	printf("Accept Thread End\n");
	return 0;
}

unsigned __stdcall WorkerThread(void* arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;
	printf("Worker Thread Start\n");
	Session* pSession = nullptr;
	DWORD transfrerred;
	OVERLAPPED* overlapped = new OVERLAPPED;
	OVERLAPPED* localoverlapped = overlapped;
	ZeroMemory(localoverlapped, sizeof(OVERLAPPED));
	while (1)
	{
		pSession = nullptr;
		transfrerred = 0;
		overlapped = localoverlapped;
		retval = GetQueuedCompletionStatus(CICP, &transfrerred, (PULONG_PTR)&pSession, &overlapped, INFINITE);
		//postqueuecompletionstatuts
		if (pSession == nullptr && transfrerred == 0 && overlapped == nullptr)
		{
			PostQueuedCompletionStatus(CICP, NULL, NULL, NULL);
			break;
		}
		if (retval == 0 || transfrerred == 0)
		{
			int ret = WSAGetLastError();
			if(ret != 64 && ret != 1236 && ret != 995 && ret != 997)
				printf("WorkerThread GQCS Error %d\n", WSAGetLastError());
			/*
			64		지정된 네트워크 이름을 더 이상 사용할수 없습니다
			995		스레드 종료 또는 애플리케이션 요청으로 인해 I/O 작업이 중단
			997		중첩된 I/O 진행 중 입니다
			1236	로컬시스템에 의해 네트워크 연결이 중단되었습니다
			*/
			goto Decrement;
		}
		if (overlapped == &pSession->RecvOverlap)
		{
			//printf("Recv Byte %d\n", transfrerred);
			pSession->lastRecvTime = timeGetTime();
			pSession->RecvQ->MoveWritePointer(transfrerred);
			Header header;
			stJob* st_Job = new stJob;
			while (1)
			{
				int size = pSession->RecvQ->GetUseSize();
				if (size < sizeof(Header))
					break;
				pSession->RecvQ->Peek((char*)&header, 2);
				if (size - sizeof(Header) < header.len)
					break;
				
				pSession->RecvQ->MoveReadPointer(sizeof(Header));

				MPacket* mPacket = new MPacket;
				pSession->RecvQ->Peek(mPacket->GetWriteBuffPtr(), header.len);
				pSession->RecvQ->MoveReadPointer(header.len);
				mPacket->MoveReadPos(sizeof(Header));
				mPacket->MoveWritePos(header.len);
				InsertJob(st_Job, mPacket);

			}
			st_Job->SessionID = pSession->ID;
			ThreadJobQueue.JobEnQueue(st_Job);
			SetEvent(g_UpdateThreadEvent);

			retval = RecvPost(pSession->ID);
		}
		else if (overlapped == &pSession->SendOverlap)
		{
			//printf("Send Byte %d\n", transfrerred);

			pSession->SendQ->MoveReadPointer(transfrerred);
			InterlockedExchange((DWORD*)&pSession->SendFlag, FALSE);

			retval = SendPost(pSession->ID);

		}
		
		else
		{
			printf("AnOther GQCS\n");

		}
		Decrement:
		if (InterlockedDecrement((DWORD*)&pSession->IOCnt) == 0)
		{
			ReleaseSession(pSession->ID);
		}

	}
	printf("End Worker Thread\n");
	return 0;
}
int LogicCnt;
int JobQCnt;
DWORD s_UpdateTime;
DWORD e_UpdateTime;
DWORD avg_UpdateTime;
int c_UpdateTime;
unsigned __stdcall UpdateThread(void*)
{
	int ret;
	HANDLE hEvent [2] = { g_ExitThreadEvent , g_UpdateThreadEvent};
	DWORD UpdateLogic = 0;
	while (1)
	{
		ret = WaitForMultipleObjects(2, hEvent, FALSE, INFINITE);
		
		if (ret == WAIT_OBJECT_0)
			break;

		s_UpdateTime = timeGetTime();
		EnterCriticalSection(&g_LogicLock);
			JobQCnt++;
			stJob* cJob = ThreadJobQueue.JobDeQueue();
			while (1)
			{
				if (cJob == nullptr)
					break;
				JobNode* jNode = cJob->head.next;
				AcquireSRWLockShared(&g_SessionLock);
				H_Session* hSession = FindH_Session(cJob->SessionID);
				ReleaseSRWLockShared(&g_SessionLock);

				if (hSession == nullptr)
				{
					//해당 Session 을 찾을수 없의면 메시지를 전부 폐기 처리
					while (1)
					{
						if (jNode == &cJob->tail)
							break;
						JobNode* preNode = jNode;
						jNode = jNode->next;
						delete preNode->Job;
						delete preNode;
					}
					delete cJob;
					cJob = ThreadJobQueue.JobDeQueue();
					continue;
				}

				if (!hSession->pSession->FirstJoin)
				{
					
					EnterCriticalSection(&g_PlayerLock);
					H_Player* hPlayer = FindH_Player(cJob->SessionID);
					if (hPlayer == nullptr)
					{
						LeaveCriticalSection(&g_PlayerLock);
						continue;
					}
					EnterCriticalSection(&hPlayer->cs);
					Player* pPlayer = hPlayer->pPlayer;
					LeaveCriticalSection(&g_PlayerLock);
					
					//접속시 메시지 전송
					Header header;
					MPacket cPacket;
					//나에게 캐릭터 생성정보
					{
						mPacketProc_CreateMyCharacter(&header, cPacket, pPlayer->PlayerID, pPlayer->Dir, pPlayer->X, pPlayer->Y, pPlayer->hp);

						SendUnitCast(cJob->SessionID, cPacket);
						//printf("CreateMyCharacter %d Byte\n", cPacket.GetDataSize());
					}
					//남에게 나의 캐릭터 생성정보
					{
						cPacket.Clear();
						mPacketProc_CreateOtherCharacter(&header, cPacket, pPlayer->PlayerID, pPlayer->Dir, pPlayer->X, pPlayer->Y, pPlayer->hp);

						SendBroadCast(cJob->SessionID, cPacket);
						//printf("Other to My %d Byte\n", cPacket.GetDataSize());
					}
					//나에게 남의 캐릭터 생성정보
					{

						st_SectorAround* around = &SectorAround[pPlayer->Sector_Y][pPlayer->Sector_X];

						for (int Cnt = 0;
							Cnt < around->Cnt;
							Cnt++)
						{
							std::list<Player*>* list = &SectorList[around->Around[Cnt].Y][around->Around[Cnt].X];
							for (std::list<Player*>::iterator iter = list->begin();
								iter != list->end();
								++iter)
							{
								if ((*iter) == pPlayer)
									continue;
								cPacket.Clear();
								mPacketProc_CreateOtherCharacter(&header, cPacket, (*iter)->PlayerID, (*iter)->Dir, (*iter)->X, (*iter)->Y, (*iter)->hp);

								SendUnitCast(cJob->SessionID, cPacket);
								if ((*iter)->walking == true)
								{
									cPacket.Clear();
									mPacketProc_MoveStart(&header, cPacket, (*iter)->PlayerID, (*iter)->Dir, (*iter)->X, (*iter)->Y);

									SendUnitCast(cJob->SessionID, cPacket);

									//printf("My to Other Action %d Byte\n", cPacket.GetDataSize());
								}
							}
						}
					}
					InterlockedExchange((DWORD*)&hSession->pSession->FirstJoin, true);
					LeaveCriticalSection(&hPlayer->cs);
				}

				//정상 메시지 처리
				while (1)
				{
					if (jNode == &cJob->tail)
						break;
					OnRecv(cJob->SessionID, jNode->Job);

					JobNode* preNode = jNode;
					jNode = jNode->next;
					delete preNode->Job;
					delete preNode;
				}
				int retval = SendPost(cJob->SessionID);
				delete cJob;
				cJob = ThreadJobQueue.JobDeQueue();
			}
		
		
			LeaveCriticalSection(&g_LogicLock);
			e_UpdateTime = timeGetTime();
			DWORD delta = e_UpdateTime - s_UpdateTime;
			if (avg_UpdateTime < delta)
			{
				avg_UpdateTime = delta;
			}

	}

	//Update Thread 정리 구간
	stJob* cJob = ThreadJobQueue.JobDeQueue();
	while (1)
	{
		if (cJob == nullptr)
			break;
		JobNode* jNode = cJob->head.next;
		while (1)
		{
			if (jNode == &cJob->tail)
				break;
			JobNode* preNode = jNode;
			jNode = jNode->next;
			delete preNode->Job;
			delete preNode;
		}
		delete cJob;
		cJob = ThreadJobQueue.JobDeQueue();
	}
	
	printf("Update Thread End\n");
	return 0;
}
DWORD s_LogicTime;
DWORD e_LogicTime;
DWORD avg_LogicTime;
int LogicDeltaTime;
unsigned __stdcall LogicThread(void*)
{
	HANDLE hEvent = g_ExitThreadEvent;
	DWORD UpdateLogic = 0;
	int ret;
	while (1)
	{
		ret = WaitForSingleObject(hEvent, 20);
		if (ret == WAIT_TIMEOUT)
		{
			EnterCriticalSection(&g_LogicLock);
			
			DWORD currentTime = timeGetTime();
		FixedUpdate:
			s_LogicTime = timeGetTime();
			LogicCnt++;
			EnterCriticalSection(&g_PlayerLock);

			
			for (std::unordered_map<__int64, H_Player*>::iterator iter = PlayerMap.begin();
				iter != PlayerMap.end();
				++iter)
			{
				int X, Y;
				H_Player* hPlayer = (*iter).second;
				if (hPlayer == nullptr)
					continue;
				Player* pPlayer = hPlayer->pPlayer;
				if (pPlayer->hp <= 0)
				{
					AddDisconnectList(pPlayer->PlayerID);
					continue;
				}

				AcquireSRWLockShared(&g_SessionLock);
				H_Session* pSession = FindH_Session(pPlayer->PlayerID);
				if (pSession != nullptr)
				{
					AcquireSRWLockShared(&pSession->srw);
					if (abs(int(currentTime - pSession->pSession->lastRecvTime)) >= dfNETWORK_PACKET_RECV_TIMEOUT)
					{

						D_Cnt++;
						AddDisconnectList(pPlayer->PlayerID);
						ReleaseSRWLockShared(&pSession->srw);
						ReleaseSRWLockShared(&g_SessionLock);
						continue;
					}
					ReleaseSRWLockShared(&pSession->srw);
				}
				ReleaseSRWLockShared(&g_SessionLock);

				if (pPlayer->walking)
				{
					switch (pPlayer->Action)
					{
					case dfPACKET_MOVE_DIR_LL:
						X = -3;
						Y = 0;
						break;
					case dfPACKET_MOVE_DIR_LU:
						X = -3;
						Y = -2;
						break;
					case dfPACKET_MOVE_DIR_UU:
						X = 0;
						Y = -2;
						break;
					case dfPACKET_MOVE_DIR_RU:
						X = 3;
						Y = -2;
						break;
					case dfPACKET_MOVE_DIR_RR:
						X = 3;
						Y = 0;
						break;
					case dfPACKET_MOVE_DIR_RD:
						X = 3;
						Y = 2;
						break;
					case dfPACKET_MOVE_DIR_DD:
						X = 0;
						Y = 2;
						break;
					case dfPACKET_MOVE_DIR_LD:
						X = -3;
						Y = 2;
						break;
					default:
						X = 0;
						Y = 0;
						break;
					}
					if (pPlayer->X + X >= dfRANGE_MOVE_LEFT && pPlayer->X + X <= dfRANGE_MOVE_RIGHT
						&& pPlayer->Y + Y >= dfRANGE_MOVE_TOP && pPlayer->Y + Y <= dfRANGE_MOVE_BOTTOM)
					{
						pPlayer->X += X;
						pPlayer->Y += Y;
						UpdateSector(pPlayer);
					}
				}
			}
			Dissconnect();
			LeaveCriticalSection(&g_PlayerLock);
			e_LogicTime = timeGetTime();
			//printf("==end-start : %d===\n", end - start);
			int delta = (e_LogicTime - s_LogicTime);
			if (avg_LogicTime <= delta)
				avg_LogicTime = delta;
			if(delta >= 0)
				UpdateLogic += delta;


			LogicDeltaTime += UpdateLogic;

			if (UpdateLogic >= 20)
			{
				UpdateLogic -= 20;
				goto FixedUpdate;
			}
			LeaveCriticalSection(&g_LogicLock);
		}
	}
	printf("Logic Thread End\n");
	return 0;
}

unsigned __stdcall Monitor(void*)
{
	HANDLE h = g_ExitThreadEvent;
	int ret;
	while (1)
	{
		ret = WaitForSingleObject(h, 1000);

		if (ret == WAIT_TIMEOUT)
		{
			if (g_Monitor || g_SyncMsg)
			{
				EnterCriticalSection(&g_PlayerLock);
				printf("Player Cnt : %d\t", (int)PlayerMap.size());
				LeaveCriticalSection(&g_PlayerLock);

				AcquireSRWLockShared(&g_SessionLock);
				printf("Session Cnt : %d\n", (int)SessionMap.size());
				ReleaseSRWLockShared(&g_SessionLock);

				printf("Accept Cnt : %d\n", (int)AceeptCnt);
				printf("Create Cnt : %d\n", CreateMyCharacterCnt);
				printf("DissconnectCnt Player : %d, Session : %d\n",PlayerDisconnectCnt, SessionDisconnectCnt);
				printf("LogicCnt : %d, JobQCnt : %d\n", LogicCnt, JobQCnt);
				
				printf("Sync Cnt : %d\n", SyncCnt);
				g_SyncMsg = false;
			}
			InterlockedExchange(&AceeptCnt, 0);
			InterlockedExchange((DWORD*)&CreateMyCharacterCnt, 0);
			InterlockedExchange((DWORD*)&PlayerDisconnectCnt, 0);
			InterlockedExchange((DWORD*)&SessionDisconnectCnt, 0);
			InterlockedExchange((DWORD*)&LogicCnt, 0);
			InterlockedExchange((DWORD*)&JobQCnt, 0);
		}
		else
			break;
	}
	
	printf("Monitor Thread End\n");
	return 0;
}

unsigned __stdcall KeyProcess(void*)
{
	int ret;
	bool bControlMode = false;
	while (1)
	{
		ret = WaitForSingleObject(g_ExitThreadEvent, 20);
		if (ret == WAIT_TIMEOUT)
		{
			if (_kbhit())
			{
				WCHAR ControlKey = _getwch();
				if (L'u' == ControlKey || L'U' == ControlKey)
				{
					bControlMode = true;
					wprintf(L"UnLock ControlMode\n");
				}
				if (L'l' == ControlKey || L'L' == ControlKey)
				{
					bControlMode = false;
					wprintf(L"Lock ControlMode\n");
				}

				if (bControlMode)
				{
					if (L'm' == ControlKey || L'M' == ControlKey)
					{
						InterlockedExchange((DWORD*)&g_Monitor, !g_Monitor);
					}
					if (L'q' == ControlKey || L'Q' == ControlKey)
					{
						PostQueuedCompletionStatus(CICP, NULL, NULL, NULL);
						closesocket(listen_sock);
						SetEvent(g_ExitThreadEvent);
						break;
					}
				}
			}
		}
	}
	printf("KeyProcess Thread End\n");
	return 0;
}

Session* FindSession(__int64 SessionID)
{
	std::unordered_map<__int64,H_Session*>::iterator iter = SessionMap.find(SessionID);
	if (iter == SessionMap.end())
		return nullptr;

	return iter->second->pSession;
}

H_Session* FindH_Session(__int64 SessionID)
{
	std::unordered_map<__int64, H_Session*>::iterator iter = SessionMap.find(SessionID);
	if (iter == SessionMap.end())
		return nullptr;

	return iter->second;
}

void ReleaseSession(__int64 SessionID)
{
	AcquireSRWLockShared(&g_SessionLock);
	H_Session* hseesion = FindH_Session(SessionID);
	Session* pSession;
	if (hseesion != nullptr)
	{
		pSession = hseesion->pSession;
		if (pSession->FirstJoin == false)
		{
			printf("False Create Character\n");
		}
	}
	ReleaseSRWLockShared(&g_SessionLock);
	OnClientLeave(SessionID);
	//RemoveSession(SessionID);
}

void RemoveSession(__int64 SessionID)
{
	
	//SessionMap
	AcquireSRWLockExclusive(&g_SessionLock);

	H_Session* hSession = FindH_Session(SessionID);
	if (hSession == nullptr)
	{
		ReleaseSRWLockExclusive(&g_SessionLock);

		return;
	}
	Session* pSession = hSession->pSession;

	SessionMap.erase(SessionID);

	AcquireSRWLockExclusive(&hSession->srw);
	ReleaseSRWLockExclusive(&g_SessionLock);
	closesocket(pSession->sock);
	delete pSession->SendQ;
	delete pSession->RecvQ;
	delete pSession;
	ReleaseSRWLockExclusive(&hSession->srw);
	delete hSession;
}

void RemoveSession(__int64 SessionID, int& b)
{
	AcquireSRWLockExclusive(&g_SessionLock);

	H_Session* hSession = FindH_Session(SessionID);
	if (hSession == nullptr)
	{
		ReleaseSRWLockExclusive(&g_SessionLock);
		//b = 0;
		return;
	}
	Session* pSession = hSession->pSession;

	SessionMap.erase(SessionID);

	AcquireSRWLockExclusive(&hSession->srw);
	ReleaseSRWLockExclusive(&g_SessionLock);
	closesocket(pSession->sock);
	delete pSession->SendQ;
	delete pSession->RecvQ;
	delete pSession;
	ReleaseSRWLockExclusive(&hSession->srw);
	delete hSession;
	//b = 1;
}

int RecvPost(__int64 SessionID)
{
	int ret;
	AcquireSRWLockShared(&g_SessionLock);

	H_Session* hSession = FindH_Session(SessionID);
	if (hSession == nullptr)
	{
		ReleaseSRWLockShared(&g_SessionLock);

		return 0;
	}
	Session* pSession = hSession->pSession;
	AcquireSRWLockShared(&hSession->srw);

	ReleaseSRWLockShared(&g_SessionLock);


	WSABUF wsabuf[2];
	wsabuf[0].buf = pSession->RecvQ->GetWritePointer();
	wsabuf[0].len = pSession->RecvQ->GetDirectEnqueueSize();
	wsabuf[1].buf = pSession->RecvQ->GetFirstPointer();
	wsabuf[1].len = pSession->RecvQ->GetFreeSize() -  pSession->RecvQ->GetDirectEnqueueSize();
	DWORD recvLen = 0;
	DWORD flags = 0;

	InterlockedIncrement((DWORD*)&pSession->IOCnt);

	ret = WSARecv(pSession->sock, wsabuf, 1, &recvLen, &flags, &pSession->RecvOverlap, NULL);
	ReleaseSRWLockShared(&hSession->srw);

	if (ret == SOCKET_ERROR)
	{
		ret = WSAGetLastError();
		if (ret != WSA_IO_PENDING)
		{
			if(ret != 10054)
			{
				printf("Recv WSARecv Error %d\n", ret);
			}
			if (InterlockedDecrement((DWORD*)&pSession->IOCnt) == 0)
			{
				ReleaseSession(pSession->ID);
			}
		}
		else
		{
			//printf("%d Recv IO_PENDING\n", (int)pSession->sock);
		}
	}
	

	return ret;
}

int SendPost(__int64 SessionID)
{
	int ret;
	AcquireSRWLockShared(&g_SessionLock);

	H_Session* hSession = FindH_Session(SessionID);
	if (hSession == nullptr)
	{
		ReleaseSRWLockShared(&g_SessionLock);
		return 0;
	}
	Session* pSession = hSession->pSession;
	AcquireSRWLockShared(&hSession->srw);
	ReleaseSRWLockShared(&g_SessionLock);
	


	if (InterlockedExchange((DWORD*)&pSession->SendFlag, TRUE) == TRUE)
	{
		ReleaseSRWLockShared(&hSession->srw);
		return 0;
	}
	if (pSession->SendQ->GetUseSize() <= 0)
	{
		ReleaseSRWLockShared(&hSession->srw);
		InterlockedExchange((DWORD*)&pSession->SendFlag, FALSE);
		return 0;
	}
	InterlockedIncrement((DWORD*)& pSession->IOCnt);
	
	WSABUF wsabuf2[2];
	DWORD recvLen2 = 0;
	DWORD flags2 = 0;
	DWORD Cnt = 1;
	wsabuf2[0].buf = pSession->SendQ->GetReadPointer();
	wsabuf2[0].len = pSession->SendQ->GetDirectDequeueSize();
	if (pSession->SendQ->GetUseSize() - pSession->SendQ->GetDirectDequeueSize() > 0)
	{
		wsabuf2[1].buf = pSession->SendQ->GetFirstPointer();
		wsabuf2[1].len = pSession->SendQ->GetUseSize() - pSession->SendQ->GetDirectDequeueSize();
		Cnt = 2;
	}
	ret = WSASend(pSession->sock, wsabuf2, Cnt, &recvLen2, flags2, &pSession->SendOverlap, NULL);
	ReleaseSRWLockShared(&hSession->srw);
	if (ret == SOCKET_ERROR)
	{
		ret = WSAGetLastError();
		if (ret != WSA_IO_PENDING)
		{
			if (ret != 10054)
			{
				printf("Send WSARecv Error %d\n", ret);
			}
			//printf("sock : %d ,%d : IOCnt \n", pSession->sock, pSession->IOCnt);
			if (InterlockedDecrement((DWORD*)&hSession->pSession->IOCnt) == 0)
			{
				ReleaseSession(SessionID);
			}
		}
		else
		{
			//printf("%d Send IO_PENDING\n", (int)pSession->sock);
		}
	}
	return ret;
}

int SendPacket(__int64 SessionID, MPacket* mPacket)
{
	AcquireSRWLockShared(&g_SessionLock);
	H_Session* hSession = FindH_Session(SessionID);
	if (hSession == nullptr)
	{
		ReleaseSRWLockShared(&g_SessionLock);
		return 0;
	}
	Session* pSession = hSession->pSession;
	AcquireSRWLockShared(&hSession->srw);

	ReleaseSRWLockShared(&g_SessionLock);

	/*Header header;
	dfheader.len = 8;
	mPacket->SetHeader(header);*/
	int ret = pSession->SendQ->Enqueue(mPacket->GetReadBuffPtr(), mPacket->GetDataSize());
	ReleaseSRWLockShared(&hSession->srw);
	
	int retval = SendPost(SessionID);
	
	

	return ret;
}
int num = 0;
void OnClientJoin(__int64 SessionID, DWORD sock)
{
	HANDLE IOretval;
	int retval;

	H_Session* phSession = new H_Session;
	InitializeSRWLock(&phSession->srw);
	Session* pSession = new Session;
	

	pSession->ID = SessionID;
	pSession->sock = sock;
	pSession->SendQ = new RingQueue;
	pSession->RecvQ = new RingQueue;
	pSession->RecvOverlap = { 0, };
	pSession->SendOverlap = { 0, };
	pSession->IOCnt = 1;
	pSession->SendFlag = false;
	pSession->lastRecvTime = timeGetTime();
	pSession->FirstJoin = false;

	inet_ntop(AF_INET, &clientaddr.sin_addr.S_un.S_addr, pSession->IP, 16);
	pSession->Port = ntohs(clientaddr.sin_port);
	
	phSession->pSession = pSession;

	//IOCP 등록
	IOretval = CreateIoCompletionPort((HANDLE)sock, CICP, (ULONG_PTR)pSession, 0);
	int IOError = WSAGetLastError();
	if (IOretval == NULL)
	{
		printf("Aceept CICP Error %d\n", GetLastError());
	}

	H_Player* phPlayer = new H_Player;
	InitializeCriticalSection(&phPlayer->cs);
	Player* pPlayer = new Player;

	pPlayer->PlayerID = pSession->ID;
	pPlayer->hp = 100;
	pPlayer->Dir = dfPACKET_MOVE_DIR_LL;
	pPlayer->walking = false;
	pPlayer->X = (rand() % (dfRANGE_MOVE_RIGHT));
	pPlayer->Y = (rand() % (dfRANGE_MOVE_BOTTOM));
	pPlayer->Sector_X = pPlayer->X / SECTORSIZE;
	pPlayer->Sector_Y = pPlayer->Y / SECTORSIZE;
	
	
	phPlayer->pPlayer = pPlayer;

	AcquireSRWLockExclusive(&g_SessionLock);
	SessionMap.insert(std::make_pair(SessionID, phSession));
	ReleaseSRWLockExclusive(&g_SessionLock);

	EnterCriticalSection(&g_PlayerLock);
	PlayerMap.insert(std::make_pair(pPlayer->PlayerID, phPlayer));
	LeaveCriticalSection(&g_PlayerLock);

	AddSectorPlayer(pPlayer);
	
	
	


	//Recv
	
	//printf("Success Client %d\n", client_sock);

	WSABUF wsabuf;
	wsabuf.buf = pSession->RecvQ->GetWritePointer();
	wsabuf.len = pSession->RecvQ->GetDirectEnqueueSize();
	DWORD recvLen = 0;
	DWORD flags = 0;
	retval = WSARecv(sock, &wsabuf, 1, &recvLen, &flags, &pSession->RecvOverlap, NULL);
	if (retval == SOCKET_ERROR)
	{
		retval = WSAGetLastError();
		if (retval != WSA_IO_PENDING)
		{
			num++;
			int iocnt = InterlockedDecrement(&pSession->IOCnt);
			if (iocnt == 0)
				ReleaseSession(pSession->ID);
			printf("Aceept WSARecv Error %d -- %d -- %d -- %d\n", retval, num, iocnt, pSession->SendFlag);
		}
	}
	stJob* st_Job = new stJob;
	st_Job->SessionID = pSession->ID;
	ThreadJobQueue.JobEnQueue(st_Job);
	SetEvent(g_UpdateThreadEvent);
}

void OnRecv(__int64 SessionID, MPacket* cPacket)
{
	unsigned short byType;
	*cPacket >> byType;
	switch (byType)
	{
	case dfPACKET_CS_MOVE_START:
	{
		PacketProc_MoveStart(SessionID, *cPacket);
	}
	break;
	case dfPACKET_CS_MOVE_STOP:
	{
		PacketProc_MoveStop(SessionID, *cPacket);
	}
	break;
	case dfPACKET_CS_ATTACK1:
	{

		PacketProc_Attack1(SessionID, *cPacket);
	}
	break;
	case dfPACKET_CS_ATTACK2:
	{
		PacketProc_Attack2(SessionID, *cPacket);
	}
	break;
	case dfPACKET_CS_ATTACK3:
	{
		PacketProc_Attack3(SessionID, *cPacket);
	}
	break;
	case dfPACKET_CS_ECHO:
	{
		PacketProc_Echo(SessionID, *cPacket);
	}
	break;
	default:
		printf("Default Msg\n");
		break;
	}
}




H_Player* FindH_Player(__int64 SessionID)
{
	std::unordered_map<__int64, H_Player*>::const_iterator iter = PlayerMap.find(SessionID);
	if (iter == PlayerMap.end())
		return nullptr;
	return iter->second;
}

void SendUnitCast(__int64 SessionID, MPacket& cPacket)
{
	SendPacket(SessionID, &cPacket);
}

void SendBroadCast(__int64 SessionID, MPacket& cPacket, bool Sendme)
{
	EnterCriticalSection(&g_PlayerLock);
	H_Player* hPlayer = FindH_Player(SessionID);
	if (hPlayer == nullptr)
	{
		LeaveCriticalSection(&g_PlayerLock);
		return;
	}
	EnterCriticalSection(&hPlayer->cs);
	Player* pPlayer = hPlayer->pPlayer;


	int MsgCnt = 0;
	st_SectorAround* pSector = &SectorAround[pPlayer->Sector_Y][pPlayer->Sector_X];
	
	if (Sendme)
	{
		for (int i = 0; i < pSector->Cnt; i++)
		{
			std::list<Player*>* playerList = &SectorList[pSector->Around[i].Y][pSector->Around[i].X];

			for (std::list<Player*>::iterator iter = playerList->begin();
				iter != playerList->end();
				++iter)
			{
				SendUnitCast((*iter)->PlayerID, cPacket);
				MsgCnt++;
			}
			MsgCnt = 0;
		}
		LeaveCriticalSection(&g_PlayerLock);
		LeaveCriticalSection(&hPlayer->cs);
		return;
	}
	//_LOG(0, L"\n");
	for (int i = 0; i < pSector->Cnt; i++)
	{
		std::list<Player*>* playerList = &SectorList[pSector->Around[i].Y][pSector->Around[i].X];
		for (std::list<Player*>::iterator iter = playerList->begin();
			iter != playerList->end();
			++iter)
		{
			if ((*iter)->PlayerID == SessionID)
				continue;
			SendUnitCast((*iter)->PlayerID, cPacket);
			MsgCnt++;
		}
		MsgCnt = 0;
	}
	//_LOG(0, L"\n");
	LeaveCriticalSection(&g_PlayerLock);
	LeaveCriticalSection(&hPlayer->cs);
}

void SendUnitSector(__int64 SessionID, MPacket& cPacket, SectorPos* pSector)
{
	std::list<Player*>* list = &SectorList[pSector->Y][pSector->X];
	std::list<Player*>::iterator iter;
	EnterCriticalSection(&g_PlayerLock);
	for (iter = list->begin(); iter != list->end(); ++iter)
	{
		if ((*iter)->PlayerID == SessionID)
			continue;
		SendUnitCast((*iter)->PlayerID, cPacket);
	}
	LeaveCriticalSection(&g_PlayerLock);
}

void AddDisconnectList(__int64 PlayerID)
{

	EnterCriticalSection(&g_DisconnectLock);
	DisconnectionList.push_back(PlayerID);
	LeaveCriticalSection(&g_DisconnectLock);
}

void Dissconnect()
{
	Header header;
	int i = 0;
	int DeleteListSize = DisconnectionList.size();
	if (DeleteListSize > 0)
	{
		EnterCriticalSection(&g_DisconnectLock);
		for (std::list<__int64>::iterator iter = DisconnectionList.begin(); iter != DisconnectionList.end();)
		{
			H_Player* hPlayer = FindH_Player(*iter);
			if (hPlayer == nullptr)
			{
				iter = DisconnectionList.erase(iter);
				continue;
			}
			Player* pPlayer = hPlayer->pPlayer;
			EnterCriticalSection(&hPlayer->cs);
			Header header;
			MPacket cPacket;

			int a = -1;
			RemoveSession(pPlayer->PlayerID, a);
			SessionDisconnectCnt++;
			if(a == 0)
				printf("a = %d\n", a);

			mPacketProc_Delete(&header, cPacket, pPlayer->PlayerID);
			RemoveSectorPlayer(pPlayer);
			SendBroadCast(pPlayer->PlayerID, cPacket);

			iter = DisconnectionList.erase(iter);
			PlayerMap.erase(pPlayer->PlayerID);
			PlayerDisconnectCnt++;
			/*AcquireSRWLockShared(&g_SessionLock);
			H_Session* hSession = FindH_Session(pPlayer->PlayerID);
			if (hSession != nullptr)
			{
				printf("Disconnect Session IOCnt %d-SendFlag %d\n", hSession->pSession->IOCnt, hSession->pSession->SendFlag);
			}
			ReleaseSRWLockShared(&g_SessionLock);*/
			delete pPlayer;
			LeaveCriticalSection(&hPlayer->cs);

			DeleteCriticalSection(&hPlayer->cs);
			delete hPlayer;
			
		}
		DisconnectionList.clear();
		LeaveCriticalSection(&g_DisconnectLock);
	}
}

void OnClientLeave(__int64 SessionID)
{
	AddDisconnectList(SessionID);
	/*

	Header header;
	MPacket cPacket;

	mPacketProc_Delete(&header, cPacket, pPlayer->PlayerID);
	SendBroadCast(pPlayer->PlayerID, cPacket);
	LeaveCriticalSection(&hPlayer->cs);*/

}

void OnClientLeaveOnDisconnect(__int64 SessionID)
{
	H_Player* hPlayer = FindH_Player(SessionID);
	if (hPlayer == nullptr)
	{
		return;
	}
	Player* pPlayer = hPlayer->pPlayer;

	PlayerMap.erase(pPlayer->PlayerID);
	RemoveSectorPlayer(pPlayer);

	Header header;
	MPacket cPacket;

	mPacketProc_Delete(&header, cPacket, pPlayer->PlayerID);
	SendBroadCast(pPlayer->PlayerID, cPacket);

	delete pPlayer;
	LeaveCriticalSection(&hPlayer->cs);

	DeleteCriticalSection(&hPlayer->cs);
	delete hPlayer;

}
