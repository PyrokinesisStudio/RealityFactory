/************************************************************************************//**
 * @file CTriggers.cpp
 * @brief Trigger handler
 *
 * This file contains the class implementation for the CTriggers class that
 * encapsulates trigger handling in the RGF.
 * @author Ralph Deane
 *//*
 * Copyright (c) 2001 Ralph Deane; All rights reserved.
 ****************************************************************************************/

// You only need the one, master include file.
#include "RabidFramework.h"

extern geSound_Def *SPool_Sound(const char *SName);

/* ------------------------------------------------------------------------------------ */
//	CTriggers
//
//	Default constructor.  Set all triggers to default values and load any audio we need.
/* ------------------------------------------------------------------------------------ */
CTriggers::CTriggers()
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	m_TriggerCount = 0;					// No triggers

	// Ok, check to see if there are triggers in this world
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");

	if(!pSet)
		return;									// No triggers

	// Ok, we have triggers somewhere.  Dig through 'em all.
	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);

		if(pTrigger->Model == NULL)
		{
			char szError[256];
			sprintf(szError, "[WARNING] File %s - Line %d: '%s' Missing Model",
					__FILE__, __LINE__, pTrigger->szEntityName);
			CCD->ReportError(szError, false);

			continue;
		}

		if(EffectC_IsStringNull(pTrigger->szEntityName))
		{
			char szName[128];
			geEntity_GetName(pEntity, szName, 128);
			pTrigger->szEntityName = szName;
		}



		m_TriggerCount++;							// Kick count

		// Ok, put this entity into the Global Entity Registry
		CCD->EntityRegistry()->AddEntity(pTrigger->szEntityName, "Trigger");

		// Now add the trigger into the Model Manager, which will do all the
		// ..hard work for us!
		{
			CCD->ModelManager()->AddModel(pTrigger->Model, ENTITY_TRIGGER);
			CCD->ModelManager()->SetLooping(pTrigger->Model, false);
			CCD->ModelManager()->SetAnimationSpeed(pTrigger->Model, pTrigger->AnimationSpeed);
		}

		// Reset all the animation data for each and every trigger
		pTrigger->bInAnimation	= GE_FALSE;	// Not animating
		pTrigger->AnimationTime = 0.0f;		// No time in animation
		pTrigger->bTrigger		= GE_FALSE;	// Not triggered
		pTrigger->time			= 0.0f;
		pTrigger->isHit			= GE_FALSE;
		pTrigger->CallBack		= GE_FALSE;
		pTrigger->bInCollision	= GE_FALSE;	// No collisions
		pTrigger->bActive		= GE_TRUE;	// Trigger is good to go
		pTrigger->bState		= GE_FALSE;	// Trigger state is off
		pTrigger->LastIncrement = 0.0f;		// No last time count
		pTrigger->theSound		= NULL;		// No sound, right now...
		pTrigger->SoundHandle	= -1;		// No sound playing

		if(!EffectC_IsStringNull(pTrigger->szSoundFile))
		{
			pTrigger->theSound = SPool_Sound(pTrigger->szSoundFile);

			if(!pTrigger->theSound)
			{
				char szError[256];
				sprintf(szError, "[WARNING] File %s - Line %d: %s: Failed to open audio file '%s'\n",
						__FILE__, __LINE__, pTrigger->szEntityName, pTrigger->szSoundFile);
				CCD->ReportError(szError, false);
			}
		}
	}

	//	Ok, we've counted the  triggers and reset 'em all to their default
	//	..values.  Leave!
	return;
}

/* ------------------------------------------------------------------------------------ */
//	~CTriggers
//
//	Default destructor
/* ------------------------------------------------------------------------------------ */
CTriggers::~CTriggers()
{
	return;
}

/* ------------------------------------------------------------------------------------ */
//	HandleCollision
//
//	Handle a collision with a  trigger
/* ------------------------------------------------------------------------------------ */
// changed RF063
int CTriggers::HandleCollision(const geWorld_Model *pModel, bool HitType, bool UseKey, const geActor *theActor)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	bool state;

	SetState();

	if(m_TriggerCount == 0)
		return RGF_FAILURE;									// None here, ignore call.

	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");

	if(!pSet)
	{
		CCD->ReportError("CTriggers: handlecollision: no triggers", false);
		return RGF_FAILURE;
	}

	// Once more we scan the list.  Does this get old, or what?
	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		// Get the  data so we can compare models
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);

		// changed QD 07/15/06
		if(!pTrigger->Model)
			continue;
		// end change

		if(pTrigger->Model == pModel)
		{
// changed RF063
			if(pTrigger->PlayerOnly && theActor != CCD->Player()->GetActor())
				return false;

// changed RF064
			if((pTrigger->bNoCollide && !UseKey) || (UseKey && !pTrigger->UseKey)
				|| (!pTrigger->bShoot && HitType)
				|| (pTrigger->bShoot && !HitType))
// end change RF064
			{
				FreeState();
				if(CCD->ModelManager()->EmptyContent(pTrigger->Model))
					return RGF_EMPTY;

				return RGF_FAILURE;
			}
// end change RF063

			state = true;

			if(!EffectC_IsStringNull(pTrigger->TriggerName))
				state = GetTTriggerState(pTrigger->TriggerName);

			if(!state)
			{
				pTrigger->CallBack = GE_TRUE;
				pTrigger->CallBackCount = 2;
			}

			// If trigger not already running, and active, trigger it!
			if((!pTrigger->bInAnimation)
				&& (pTrigger->bActive == GE_TRUE)
				&& pTrigger->isHit == GE_FALSE
				&& state == true)
			{
				pTrigger->bState = GE_TRUE;
				pTrigger->bTrigger = GE_TRUE;			// It's this one, trigger the animation
				CCD->ModelManager()->Start(pTrigger->Model);
				pTrigger->SoundHandle = PlaySound(pTrigger->theSound, pTrigger->origin, pTrigger->bAudioLoops);
				pTrigger->isHit = GE_TRUE;
				pTrigger->time = 0.0f;
				pTrigger->bInAnimation = GE_TRUE;
			}

			FreeState();

			if(CCD->ModelManager()->EmptyContent(pTrigger->Model))
				return RGF_EMPTY;

			return RGF_SUCCESS;  // Hmmph, we hit a trigger
		}
	}

	FreeState();
	return RGF_FAILURE;							// We hit no known triggers
}


// MOD010122 - Added this function.
/* ------------------------------------------------------------------------------------ */
//	HandleTriggerEvent
//
//	Handle an animated model's trigger event
/* ------------------------------------------------------------------------------------ */
bool CTriggers::HandleTriggerEvent(const char *TName)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	bool state;

	SetState();

	if(m_TriggerCount == 0)
		return false;									// None here, ignore call.

	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");

	if(!pSet)
	{
		CCD->ReportError("CTriggers: handletriggerevent: no triggers", false);
		return false;
	}

	//	Once more we scan the list.  Does this get old, or what?
	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		// Get the  data so we can compare models
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);

		// changed QD 07/15/06
		if(!pTrigger->Model)
			continue;
		// end change

		if(!strcmp(pTrigger->szEntityName, TName))
		{
			state = true;

			if(!EffectC_IsStringNull(pTrigger->TriggerName))
				state = GetTTriggerState(pTrigger->TriggerName);

			if(!state)
			{
				pTrigger->CallBack = GE_TRUE;
				pTrigger->CallBackCount = 2;
			}

			// If trigger not already running, and active, trigger it!
			if((!pTrigger->bInAnimation)
				&& pTrigger->bActive == GE_TRUE
				&& pTrigger->isHit == GE_FALSE
				&& state == true)
			{
				pTrigger->bState = GE_TRUE;
				pTrigger->bTrigger = GE_TRUE;			// It's this one, trigger the animation
// changed RF063
				CCD->ModelManager()->Start(pTrigger->Model);
// end change RF063
				pTrigger->SoundHandle = PlaySound(pTrigger->theSound, pTrigger->origin, pTrigger->bAudioLoops);
				pTrigger->isHit = GE_TRUE;
				pTrigger->time = 0.0f;
				pTrigger->bInAnimation = GE_TRUE;
			}

			FreeState();
			return true;  // Hmmph, we hit a trigger
		}
	}

    FreeState();
    return false;							// We hit no known triggers
}

/* ------------------------------------------------------------------------------------ */
//	PlaySound
/* ------------------------------------------------------------------------------------ */
// changed QD 12/15/05 - changed 2nd argument from geVec3d to const geVec3d&
int CTriggers::PlaySound(geSound_Def *theSound, const geVec3d &Origin, bool SoundLoop)
{
	if(!theSound)
		return -1;

	Snd Sound;

	memset(&Sound, 0, sizeof(Sound));
    geVec3d_Copy(&Origin, &(Sound.Pos));
    Sound.Min = CCD->GetAudibleRadius();
	Sound.Loop = SoundLoop;
	Sound.SoundDef = theSound;
	int index = CCD->EffectManager()->Item_Add(EFF_SND, (void*)&Sound);

	if(SoundLoop)
		return index;

	return -1;
}

/* ------------------------------------------------------------------------------------ */
//	IsATrigger
//
//	Return TRUE if the passed-in model is a  trigger, FALSE otherwise
/* ------------------------------------------------------------------------------------ */
bool CTriggers::IsATrigger(const geWorld_Model *theModel)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	if(m_TriggerCount == 0)
		return false;										// Don't waste time here.

	// Ok, check to see if there are  triggers in this world
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");

	if(!pSet)
		return false;									// No  triggers

	// Ok, we have  triggers somewhere.  Dig through 'em all.
	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);

		// changed QD 07/15/06
		if(!pTrigger->Model)
			continue;
		// end change

		if(pTrigger->Model == theModel)
			return true;							// Model IS a trigger
	}

	return false;							// Nope, it's not a trigger
}

/* ------------------------------------------------------------------------------------ */
//	Tick
//
//	Increment animation times for all _animating_  triggers that aren't
//	..in a collision state.
/* ------------------------------------------------------------------------------------ */
void CTriggers::Tick(geFloat dwTicks)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	if(m_TriggerCount == 0)
		return;									// Don't waste time here

	// Ok, check to see if there are  triggers in this world
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");

	if(!pSet)
		return;									// No  triggers

	// Ok, we have triggers somewhere.  Dig through 'em all.
	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);

		if(!pTrigger->Model)
			continue;

		pTrigger->LastIncrement = dwTicks;					// The amount that moved

		if(pTrigger->isHit == GE_TRUE)
		{
			pTrigger->time += dwTicks;

			if(pTrigger->bState == GE_TRUE)
			{
				if(pTrigger->time >= (pTrigger->TimeOn*1000.0f))
				{
					pTrigger->bState = GE_FALSE;
					pTrigger->time = 0.0f;
					// MOD010122 - Next line commented out to fix bug.  isHit should not be turned off until the full trigger
					//             cycle, on-off is complete.
					//					pTrigger->isHit=false;
				}
			}
			else
			{
				if(pTrigger->time >= (pTrigger->TimeOff*1000.0f))
				{
					pTrigger->bState = GE_FALSE;
					pTrigger->isHit = GE_FALSE;
					pTrigger->time = 0.0f;
				}
			}
		}

		if(pTrigger->CallBack == GE_TRUE)
		{
			pTrigger->CallBackCount -= 1;

			if(pTrigger->CallBackCount == 0)
				pTrigger->CallBack = GE_FALSE;
		}

		if((pTrigger->bInAnimation == GE_TRUE) &&
			(CCD->ModelManager()->IsRunning(pTrigger->Model) == false))
		{
			// Animation has stopped/not running, handle it.
			// MOD010122 - The "&& (!pTrigger->isHit)" was added to the next if statement.  We don't want to reset until
			//             the full trigger cycle, on-off, is complete.
			if((pTrigger->bOneShot != GE_TRUE) && (pTrigger->bActive == GE_TRUE) && (!pTrigger->isHit))
			{
				// Ok, not one-shot, reset the door
				pTrigger->bInAnimation = GE_FALSE;
				pTrigger->bTrigger = GE_FALSE;
			}

			CCD->ModelManager()->Stop(pTrigger->Model);

			if(pTrigger->SoundHandle != -1)
			{
				CCD->EffectManager()->Item_Delete(EFF_SND, pTrigger->SoundHandle);
				pTrigger->SoundHandle = -1;
			}
		}
	}

	return;
}

/* ------------------------------------------------------------------------------------ */
//	SaveTo
//
//	Save the current state of every  trigger in the current world off to an open file.
/* ------------------------------------------------------------------------------------ */
int CTriggers::SaveTo(FILE *SaveFD, bool type)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	// Ok, check to see if there are  triggers in this world
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");

	if(!pSet)
		return RGF_SUCCESS;									// No triggers, whatever...

	// Ok, we have triggers somewhere.  Dig through 'em all.
	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);

		// changed QD 07/15/06
		if(!pTrigger->Model)
			continue;
		// end change

		WRITEDATA(type, &pTrigger->bInAnimation,	sizeof(geBoolean),	1, SaveFD);
		WRITEDATA(type, &pTrigger->bTrigger,		sizeof(geBoolean),	1, SaveFD);
		WRITEDATA(type, &pTrigger->AnimationTime,	sizeof(int),		1, SaveFD);
		WRITEDATA(type, &pTrigger->bInCollision,	sizeof(geBoolean),	1, SaveFD);
		WRITEDATA(type, &pTrigger->bActive,			sizeof(geBoolean),	1, SaveFD);
		WRITEDATA(type, &pTrigger->LastIncrement,	sizeof(int),		1, SaveFD);
		WRITEDATA(type, &pTrigger->tDoor,			sizeof(float),		1, SaveFD);
		WRITEDATA(type, &pTrigger->origin,			sizeof(geVec3d),	1, SaveFD);
		WRITEDATA(type, &pTrigger->bState,			sizeof(geBoolean),	1, SaveFD);
		WRITEDATA(type, &pTrigger->time,			sizeof(float),		1, SaveFD);
		WRITEDATA(type, &pTrigger->isHit,			sizeof(geBoolean),	1, SaveFD);
		WRITEDATA(type, &pTrigger->CallBack,		sizeof(geBoolean),	1, SaveFD);
		WRITEDATA(type, &pTrigger->CallBackCount,	sizeof(int),		1, SaveFD);
	}

	return RGF_SUCCESS;
}

/* ------------------------------------------------------------------------------------ */
//	RestoreFrom
//
//	Restore the state of every  trigger in the current world from an open file.
/* ------------------------------------------------------------------------------------ */
int CTriggers::RestoreFrom(FILE *RestoreFD, bool type)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	// Ok, check to see if there are  triggers in this world
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");

	if(!pSet)
		return RGF_SUCCESS;									// No triggers, whatever...

	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);

		// changed QD 07/15/06
		if(!pTrigger->Model)
			continue;
		// end change

		READDATA(type, &pTrigger->bInAnimation,		sizeof(geBoolean),	1, RestoreFD);
		READDATA(type, &pTrigger->bTrigger,			sizeof(geBoolean),	1, RestoreFD);
		READDATA(type, &pTrigger->AnimationTime,	sizeof(int),		1, RestoreFD);
		READDATA(type, &pTrigger->bInCollision,		sizeof(geBoolean),	1, RestoreFD);
		READDATA(type, &pTrigger->bActive,			sizeof(geBoolean),	1, RestoreFD);
		READDATA(type, &pTrigger->LastIncrement,	sizeof(int),		1, RestoreFD);
		READDATA(type, &pTrigger->tDoor,			sizeof(float),		1, RestoreFD);
		READDATA(type, &pTrigger->origin,			sizeof(geVec3d),	1, RestoreFD);
		READDATA(type, &pTrigger->bState,			sizeof(geBoolean),	1, RestoreFD);
		READDATA(type, &pTrigger->time,				sizeof(float),		1, RestoreFD);
		READDATA(type, &pTrigger->isHit,			sizeof(geBoolean),	1, RestoreFD);
		READDATA(type, &pTrigger->CallBack,			sizeof(geBoolean),	1, RestoreFD);
		READDATA(type, &pTrigger->CallBackCount,	sizeof(int),		1, RestoreFD);

		if(pTrigger->bInAnimation)
			geWorld_OpenModel(CCD->World(), pTrigger->Model, GE_TRUE);
    }

	return RGF_SUCCESS;
}

//	******************** CRGF Overrides ********************

/* ------------------------------------------------------------------------------------ */
//	LocateEntity
//
//	Given a name, locate the desired item in the currently loaded level
//	..and return it's user data.
/* ------------------------------------------------------------------------------------ */
int CTriggers::LocateEntity(const char *szName, void **pEntityData)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	// Ok, check to see if there are  triggers in this world
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");

	if(!pSet)
		return RGF_NOT_FOUND;									// No triggers

	// Ok, we have  triggers.  Dig through 'em all.
	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		Trigger *pTheEntity = (Trigger*)geEntity_GetUserData(pEntity);

		if(!strcmp(pTheEntity->szEntityName, szName))
		{
			*pEntityData = (void*)pTheEntity;
			return RGF_SUCCESS;
		}
	}

	return RGF_NOT_FOUND;								// Sorry, no such entity here
}

/* ------------------------------------------------------------------------------------ */
//	ReSynchronize
//
//	Correct internal timing to match current time, to make up for time lost
//	..when outside the game loop (typically in "menu mode").
/* ------------------------------------------------------------------------------------ */
int CTriggers::ReSynchronize()
{
	return RGF_SUCCESS;
}

/* ------------------------------------------------------------------------------------ */
//	SetState
/* ------------------------------------------------------------------------------------ */
void CTriggers::SetState()
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	TState *pool;

	Bottom = (TState *)NULL;

	if(m_TriggerCount == 0)
		return;						// Don't waste CPU cycles

	// Ok, check to see if there are triggers in this world
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");

	if(!pSet)
		return;

	// Ok, we have triggers somewhere.  Dig through 'em all.
	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		Trigger *pSource = (Trigger*)geEntity_GetUserData(pEntity);

		if(EffectC_IsStringNull(pSource->szEntityName))
		{
			char szName[128];
			geEntity_GetName(pEntity, szName, 128);
			pSource->szEntityName = szName;
		}

		pool = GE_RAM_ALLOCATE_STRUCT(TState);
		memset(pool, 0xbb, sizeof(TState));
		pool->next = Bottom;
		Bottom = pool;

		if(pool->next)
			pool->next->prev = pool;

		pool->Name = strdup(pSource->szEntityName);
		pool->state = pSource->bState;
	}

	return;
}

/* ------------------------------------------------------------------------------------ */
//	GetTTriggerState
/* ------------------------------------------------------------------------------------ */
bool CTriggers::GetTTriggerState(const char *Name)
{
	TState *pool;

	char *EntityType = CCD->EntityRegistry()->GetEntityType(Name);

	if(EntityType)
	{
		if(!stricmp(EntityType, "Trigger"))
		{
			pool = Bottom;

			while(pool != NULL)
			{
				if(!stricmp(Name, pool->Name))
					return pool->state;

				pool = pool->next;
			}

			char szError[256];
			sprintf(szError, "[WARNING] File %s - Line %d: Invalid Trigger Name '%s'\n",
					__FILE__, __LINE__, Name);
			CCD->ReportError(szError, false);
			return false;
		}
	}

	return GetTriggerState(Name);
}

/* ------------------------------------------------------------------------------------ */
//	FreeState
/* ------------------------------------------------------------------------------------ */
void CTriggers::FreeState()
{
	TState *pool, *temp;
	pool = Bottom;

	while(pool != NULL)
	{
		temp = pool->next;
		free(pool->Name);
		geRam_Free(pool);
		pool = temp;
	}
}


/* ----------------------------------- END OF FILE ------------------------------------ */
