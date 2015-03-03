/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_DOM_H
#define GAME_SERVER_GAMEMODES_DOM_H
#include <game/server/gamecontroller.h>
#include <game/server/entity.h>

class CGameControllerDOM : public IGameController
{
private:
	vec2 flagPoints[10];
	vec2 flagControlling[10];
	int flagCount;
	int teamWinning;
	int Weapon;
	int gunDelay;
	int gunReload;
	float scoreCounter;
	float broadcastCounter;

public:
	CGameControllerDOM(class CGameContext *pGameServer);

	virtual bool OnEntity(int Index, vec2 Pos);
	virtual void Snap(int SnappingClient);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual void Tick();
};

#endif // dom_h