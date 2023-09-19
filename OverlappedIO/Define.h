#pragma once
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include "RingQueue.h"
#include <float.h>

#ifndef __PACKET_DEFINE__
#define __PACKET_DEFINE__

#define SERVERPORT 20000
#define dfNETWORK_PACKET_RECV_TIMEOUT	30000
//---------------------------------------------------------------
// ��Ŷ���.
//
//---------------------------------------------------------------
/*
	BYTE	byCode;			// ��Ŷ�ڵ� 0x89 ����.
	BYTE	bySize;			// ��Ŷ ������.
	BYTE	byType;			// ��ŶŸ��.
*/

struct Session
{
	__int64 ID;
	SOCKET sock;
	OVERLAPPED SendOverlap;
	OVERLAPPED RecvOverlap;
	RingQueue* SendQ;
	RingQueue* RecvQ;
	DWORD MyJobQ;
	DWORD IOCnt = 0;
	BOOL SendFlag;
	DWORD UseFlag = FALSE;
	BOOL WaitforRecv = false;
	DWORD lastRecvTime;
	BOOL FirstJoin = false;
	char IP[16];
	int Port;
};

struct Player
{
	__int64 PlayerID;
	unsigned short X, Y;
	unsigned short Sector_X, Sector_Y;
	char hp;
	char Action;
	bool walking;
	char Dir;
	DWORD lastRecvTime;
	
};

struct H_Session
{
	SRWLOCK srw;
	Session* pSession;
};
struct H_Player
{
	CRITICAL_SECTION cs;
	Player* pPlayer;
};

struct PayLoad
{
	__int64 Data;
};


#pragma pack(1)

struct Header
{
	unsigned short len;
};

#define	dfPACKET_SC_CREATE_MY_CHARACTER			0
//---------------------------------------------------------------
// Ŭ���̾�Ʈ �ڽ��� ĳ���� �Ҵ�		Server -> Client
//
// ������ ���ӽ� ���ʷ� �ްԵǴ� ��Ŷ���� �ڽ��� �Ҵ���� ID ��
// �ڽ��� ���� ��ġ, HP �� �ް� �ȴ�. (ó���� �ѹ� �ް� ��)
// 
// �� ��Ŷ�� ������ �ڽ��� ID,X,Y,HP �� �����ϰ� ĳ���͸� �������Ѿ� �Ѵ�.
//
//	4	-	ID
//	1	-	Direction	(LL / RR)
//	2	-	X
//	2	-	Y
//	1	-	HP
//
//---------------------------------------------------------------
struct SC_CREATE_MY_CHARACTER
{
	int ID;
	char Direction;
	unsigned short X;
	unsigned short Y;
	char HP;
};

#define	dfPACKET_SC_CREATE_OTHER_CHARACTER		1
//---------------------------------------------------------------
// �ٸ� Ŭ���̾�Ʈ�� ĳ���� ���� ��Ŷ		Server -> Client
//
// ó�� ������ ���ӽ� �̹� ���ӵǾ� �ִ� ĳ���͵��� ����
// �Ǵ� �����߿� ���ӵ� Ŭ���̾�Ʈ���� ������ ����.
//
//
//	4	-	ID
//	1	-	Direction	(LL / RR)
//	2	-	X
//	2	-	Y
//	1	-	HP
//
//---------------------------------------------------------------
struct SC_CREATE_OTHER_CHARACTER
{
	int ID;
	char Direction;
	unsigned short X;
	unsigned short Y;
	char HP;
};

#define	dfPACKET_SC_DELETE_CHARACTER			2
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ						Server -> Client
//
// ĳ������ �������� �Ǵ� ĳ���Ͱ� �׾����� ���۵�.
//
//	4	-	ID
//
//---------------------------------------------------------------
struct SC_DELETE_CHARACTER
{
	int ID;
};


#define	dfPACKET_CS_MOVE_START					10
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Client -> Server
//
// �ڽ��� ĳ���� �̵����۽� �� ��Ŷ�� ������.
// �̵� �߿��� �� ��Ŷ�� ������ ������, Ű �Է��� ����Ǿ��� ��쿡��
// ������� �Ѵ�.
//
// (���� �̵��� ���� �̵� / ���� �̵��� ���� ���� �̵�... ���)
//
//	1	-	Direction	( ���� ������ �� 8���� ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct CS_MOVE_START
{
	char Direction;
	unsigned short X;
	unsigned short Y;
};
#define dfPACKET_MOVE_DIR_LL					0
#define dfPACKET_MOVE_DIR_LU					1
#define dfPACKET_MOVE_DIR_UU					2
#define dfPACKET_MOVE_DIR_RU					3
#define dfPACKET_MOVE_DIR_RR					4
#define dfPACKET_MOVE_DIR_RD					5
#define dfPACKET_MOVE_DIR_DD					6
#define dfPACKET_MOVE_DIR_LD					7




#define	dfPACKET_SC_MOVE_START					11
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Server -> Client
//
// �ٸ� ������ ĳ���� �̵��� �� ��Ŷ�� �޴´�.
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�� �̵�ó���� ���ֵ��� �Ѵ�.
// 
// ��Ŷ ���� �� �ش� Ű�� ����ؼ� ���������� �����ϰ�
// �ش� �������� ��� �̵��� �ϰ� �־�߸� �Ѵ�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ �� 8���� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct SC_MOVE_START
{
	int ID;
	char Direction;
	unsigned short X;
	unsigned short Y;
};



#define	dfPACKET_CS_MOVE_STOP					12
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Client -> Server
//
// �̵��� Ű���� �Է��� ��� �����Ǿ��� ��, �� ��Ŷ�� ������ �����ش�.
// �̵��� ���� ��ȯ�ÿ��� ��ž�� ������ �ʴ´�.
//
//	1	-	Direction	( ���� ������ �� ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct CS_MOVE_STOP
{
	char Direction;
	unsigned short X;
	unsigned short Y;
};

#define	dfPACKET_SC_MOVE_STOP					13
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Server -> Client
//
// ID �� �ش��ϴ� ĳ���Ͱ� �̵��� ������̹Ƿ� 
// ĳ���͸� ã�Ƽ� �����, ��ǥ�� �Է����ְ� ���ߵ��� ó���Ѵ�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct SC_MOVE_STOP
{
	int ID;
	char Direction;
	unsigned short X;
	unsigned short Y;
};


#define	dfPACKET_CS_ATTACK1						20
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Client -> Server
//
// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
//
// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
//
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y	
//
//---------------------------------------------------------------
struct CS_ATTACK1
{
	char Direction;
	unsigned short X;
	unsigned short Y;
};
#define	dfPACKET_SC_ATTACK1						21
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Server -> Client
//
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����1�� �������� �׼��� �����ش�.
// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct SC_ATTACK1
{
	int ID;
	char Direction;
	unsigned short X;
	unsigned short Y;
};


#define	dfPACKET_CS_ATTACK2						22
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Client -> Server
//
// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
//
// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
//
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct CS_ATTACK2
{
	char Direction;
	unsigned short X;
	unsigned short Y;
};

#define	dfPACKET_SC_ATTACK2						23
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Server -> Client
//
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����2�� �������� �׼��� �����ش�.
// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct SC_ATTACK2
{
	int ID;
	char Direction;
	unsigned short X;
	unsigned short Y;
};

#define	dfPACKET_CS_ATTACK3						24
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Client -> Server
//
// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
//
// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
//
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct CS_ATTACK3
{
	char Direction;
	unsigned short X;
	unsigned short Y;
};

#define	dfPACKET_SC_ATTACK3						25
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Server -> Client
//
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����3�� �������� �׼��� �����ش�.
// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct SC_ATTACK3
{
	int ID;
	char Direction;
	unsigned short X;
	unsigned short Y;
};




#define	dfPACKET_SC_DAMAGE						30
//---------------------------------------------------------------
// ĳ���� ������ ��Ŷ							Server -> Client
//
// ���ݿ� ���� ĳ������ ������ ����.
//
//	4	-	AttackID	( ������ ID )
//	4	-	DamageID	( ������ ID )
//	1	-	DamageHP	( ������ HP )
//
//---------------------------------------------------------------
struct SC_DAMAGE
{
	int AttackID;
	int DamageID;
	char DamageHP;
};







// ������...
#define	dfPACKET_CS_SYNC						250
//---------------------------------------------------------------
// ����ȭ�� ���� ��Ŷ					Client -> Server
//
//
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------


#define	dfPACKET_SC_SYNC						251
//---------------------------------------------------------------
// ����ȭ�� ���� ��Ŷ					Server -> Client
//
// �����κ��� ����ȭ ��Ŷ�� ������ �ش� ĳ���͸� ã�Ƽ�
// ĳ���� ��ǥ�� �������ش�.
//
//	4	-	ID
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
#define	dfPACKET_CS_ECHO						252
//---------------------------------------------------------------
// Echo �� ��Ŷ					Client -> Server
//
//	4	-	Time
//
//---------------------------------------------------------------

#define	dfPACKET_SC_ECHO						253


#define dfERROR_RANGE		50

#define dfRANGE_MOVE_TOP	0
#define dfRANGE_MOVE_LEFT	0
#define dfRANGE_MOVE_RIGHT	6400
#define dfRANGE_MOVE_BOTTOM	6400

#define dfRANGE_MAX_X	6400
#define dfRANGE_MAX_Y	6400

#define dfATTACK1_RANGE_X		80
#define dfATTACK2_RANGE_X		90
#define dfATTACK3_RANGE_X		100

#define dfATTACK1_RANGE_Y		10
#define dfATTACK2_RANGE_Y		10
#define dfATTACK3_RANGE_Y		20

const int SECTORSIZE = 150;
const int SECTOR_X = dfRANGE_MAX_X / SECTORSIZE + 1;
const int SECTOR_Y = dfRANGE_MAX_Y / SECTORSIZE + 1;

#endif
#pragma pack(0)