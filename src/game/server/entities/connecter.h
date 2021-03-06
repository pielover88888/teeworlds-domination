/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_LASER_H
#define GAME_SERVER_ENTITIES_LASER_H

#include <game/server/entity.h>

class CConnecter : public CEntity
{
public:
	CConnecter(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, int Distance);

	virtual void Reset();
	virtual void Snap(int SnappingClient);
	virtual void Tick();
	virtual void TickPaused();

private:
	vec2 m_From;
	vec2 m_Dir;
	float m_Energy;
	int m_Bounces;
	int m_EvalTick;
	int m_Owner;
};

#endif
