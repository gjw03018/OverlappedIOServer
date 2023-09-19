#include "MsgProc.h"
#include "OverlapNetwork.h"

int CreateMyCharacterCnt = 0;
bool g_SyncMsg = false;
void mPacketProc_CreateMyCharacter(Header* header, MPacket& msg, int ID, char Direction, unsigned short X, unsigned short Y, char HP )
{
	InterlockedIncrement((DWORD*)&CreateMyCharacterCnt);
	header->len = sizeof(ID) + sizeof(X) + sizeof(Y) + sizeof(HP) + sizeof(Direction) + 2;
	msg.SetHeader(*header);
	msg << (unsigned short)dfPACKET_SC_CREATE_MY_CHARACTER << ID << Direction << X << Y << HP;
}

void mPacketProc_CreateOtherCharacter(Header* header, MPacket& msg, int ID, char Direction, unsigned short X, unsigned short Y, char HP)
{
	header->len = sizeof(ID) + sizeof(X) + sizeof(Y) + sizeof(HP) + sizeof(Direction) + sizeof(Header);
	msg.SetHeader(*header);
	msg << (unsigned short)dfPACKET_SC_CREATE_OTHER_CHARACTER << ID << Direction << X << Y << HP;
}
void mPacketProc_MoveStart(Header* header, MPacket& msg, int ID, char Dir, unsigned short X, unsigned short Y)
{
	header->len = sizeof(ID) + sizeof(Dir) + sizeof(X) + sizeof(Y) + 2;
	msg.SetHeader(*header);
	msg << (unsigned short)dfPACKET_SC_MOVE_START << ID << Dir << X << Y;
}
void mPacketProc_MoveStop(Header* header, MPacket& msg, int ID, char Dir, unsigned short X, unsigned short Y)
{
	header->len = sizeof(ID) + sizeof(Dir) + sizeof(X) + sizeof(Y) + 2;
	msg.SetHeader(*header);
	msg << (unsigned short)dfPACKET_SC_MOVE_STOP << ID << Dir << X << Y;
}
void mPacketProc_Attack1(Header* header, MPacket& msg, int ID, char Dir, unsigned short X, unsigned short Y)
{
	header->len = sizeof(ID) + sizeof(Dir) + sizeof(X) + sizeof(Y) + 2;
	msg.SetHeader(*header);
	msg << (unsigned short)dfPACKET_SC_ATTACK1 << ID << Dir << X << Y;
}
void mPacketProc_Attack2(Header* header, MPacket& msg, int ID, char Dir, unsigned short X, unsigned short Y)
{
	header->len = sizeof(ID) + sizeof(Dir) + sizeof(X) + sizeof(Y) + 2;
	msg.SetHeader(*header);
	msg << (unsigned short)dfPACKET_SC_ATTACK2 << ID << Dir << X << Y;
}
void mPacketProc_Attack3(Header* header, MPacket& msg, int ID, char Dir, unsigned short X, unsigned short Y)
{
	header->len = sizeof(ID) + sizeof(Dir) + sizeof(X) + sizeof(Y) + 2;

	msg.SetHeader(*header);
	msg << (unsigned short)dfPACKET_SC_ATTACK3 << ID << Dir << X << Y;
}
void mPacketProc_Damage(Header* header, MPacket& msg, int AttackID, int DamageID, char hp)
{
	header->len = sizeof(AttackID) + sizeof(DamageID) + sizeof(hp) + 2;
	msg.SetHeader(*header);
	msg << (unsigned short)dfPACKET_SC_DAMAGE << AttackID << DamageID << hp;
}
void mPacketProc_Delete(Header* header, MPacket& msg, int ID)
{
	header->len = sizeof(ID) + 2;
	msg.SetHeader(*header);
	msg << (unsigned short)dfPACKET_SC_DELETE_CHARACTER << ID;
}
int SyncCnt;
void mPacketProc_Syn(Header* header, MPacket& msg, int ID, unsigned short X, unsigned short Y)
{
	g_SyncMsg = true;
	SyncCnt++;
	header->len = 8 + 2;
	msg.SetHeader(*header);
	msg << (unsigned short)dfPACKET_SC_SYNC << ID << X << Y;
}

bool PacketProc_MoveStart(__int64 PlayerID, MPacket& msg)
{
	Header header;
	EnterCriticalSection(&g_PlayerLock);
	H_Player* hPlayer = FindH_Player(PlayerID);
	if (hPlayer == nullptr)
	{
		LeaveCriticalSection(&g_PlayerLock);
		return false;
	}
	Player* pPlayer = hPlayer->pPlayer;
	EnterCriticalSection(&hPlayer->cs);

	LeaveCriticalSection(&g_PlayerLock);
	char Dir; unsigned short X; unsigned short Y;

	msg >> Dir >> X >> Y;

	pPlayer->Action = Dir;
	pPlayer->walking = true;
	if (abs(pPlayer->X - X) > dfERROR_RANGE ||
		abs(pPlayer->Y - Y) > dfERROR_RANGE)
	{
		msg.Clear();
		mPacketProc_Syn(&header, msg, pPlayer->PlayerID, X, Y);
		SendUnitCast(pPlayer->PlayerID, msg);
		//printf("START [p%d'%d, p%d'%d] %d %d\n", pPlayer->X, X, pPlayer->Y, Y, pPlayer->Dir, pPlayer->Action);
		//_LOG(2, L"--MoveStart SYNC-- Session ID %d  [P:%d'%d, p:%d'%d] %d %d", pSession->sock, pPlayer->X, X, pPlayer->Y,Y,pPlayer->Dir, pPlayer->Action);
		//printf("%f\n", accrueTime);
	}

	
	switch (Dir)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pPlayer->Dir = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		pPlayer->Dir = dfPACKET_MOVE_DIR_LL;
		break;
	}

	pPlayer->X = X;
	pPlayer->Y = Y;
	LeaveCriticalSection(&hPlayer->cs);

	MPacket cPakcet;
	mPacketProc_MoveStart(&header, cPakcet, pPlayer->PlayerID, Dir, pPlayer->X, pPlayer->Y);
	
	SendBroadCast(PlayerID, cPakcet);
	return true;

}

bool PacketProc_MoveStop(__int64 PlayerID, MPacket& msg)
{
	Header header;
	EnterCriticalSection(&g_PlayerLock);
	H_Player* hPlayer = FindH_Player(PlayerID);
	if (hPlayer == nullptr)
	{
		LeaveCriticalSection(&g_PlayerLock);
		return false;
	}
	Player* pPlayer = hPlayer->pPlayer;
	EnterCriticalSection(&hPlayer->cs);
	

	char Dir; unsigned short X; unsigned short Y;
	msg >> Dir >> X >> Y;

	pPlayer->Action = Dir;
	pPlayer->walking = false;

	if (abs(pPlayer->X - X) > dfERROR_RANGE ||
		abs(pPlayer->Y - Y) > dfERROR_RANGE)
	{
		msg.Clear();
		mPacketProc_Syn(&header, msg, pPlayer->PlayerID, X, Y);
		SendUnitCast(pPlayer->PlayerID, msg);
		/*
		//printf("==Syn==\n");
		//printf("STOP [p%d'%d, p%d'%d] %d %d\n", pPlayer->X, X, pPlayer->Y, Y, pPlayer->Dir, pPlayer->Action);
		//_LOG(2, L"--MoveStop SYNC-- Session ID %d  [%d'%d, %d'%d] %d %d", pSession->sock, pPlayer->X, X, pPlayer->Y, Y, pPlayer->Dir, pPlayer->Action);
		//printf("%f\n", accrueTime);
		*/
	}
	switch (Dir)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pPlayer->Dir = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		pPlayer->Dir = dfPACKET_MOVE_DIR_LL;
		break;
	}

	pPlayer->X = X;
	pPlayer->Y = Y;
	UpdateSector(pPlayer);
	LeaveCriticalSection(&hPlayer->cs);
	LeaveCriticalSection(&g_PlayerLock);
		
	MPacket cPacket;
	mPacketProc_MoveStop(&header, cPacket, pPlayer->PlayerID, Dir, X, Y);
	SendBroadCast(PlayerID, cPacket);
	return true;
}

bool LeftAttackDistance(Player* player, Player* target, int distance_X, int distance_Y)
{
	//공격 범위 체크
	if (target->X <= player->X)
	{
		if ((player->X - target->X) < distance_X && abs(player->Y - target->Y) <= (distance_Y * .5))
		{
			return true;
		}
	}
	return false;
}

bool RightAttackDistance(Player* player, Player* target, int distance_X, int distance_Y)
{
	//공격 범위 체크
	if (player->X <= target->X)
	{
		if ((target->X - player->X) < distance_X && abs(player->Y - target->Y) <= (distance_Y * .5))
		{
			return true;
		}
	}
	return false;
}


bool PacketProc_Attack1(__int64 PlayerID, MPacket& msg)
{
	Header header;
	EnterCriticalSection(&g_PlayerLock);
	H_Player* hPlayer = FindH_Player(PlayerID);
	if (hPlayer == nullptr)
	{
		LeaveCriticalSection(&g_PlayerLock);
		return false;
	}
	Player* pPlayer = hPlayer->pPlayer;
	EnterCriticalSection(&hPlayer->cs);

	LeaveCriticalSection(&g_PlayerLock);
	char Dir; unsigned short X; unsigned short Y;
	msg >> Dir >> X >> Y;
	pPlayer->Dir = Dir;
	pPlayer->X = X;
	pPlayer->Y = Y;
	//다른 플레이어에게 공격 함을 알림
	MPacket cPacket;
	mPacketProc_Attack1(&header, cPacket, pPlayer->PlayerID, pPlayer->Dir, pPlayer->X, pPlayer->Y);

	
	bool (*fp)(Player*, Player*, int, int);

	if (pPlayer->Dir == dfPACKET_MOVE_DIR_LL)
		fp = LeftAttackDistance;
	else
		fp = RightAttackDistance;

	//공격 범위 체크
	std::list<Player*>* PlayerList = &SectorList[pPlayer->Sector_Y][pPlayer->Sector_X];

	LeaveCriticalSection(&hPlayer->cs);
	SendBroadCast(PlayerID, cPacket);

	std::list<Player*>::iterator iter;
	EnterCriticalSection(&g_PlayerLock);
	
	for (iter = PlayerList->begin(); iter != PlayerList->end(); ++iter)
	{
		if ((*iter) != pPlayer)
		{
			if (fp(pPlayer, (*iter), dfATTACK1_RANGE_X, dfATTACK1_RANGE_Y))
			{
				
				(*iter)->hp--;
				cPacket.Clear();
				mPacketProc_Damage(&header, cPacket, pPlayer->PlayerID, (*iter)->PlayerID, (*iter)->hp);
				SendBroadCast(PlayerID, cPacket, true);
			}
		}
	}
	LeaveCriticalSection(&g_PlayerLock);
	return true;
}
bool PacketProc_Attack2(__int64 PlayerID, MPacket& msg)
{
	Header header;
	EnterCriticalSection(&g_PlayerLock);
	H_Player* hPlayer = FindH_Player(PlayerID);
	if (hPlayer == nullptr)
	{
		LeaveCriticalSection(&g_PlayerLock);
		return false;
	}
	Player* pPlayer = hPlayer->pPlayer;
	EnterCriticalSection(&hPlayer->cs);

	LeaveCriticalSection(&g_PlayerLock);
	char Dir; unsigned short X; unsigned short Y;
	msg >> Dir >> X >> Y;
	pPlayer->Dir = Dir;
	pPlayer->X = X;
	pPlayer->Y = Y;

	//다른 플레이어에게 공격 함을 알림
	MPacket cPacket;
	mPacketProc_Attack2(&header, cPacket, pPlayer->PlayerID, pPlayer->Dir, pPlayer->X, pPlayer->Y);


	bool (*fp)(Player*, Player*, int, int);

	if (pPlayer->Dir == dfPACKET_MOVE_DIR_LL)
		fp = LeftAttackDistance;
	else
		fp = RightAttackDistance;

	//공격 범위 체크
	std::list<Player*>* PlayerList = &SectorList[pPlayer->Sector_Y][pPlayer->Sector_X];
	LeaveCriticalSection(&hPlayer->cs);
	SendBroadCast(PlayerID, cPacket);

	std::list<Player*>::iterator iter;
	EnterCriticalSection(&g_PlayerLock);
	
	for (iter = PlayerList->begin(); iter != PlayerList->end(); ++iter)
	{
		if ((*iter) != pPlayer)
		{
			if (fp(pPlayer, (*iter), dfATTACK1_RANGE_X, dfATTACK1_RANGE_Y))
			{
				(*iter)->hp--;
				cPacket.Clear();
				mPacketProc_Damage(&header, cPacket, pPlayer->PlayerID, (*iter)->PlayerID, (*iter)->hp);
				SendBroadCast(PlayerID, cPacket, true);
			}
		}
	}
	LeaveCriticalSection(&g_PlayerLock);
	return true;
}
bool PacketProc_Attack3(__int64 PlayerID, MPacket& msg)
{
	Header header;
	EnterCriticalSection(&g_PlayerLock);
	H_Player* hPlayer = FindH_Player(PlayerID);
	if (hPlayer == nullptr)
	{
		LeaveCriticalSection(&g_PlayerLock);
		return false;
	}
	Player* pPlayer = hPlayer->pPlayer;
	EnterCriticalSection(&hPlayer->cs);

	LeaveCriticalSection(&g_PlayerLock);
	char Dir; unsigned short X; unsigned short Y;
	msg >> Dir >> X >> Y;
	pPlayer->Dir = Dir;
	pPlayer->X = X;
	pPlayer->Y = Y;

	//다른 플레이어에게 공격 함을 알림
	MPacket cPacket;
	mPacketProc_Attack3(&header, cPacket, pPlayer->PlayerID, pPlayer->Dir, pPlayer->X, pPlayer->Y);

	

	bool (*fp)(Player*, Player*, int, int);

	if (pPlayer->Dir == dfPACKET_MOVE_DIR_LL)
		fp = LeftAttackDistance;
	else
		fp = RightAttackDistance;

	//공격 범위 체크
	std::list<Player*>* PlayerList = &SectorList[pPlayer->Sector_Y][pPlayer->Sector_X];

	LeaveCriticalSection(&hPlayer->cs);
	SendBroadCast(PlayerID, cPacket);

	std::list<Player*>::iterator iter;
	EnterCriticalSection(&g_PlayerLock);
	
	for (iter = PlayerList->begin(); iter != PlayerList->end(); ++iter)
	{
		if ((*iter) != pPlayer)
		{
			if (fp(pPlayer, (*iter), dfATTACK1_RANGE_X, dfATTACK1_RANGE_Y))
			{
				(*iter)->hp--;
				cPacket.Clear();
				mPacketProc_Damage(&header, cPacket, pPlayer->PlayerID, (*iter)->PlayerID, (*iter)->hp);
				SendBroadCast(PlayerID, cPacket, true);
			}
		}
	}
	LeaveCriticalSection(&g_PlayerLock);
	return true;
}

bool PacketProc_Echo(__int64 PlayerID, MPacket& msg)
{
	MPacket cPacket;
	Header header;
	header.len = 4 + 2;
	cPacket << (unsigned short)dfPACKET_SC_ECHO;
	cPacket.SetHeader(header);

	cPacket.PutData(msg.GetReadBuffPtr(), 4);

	cPacket.MoveWritePos(4);

	SendUnitCast(PlayerID, cPacket);
	return true;
}
