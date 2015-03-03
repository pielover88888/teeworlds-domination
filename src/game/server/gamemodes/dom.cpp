/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

// repo @ github.com/shorefire //

#include "dom.h"

#include <game/server/entities/pickup.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <math.h>

CGameControllerDOM::CGameControllerDOM(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// init our control points array
	// flagControlling x = TEAM_RED, y = TEAM_BLUE
	for(int i=0; i<10; i++) {
		flagPoints[i] = vec2(0, 0);
		flagControlling[i] = vec2(0, 0);
	}

	flagCount = 0;
	scoreCounter = Server()->Tick();
	broadcastCounter = Server()->Tick();
	teamWinning = -1;
	Weapon = WEAPON_GUN;

	gunReload = g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Ammoregentime;
	gunDelay = g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Firedelay;

	m_pGameType = "DOM";
	m_GameFlags = GAMEFLAG_TEAMS|GAMEFLAG_FLAGS;
}

bool CGameControllerDOM::OnEntity(int Index, vec2 Pos)
{
	// dont spawn shields/weapons if we are not playing normal domination
	if(str_comp(g_Config.m_SvGametype, "dom") != 0) {
		if(Index != ENTITY_SPAWN && Index != ENTITY_SPAWN_RED && Index != ENTITY_SPAWN_BLUE && 
			Index != ENTITY_FLAGSTAND_RED && Index != ENTITY_FLAGSTAND_BLUE) {
			return false;
		}
	}

	if(IGameController::OnEntity(Index, Pos))
		return true;

	if(Index != ENTITY_FLAGSTAND_RED && Index != ENTITY_FLAGSTAND_BLUE)
		return false;

	// add the control points
	flagPoints[flagCount] = Pos;
	flagCount++;

	return true;
}

void CGameControllerDOM::Tick()
{
	IGameController::Tick();


	// reset flag controlling count
	for(int i=0; i<flagCount+1; i++)
		flagControlling[i] = vec2(0, 0);


	// check to see what team has control of what flags
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(GameServer()->m_apPlayers[i]) {
			for(int flag=0; flag<flagCount+1; flag++) {
				if(!GameServer()->m_apPlayers[i]->GetCharacter())
					break;

				CCharacter* Character = GameServer()->m_apPlayers[i]->GetCharacter();
				vec2 CharPos = Character->m_Pos;
				vec2 PointPos = flagPoints[flag];

				int DistanceToPoint = (abs(CharPos.x-PointPos.x) + abs(CharPos.y-PointPos.y));


				if(DistanceToPoint < 640) {
					if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_RED)
						flagControlling[flag].x++;
					if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_BLUE)
						flagControlling[flag].y++;
				}
			}
		}
	}

	// team controlling most flags
	vec2 teamPoints(0, 0);

	for(int i=0; i<flagCount+1; i++) {
		if(flagControlling[i].x < flagControlling[i].y)
			teamPoints.x++;
		if(flagControlling[i].x > flagControlling[i].y)
			teamPoints.y++;
	}

	// give points to team with most flags
	if(Server()->Tick() - scoreCounter > 5.0f) {
		scoreCounter = Server()->Tick();
		if(teamPoints.x > teamPoints.y) {
			m_aTeamscore[TEAM_RED^1] += 1;
			teamWinning = TEAM_RED;
		} else if(teamPoints.x < teamPoints.y) {
			m_aTeamscore[TEAM_BLUE^1] += 1;
			teamWinning = TEAM_BLUE;
		} else {
			teamWinning = -1;
		}

		// regenerate ammo
		if(str_comp(g_Config.m_SvGametype, "dom") != 0) {
			for(int i = 0; i < MAX_CLIENTS; i++) {
				if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter()) {
					CCharacter* Character = GameServer()->m_apPlayers[i]->GetCharacter();
					Character->GiveWeapon(Weapon, 1);
				}
			}
		}
	}

	if(teamWinning == TEAM_BLUE || teamWinning == TEAM_RED) {
		if(Server()->Tick() - broadcastCounter > 100.0f) {
			broadcastCounter = Server()->Tick();

			char aBuf[512];
			if(teamWinning == TEAM_RED)
				str_format(aBuf, sizeof(aBuf), "Blue team is holding the most points");
			else
				str_format(aBuf, sizeof(aBuf), "Red team is holding the most points");
			
			GameServer()->SendBroadcast(aBuf, -1);
		}
	}
}

void CGameControllerDOM::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
	if(!pGameDataObj)
		return;

	if(teamWinning == TEAM_RED) {
		pGameDataObj->m_FlagCarrierRed = FLAG_MISSING;
		pGameDataObj->m_FlagCarrierBlue = FLAG_ATSTAND;
	} else if(teamWinning == TEAM_BLUE) {
		pGameDataObj->m_FlagCarrierRed = FLAG_ATSTAND;
		pGameDataObj->m_FlagCarrierBlue = FLAG_MISSING;
	} else {
		pGameDataObj->m_FlagCarrierRed = FLAG_MISSING;
		pGameDataObj->m_FlagCarrierBlue = FLAG_MISSING;
	}

	pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
	pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];
}

void CGameControllerDOM::OnCharacterSpawn(class CCharacter *pChr)
{
	g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Ammoregentime = gunReload;
	g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Firedelay = gunDelay;

	// starting weapons
	if(str_comp(g_Config.m_SvGametype, "domgrenade") == 0) {
		pChr->IncreaseHealth(1);
		pChr->GiveWeapon(WEAPON_GRENADE, 1);
		pChr->SetWeapon(WEAPON_GRENADE);
		Weapon = WEAPON_GRENADE;
	} else if(str_comp(g_Config.m_SvGametype, "domrifle") == 0) {
		pChr->IncreaseHealth(1);
		pChr->GiveWeapon(WEAPON_RIFLE, 1);
		pChr->SetWeapon(WEAPON_RIFLE);
		Weapon = WEAPON_RIFLE;
	} else if(str_comp(g_Config.m_SvGametype, "domgun") == 0) {
		pChr->IncreaseHealth(1);
		pChr->GiveWeapon(WEAPON_GUN, 1);
		pChr->SetWeapon(WEAPON_GUN);
		Weapon = WEAPON_GUN;
		gunReload = g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Ammoregentime;
		gunDelay = g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Firedelay;
		g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Ammoregentime = g_pData->m_Weapons.m_aId[WEAPON_RIFLE].m_Ammoregentime;
		g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Firedelay = g_pData->m_Weapons.m_aId[WEAPON_RIFLE].m_Firedelay;
	} else {
		pChr->IncreaseHealth(10);
		pChr->GiveWeapon(WEAPON_HAMMER, -1);
		pChr->GiveWeapon(WEAPON_GUN, 10);
		Weapon = WEAPON_GUN;
	}
	
}