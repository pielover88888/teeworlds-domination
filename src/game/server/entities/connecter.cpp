/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "connecter.h"

CConnecter::CConnecter(CGameWorld *pGameWorld, vec2 Pos, vec2 To, int Distance)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos + To * Distance;
	m_From = Pos;
	m_Owner = -1;
	m_Bounces = 0;
	m_EvalTick = 0;
	GameWorld()->InsertEntity(this);
}

void CConnecter::Tick()
{
	m_EvalTick = Server()->Tick();

	if(m_Energy < 0) {
		Reset();
		return;
	}

	m_Energy = -1;
}

void CConnecter::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CConnecter::TickPaused()
{
	++m_EvalTick;
}

void CConnecter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = m_EvalTick;
}
