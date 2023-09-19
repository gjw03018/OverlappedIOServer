#pragma once
#include <list>
#include "Define.h"


struct SectorPos
{
	int X;
	int Y;
};
struct st_SectorAround
{
	int Cnt = 0;
	SectorPos Around[9];
};

extern std::list<Player*> SectorList[SECTOR_Y][SECTOR_X];
extern st_SectorAround SectorAround[SECTOR_Y][SECTOR_X];

void UpdateSector(Player* pPlayer);
void AddSectorPlayer(Player* pPlayer);
void RemoveSectorPlayer(Player* pPlayer);

void NetOldSector(Player* pPlayer, st_SectorAround* OldSector, st_SectorAround* NewSector);
void New_Sector(Player* pPlayer, st_SectorAround* OldSector, st_SectorAround* NewSector);

void SetSector();
	

