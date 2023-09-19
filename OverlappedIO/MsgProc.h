#pragma once
#include "MPacket.h"
void mPacketProc_CreateMyCharacter(Header* header, MPacket& msg, int ID, char Direction, unsigned short X, unsigned short Y, char HP );
void mPacketProc_CreateOtherCharacter(Header* header, MPacket& msg, int ID, char Direction, unsigned short X, unsigned short Y, char HP);



void mPacketProc_MoveStart(Header* header, MPacket& msg, int ID, char Dir, unsigned short X, unsigned short Y);
void mPacketProc_MoveStop(Header* header, MPacket& msg, int ID, char Dir, unsigned short X, unsigned short Y);
void mPacketProc_Attack1(Header* header, MPacket& msg, int ID, char Dir, unsigned short X, unsigned short Y);
void mPacketProc_Attack2(Header* header, MPacket& msg, int ID, char Dir, unsigned short X, unsigned short Y);
void mPacketProc_Attack3(Header* header, MPacket& msg, int ID, char Dir, unsigned short X, unsigned short Y);
void mPacketProc_Damage(Header* header, MPacket& msg, int AttackID, int DamageID, char hp);
void mPacketProc_Delete(Header* header, MPacket& msg, int ID);
void mPacketProc_Syn(Header* header, MPacket& msg, int ID, unsigned short X, unsigned short Y);

bool PacketProc_MoveStart(__int64 PlayerID, MPacket& msg);
bool PacketProc_MoveStop(__int64 PlayerID, MPacket& msg);
bool PacketProc_Attack1(__int64 PlayerID, MPacket& msg);
bool PacketProc_Attack2(__int64 PlayerID, MPacket& msg);
bool PacketProc_Attack3(__int64 PlayerID, MPacket& msg);
bool PacketProc_Echo(__int64 PlayerID, MPacket& msg);

extern int CreateMyCharacterCnt;
extern int SyncCnt;
extern bool g_SyncMsg;