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
	teamWinning = -1;

	m_pGameType = "DOM";
	m_GameFlags = GAMEFLAG_TEAMS|GAMEFLAG_FLAGS;
}

bool CGameControllerDOM::OnEntity(int Index, vec2 Pos)
{
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