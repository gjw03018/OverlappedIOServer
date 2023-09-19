#include "CSector.h"
#include "OverlapNetwork.h"
#include "MsgProc.h"

std::list<Player*> SectorList[SECTOR_Y][SECTOR_X];


st_SectorAround SectorAround[SECTOR_Y][SECTOR_X];

void AddSectorPlayer(Player* pPlayer)
{
	SectorList[pPlayer->Sector_Y][pPlayer->Sector_X].push_back(pPlayer);
}

void RemoveSectorPlayer(Player* pPlayer)
{
	int Sector_X = pPlayer->Sector_X;
	int Sector_Y = pPlayer->Sector_Y;
	std::list<Player*>* list = &SectorList[Sector_Y][Sector_X];
	std::list<Player*>::iterator iter;

	for (iter = list->begin(); iter != list->end(); ++iter)
	{
		if (pPlayer->PlayerID == (*iter)->PlayerID)
		{
			list->erase(iter);
			break;
		}
	}
}

void NetOldSector(Player* pPlayer, st_SectorAround* OldSector, st_SectorAround* NewSector)
{
	bool bfind = false;
	//OldSector Packet
	MPacket cPacket2;
	Header header2;
	//Delete Packet
	MPacket cPacket;
	Header header;
	mPacketProc_Delete(&header, cPacket, pPlayer->PlayerID);
	

	for (int oCnt = 0; oCnt < OldSector->Cnt; oCnt++)
	{
		bfind = false;
		for (int nCnt = 0; nCnt < NewSector->Cnt; nCnt++)
		{
			if (OldSector->Around[oCnt].X == NewSector->Around[nCnt].X &&
				OldSector->Around[oCnt].Y == NewSector->Around[nCnt].Y)
			{
				bfind = true;
				break;
			}
		}
		if (!bfind)
		{
			//삭제된곳에 나의 삭제 정보 보내기
			SendUnitSector(-1, cPacket, &OldSector->Around[oCnt]);
			//나한테 지워지는 섹터 정보 보내기
			std::list<Player*>* PlayerList = &SectorList[OldSector->Around[oCnt].Y][OldSector->Around[oCnt].X];
			for (std::list<Player*>::iterator iter = PlayerList->begin();
				iter != PlayerList->end(); ++iter)
			{
				mPacketProc_Delete(&header2, cPacket2, (*iter)->PlayerID);
				
				SendUnitCast(pPlayer->PlayerID, cPacket2);
				cPacket2.Clear();
			}
		}
	}
}

void New_Sector(Player* pPlayer, st_SectorAround* OldSector, st_SectorAround* NewSector)
{
	bool bfind = false;
	
	//Create Packet
	MPacket cPacket;
	Header header;
	//나의 캐릭터 생성 메세지 다른사람들에게
	mPacketProc_CreateOtherCharacter(&header, cPacket, pPlayer->PlayerID, pPlayer->Dir, pPlayer->X, pPlayer->Y, pPlayer->hp);
	MPacket walkPacket;
	Header walkHeader;
	if (pPlayer->walking)
	{
		mPacketProc_MoveStart(&walkHeader, walkPacket, pPlayer->PlayerID, pPlayer->Action, pPlayer->X, pPlayer->Y);
	}

	//NewSector Packet
	MPacket cPacket2;
	Header header2;
	MPacket wPacket2;
	Header wheader2;
	for (int oCnt = 0; oCnt < NewSector->Cnt; oCnt++)
	{
		bfind = false;
		for (int nCnt = 0; nCnt < OldSector->Cnt; nCnt++)
		{
			if (NewSector->Around[oCnt].X == OldSector->Around[nCnt].X &&
				NewSector->Around[oCnt].Y == OldSector->Around[nCnt].Y)
			{
				bfind = true;
				break;
			}
		}
		if (!bfind)
		{
			SendUnitSector(pPlayer->PlayerID, cPacket, &NewSector->Around[oCnt]);
			if (pPlayer->walking)
			{
				SendUnitSector(pPlayer->PlayerID, walkPacket, &NewSector->Around[oCnt]);
			}
			std::list<Player*>* PlayerList = &SectorList[NewSector->Around[oCnt].Y][NewSector->Around[oCnt].X];
			for (std::list<Player*>::iterator iter = PlayerList->begin();
					iter != PlayerList->end(); ++iter)
			{
				if (pPlayer == *iter)
					continue;
				mPacketProc_CreateOtherCharacter(&header2, cPacket2, (*iter)->PlayerID,(*iter)->Dir, (*iter)->X, (*iter)->Y, (*iter)->hp);
				if ((*iter)->walking)
				{
					mPacketProc_MoveStart(&wheader2, wPacket2, (*iter)->PlayerID, (*iter)->Action, (*iter)->X, (*iter)->Y);
				}

				SendUnitCast(pPlayer->PlayerID, cPacket2);
				cPacket2.Clear();
				wPacket2.Clear();
			}
		}
	}
}

void UpdateSector(Player* pPlayer)
{
	int Sector_X = pPlayer->X / SECTORSIZE;
	int Sector_Y = pPlayer->Y / SECTORSIZE;
	//변경이 없다면
	if (Sector_X == pPlayer->Sector_X && Sector_Y == pPlayer->Sector_Y)
		return;
	//변경이 있다면
	int Old_Sector_X = pPlayer->Sector_X;
	int Old_Sector_Y = pPlayer->Sector_Y;
	RemoveSectorPlayer(pPlayer);
	//변경된 섹터
	pPlayer->Sector_X = Sector_X;
	pPlayer->Sector_Y = Sector_Y;
	AddSectorPlayer(pPlayer);
	/*for (int y = 0; y < SECTOR_Y; y++)
	{
		for (int x = 0; x < SECTOR_X; x++)
		{
			if (SectorList[y][x].size() >= 1)
			{
				printf("[%d,%d,%d]", y, x, SectorList[y][x].size());
			}
		}
	}
	printf("\n");*/
	st_SectorAround* OldSector = &SectorAround[Old_Sector_Y][Old_Sector_X];
	st_SectorAround* NewSector = &SectorAround[Sector_Y][Sector_X];
	//st_SectorAround* MySector
	
	
	NetOldSector(pPlayer, OldSector, NewSector);
	//New Packet
	//AddSector Packet
	
	
	New_Sector(pPlayer, OldSector, NewSector);
	
}

void SetSector()
{
	int _Y; int _X;
	for (int iCntY = 0; iCntY < SECTOR_Y; iCntY++)
	{
		for (int iCntX = 0; iCntX < SECTOR_X; iCntX++)
		{

			_Y = iCntY - 1;
			_X = iCntX - 1;
			for (int Y = 0; Y < 3; Y++)
			{
				if (_Y + Y < 0 || _Y + Y >= SECTOR_Y)
					continue;
				for (int X = 0; X < 3; X++)
				{
					if (_X + X < 0 || _X + X >= SECTOR_X)
						continue;
					SectorAround[iCntY][iCntX].Around[SectorAround[iCntY][iCntX].Cnt].X = _X + X;
					SectorAround[iCntY][iCntX].Around[SectorAround[iCntY][iCntX].Cnt].Y = _Y + Y;
					SectorAround[iCntY][iCntX].Cnt++;
				}
			}
		}
	}

}
