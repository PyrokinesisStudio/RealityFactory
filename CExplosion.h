/************************************************************************************//**
 * @file CExplosion.h
 * @brief Explosion Manager handler
 *
 * This file contains the class declarations for the Explosion Manager that
 * handles Explosions for RGF-based games.
 * @author Ralph Deane
 *//*
 * Copyright(c) 1999 Ralph Deane; All rights reserved.
 ****************************************************************************************/

#ifndef __RGF_EXPMANAGER_H_
#define __RGF_EXPMANAGER_H_

#define MAXEXP 50

typedef struct preExplosion
{
	geBoolean	Active;
	char		Name[64];
	char		Effect[10][64];
	float		Delay[10];
	geVec3d		Offset[10];

} preExplosion;

typedef struct DelayExp
{
	DelayExp	*prev;
	DelayExp	*next;
	int			Type;
	geVec3d		Position;
	geVec3d		Offset;
	float		Delay;
	bool		Attached;
// changed RF063
	geActor		*Actor;
	char		Bone[64];
	int			index;
	bool		Tilt;
// end change RF063

} DelayExp;


class CExplosionInit : public CRGFComponent
{
public:
	CExplosionInit();
	~CExplosionInit();

	void AddExplosion(char *Name, const geVec3d &Position, geActor *theActor, char *theBone);
	void AddExplosion(char *Name, const geVec3d &Position, geActor *theActor, char *theBone, bool Tilt);
	void AddExplosion(char *Name, const geVec3d &Position);
	void Tick(geFloat dwTicks);
	void UnAttach(geActor *Actor);

private:

	preExplosion Explosions[MAXEXP];
	DelayExp *Bottom;
};

class CExplosion : public CRGFComponent
{
public:
	CExplosion();
	~CExplosion();
	void Tick(geFloat dwTicks);
private:
};

#endif

/* ----------------------------------- END OF FILE ------------------------------------ */
