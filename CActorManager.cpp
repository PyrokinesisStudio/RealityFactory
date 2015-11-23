/*
CActorManager.cpp:		Actor Manager

  This file contains the class implementation of the Actor Manager
  system for Reality Factory.
  
	It's the Actor Manager's job to load, remove, translate,
	rotate, and scale all the actors in the system.  The Actor Manager
	maintains a database of "loaded actors" from which instances can
	be created.
*/

#include "RabidFramework.h"		// The One True Include File

extern "C" void	DrawBoundBox(geWorld *World, const geVec3d *Pos, const geVec3d *Min, const geVec3d *Max);
extern geBitmap *TPool_Bitmap(char *DefaultBmp, char *DefaultAlpha, char *BName, char *AName);

//	Default constructor

CActorManager::CActorManager()
{
	for(int nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
		MainList[nTemp] = NULL;
	
	m_GlobalInstanceCount = 0;			// No instances
// changed RF064
	ShadowBitmap = TPool_Bitmap("lvsmoke.Bmp", "a_lvsmoke.Bmp", NULL, NULL);
	ShadowAlpha = 128.0f;
	AttrFile.SetPath("material.ini");
	ValidAttr = true;
	if(!AttrFile.ReadFile())
		ValidAttr = false;
// end change RF064
	
	return;
}

//	Default destructor.  Massacre everyone on the level.

CActorManager::~CActorManager()
{
	for(int nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
	{
		RemoveAllActors(MainList[nTemp]);
		if(MainList[nTemp] == NULL)
			continue;					// Empty slot, ignore
		if(MainList[nTemp]->szFilename != NULL)
		{
			delete MainList[nTemp]->szFilename;
		}
		if(MainList[nTemp]->theActorDef != NULL)
		{
			geActor_DefDestroy(&MainList[nTemp]->theActorDef);
			geActor_Destroy(&MainList[nTemp]->Actor);
		}
		delete MainList[nTemp];
		MainList[nTemp] = NULL;
	}
	
	return;
}

//	LoadActor
//
//	If the required actor isn't loaded, load it into memory and
//	..make a new instance list for it.  If it's already loaded,
//	..make a new instance of it.  Regardless, return a numeric
//	..handle to the actor instance.

geActor *CActorManager::LoadActor(char *szFilename, geActor *OldActor)
{
	geVFile *ActorFile;
	geActor *theActor = NULL;
	
	//	Ok, scan the mainlist for this actor
	
	for(int nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
	{
		// Ok, something in this entry?
		if(MainList[nTemp] != NULL)
		{
			// Check the source filename, is it the same?
			if(!strcmp(MainList[nTemp]->szFilename, szFilename))
			{
				// It's the same, add a new instance of this actor
				m_GlobalInstanceCount++;
				theActor = AddNewInstance(MainList[nTemp], OldActor);
				return theActor;
			}
		}
	}
	
	//	Guess what?  We don't have an entry for it!  Make one.
	
	//	Try to open up the desired .act file
	
	CCD->OpenRFFile(&ActorFile, kActorFile, szFilename,
								GE_VFILE_OPEN_READONLY);
	
	geActor_Def *ActorDef = NULL;
	
	if(ActorFile)
	{
		// Create a definition of the actor
		ActorDef = geActor_DefCreateFromFile(ActorFile);
		if(!ActorDef)
		{
			geVFile_Close(ActorFile);							// Clean up in case of error
			char szError[256];
			sprintf(szError,"CActorManager: can't create def from file '%s'", szFilename);
			CCD->ReportError(szError, false);
			return RGF_FAILURE;
		}
	}
	else							// File didn't open, error out.
	{
		char szError[256];
		sprintf(szError,"CActorManager: can't load actor '%s'", szFilename);
		CCD->ReportError(szError, false);
		return RGF_FAILURE;
	}
	
	geVFile_Close(ActorFile);											// Close the input file.
	
	int nSlot = 0;
	while(MainList[nSlot] != NULL)
		nSlot++;
	
	MainList[nSlot] = new LoadedActorList;
	memset(MainList[nSlot], 0, sizeof(LoadedActorList));
	
	//	Ok, set up the master entry in the table...
	
	MainList[nSlot]->theActorDef = ActorDef;
	MainList[nSlot]->Actor = geActor_Create(ActorDef);
	geWorld_AddActor(CCD->World(), MainList[nSlot]->Actor, 
		GE_ACTOR_RENDER_NORMAL | GE_ACTOR_COLLIDE, 0xffffffff);
	geWorld_RemoveActor(CCD->World(), MainList[nSlot]->Actor);
	MainList[nSlot]->szFilename = new char[strlen(szFilename)+1];
	// weapon
	memcpy(MainList[nSlot]->szFilename, szFilename, strlen(szFilename)+1);
	MainList[nSlot]->InstanceCount = 0;
	MainList[nSlot]->IList = NULL;
	
	//	Now we need to create an initial instance
	
	m_GlobalInstanceCount++;
	theActor = AddNewInstance(MainList[nSlot], NULL);

	return theActor;
}

geActor *CActorManager::SpawnActor(char *szFilename, geVec3d thePosition, geVec3d Rotation,
								   char *DefaultMotion, char *CurrentMotion, geActor *OldActor)
{
	//	Try and load it...
	if(EffectC_IsStringNull(szFilename))
		return NULL;
	
	geActor *theActor = LoadActor(szFilename, OldActor);
	
	if(theActor != NULL)
	{
		// It worked!  Configure the new actor
		SetAligningRotation(theActor, Rotation);					// Rotate the actor
		geVec3d theRotation = {0.0, 0.0, 0.0};
		Rotate(theActor, theRotation);
		Position(theActor, thePosition);				// Position the actor
		SetMotion(theActor, CurrentMotion);	// Set actor motion
		SetDefaultMotion(theActor, DefaultMotion);	// Set default motion
	}
	
	return theActor;
}

// changed RF064
int CActorManager::ChangeMaterial(geActor *theActor, char *Change)
{
	geBody		*Body;
	int32		Material;
	int32		MaterialCount;
	int FirstIndex;
	geBitmap	*Bitmap;
	float		R, G, B;
	const char		*MaterialName;

	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL || !ValidAttr)
		return RGF_NOT_FOUND;

	CString KeyName = AttrFile.FindFirstKey();
	CString Type, Value;
	while(KeyName != "")
	{
		if(!strcmp(KeyName, Change))
		{
			Body = geActor_GetBody(pEntry->theDef);
			MaterialCount = geActor_GetMaterialCount(pEntry->Actor);

			Type = AttrFile.FindFirstName(KeyName);
			Value = AttrFile.FindFirstValue();
			while(Type!="")
			{
				bool found = false;
				for ( Material = 0; Material < MaterialCount; Material++ )
				{
					geBody_GetMaterial( Body, Material, &MaterialName, &Bitmap, &R, &G, &B );
					if(!strcmp(Type, MaterialName))
					{
						found = true;
						break;
					}
				}
				if(!found)
					return RGF_FAILURE;

				FirstIndex = Material;
				found = false;
				for ( Material = 0; Material < MaterialCount; Material++ )
				{
					geBody_GetMaterial( Body, Material, &MaterialName, &Bitmap, &R, &G, &B );
					if(!strcmp(Value, MaterialName))
					{
						found = true;
						break;
					}
				}
				if(!found)
					return RGF_FAILURE;

				geActor_SetMaterial(pEntry->Actor, FirstIndex, Bitmap, R, G, B );

				Type = AttrFile.FindNextName();
				Value = AttrFile.FindNextValue();
			}
			return RGF_SUCCESS;
		}
		KeyName = AttrFile.FindNextKey();
	}
	return RGF_FAILURE;
}
// end change RF064

//	RemoveActor
//
//	Remove the actor instance referred to by the given handle.
//	..This takes it out of the game IMMEDIATELY.

int CActorManager::RemoveActor(geActor *theActor)
{
	return RemoveInstance(theActor);
}

//	SetAligningRotation
//
//	Set the rotation necessary to align actor with world

int CActorManager::SetAligningRotation(geActor *theActor, geVec3d theRotation)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->BaseRotation = theRotation;
	
	return RGF_SUCCESS;
}

//	GetAligningRotation
//
//	Get the alignment rotation for this actor

int CActorManager::GetAligningRotation(geActor *theActor, geVec3d *theRotation)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*theRotation = pEntry->BaseRotation;
	
	return RGF_SUCCESS;
}

//	SetType
//
//	Sets the TYPE of an actor (prop, vehicle, NPC, etc.)

int CActorManager::SetType(geActor *theActor, int nType)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->ActorType = nType;
	
	return RGF_SUCCESS;
}

//	GetType
//
//	Gets the TYPE of an actor (prop, vehicle, NPC, etc.)

int CActorManager::GetType(geActor *theActor, int *nType)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*nType = pEntry->ActorType;
	
	return RGF_SUCCESS;
}

//	Rotate
//
//	Assign an absolute rotation to an actor instance

int CActorManager::Rotate(geActor *theActor, geVec3d theRotation)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->localRotation = theRotation;		// Set it, and forget it!
	
	UpdateActorState(pEntry);
	
	return RGF_SUCCESS;
}

//	Position
//
//	Assign an absolute position to an actor instance

int CActorManager::Position(geActor *theActor, geVec3d Position)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->OldTranslation = pEntry->localTranslation;
	
	pEntry->localTranslation = Position;
	
	UpdateActorState(pEntry);
	
	return RGF_SUCCESS;
}

//	MoveAway
//
//	Move an actor to a position totally outside any possible world.

int CActorManager::MoveAway(geActor *theActor)
{
	geVec3d thePosition = {10000.0f,10000.0f,10000.0f};
	Position(theActor, thePosition);
	
	return RGF_SUCCESS;
}

//	GetPosition
//
//	Get the current position of a specific actor

int CActorManager::GetPosition(geActor *theActor, geVec3d *thePosition)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*thePosition = pEntry->localTranslation;
	
	return RGF_SUCCESS;
}

//	GetRotation
//
//	Get the current rotation for a specific actor

int CActorManager::GetRotation(geActor *theActor, geVec3d *theRotation)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*theRotation = pEntry->localRotation;
	theRotation->X += pEntry->BaseRotation.X;
	theRotation->Y += pEntry->BaseRotation.Y;
	theRotation->Z += pEntry->BaseRotation.Z;
	
	return RGF_SUCCESS;
}

int CActorManager::GetRotate(geActor *theActor, geVec3d *theRotation)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*theRotation = pEntry->localRotation;
	
	return RGF_SUCCESS;
}

//	TurnLeft
//
//	Rotate actor to turn to the LEFT

int CActorManager::TurnLeft(geActor *theActor, geFloat theAmount)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->localRotation.Y -= theAmount;
	
	UpdateActorState(pEntry);
	
	return RGF_SUCCESS;
}

//	TurnRight
//
//	Rotate actor to turn to the RIGHT

int CActorManager::TurnRight(geActor *theActor, geFloat theAmount)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->localRotation.Y += theAmount;
	
	UpdateActorState(pEntry);
	
	return RGF_SUCCESS;
}


//	TiltUp
//
//	Rotate actor to tilt up

int CActorManager::TiltUp(geActor *theActor, geFloat theAmount)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	if(pEntry->AllowTilt)
		pEntry->localRotation.X += theAmount;
	else
		pEntry->TiltX += theAmount;
	
	UpdateActorState(pEntry);
	
	return RGF_SUCCESS;
}


//	TiltDown
//
//	Rotate actor to tilt up

int CActorManager::TiltDown(geActor *theActor, geFloat theAmount)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	if(pEntry->AllowTilt)
		pEntry->localRotation.X -= theAmount;
	else
		pEntry->TiltX -= theAmount;
	
	UpdateActorState(pEntry);
	
	return RGF_SUCCESS;
}

float CActorManager::GetTiltX(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return 0.0f;					// Actor not found?!?!

	return pEntry->TiltX;
}

int CActorManager::ReallyFall(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;
	
//	if(pEntry->GravityTime != 0.0f)
//		return RGF_SUCCESS;

	geVec3d StartPos, EndPos;
	GE_Collision Collision;
	
	StartPos = EndPos = pEntry->localTranslation;
	EndPos.Y -= pEntry->MaxStepHeight;
	
	CCD->Collision()->IgnoreContents(false);
	CCD->Collision()->CheckLevel(RGF_COLLISIONLEVEL_1); // RGF_COLLISIONLEVEL_2
	
	//	Start off with the infamous Collision Check.
	
	bool Result = CCD->Collision()->CheckActorCollision(StartPos, EndPos, pEntry->Actor,
		&Collision);
	
	if(Result)
		return RGF_FAILURE;
	
	return RGF_SUCCESS;
}

int CActorManager::CheckReallyFall(geActor *theActor, geVec3d StartPos)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;
	
	geVec3d EndPos;
	GE_Collision Collision;
	
	EndPos = StartPos;
	EndPos.Y -= pEntry->MaxStepHeight;
	
	CCD->Collision()->IgnoreContents(false);
	CCD->Collision()->CheckLevel(RGF_COLLISIONLEVEL_1); // RGF_COLLISIONLEVEL_2
	
	//	Start off with the infamous Collision Check.
	
	bool Result = CCD->Collision()->CheckActorCollision(StartPos, EndPos, pEntry->Actor,
		&Collision);
	
	if(Result)
		return RGF_FAILURE;
	
	return RGF_SUCCESS;
}

//	UpVector
//
//	Return the UP vector for a specified actor

int CActorManager::UpVector(geActor *theActor, geVec3d *UpVector)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	geXForm3d thePosition;
	
	geXForm3d_SetIdentity(&thePosition);		// Initialize
	
	// Perform orienting rotations to properly align model
	
	geVec3d theRotation = pEntry->localRotation;
	
	geXForm3d_RotateZ(&thePosition, theRotation.Z);
	geXForm3d_RotateX(&thePosition, theRotation.X);
	geXForm3d_RotateY(&thePosition, theRotation.Y);
	
	// Finally, translate this mother!
	
	geXForm3d_Translate(&thePosition, pEntry->localTranslation.X, 
		pEntry->localTranslation.Y, pEntry->localTranslation.Z);
	
	geXForm3d_GetUp(&thePosition, UpVector);		// Up... yours?
	
	return RGF_SUCCESS;
}

//	InVector
//
//	Return the IN vector for a specified actor

int CActorManager::InVector(geActor *theActor, geVec3d *InVector)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	geXForm3d thePosition;
	
	geXForm3d_SetIdentity(&thePosition);		// Initialize
	
	// Perform orienting rotations to properly align model
	
	geVec3d theRotation = pEntry->localRotation;
	
	geXForm3d_RotateZ(&thePosition, theRotation.Z);
	geXForm3d_RotateX(&thePosition, theRotation.X);
	geXForm3d_RotateY(&thePosition, theRotation.Y);
	
	// Finally, translate this mother!
	
	geXForm3d_Translate(&thePosition, pEntry->localTranslation.X, 
		pEntry->localTranslation.Y, pEntry->localTranslation.Z);
	
	geXForm3d_GetIn(&thePosition, InVector);		// In & out
	
	return RGF_SUCCESS;
}

//	LeftVector
//
//	Return the LEFT vector for a specified actor

int CActorManager::LeftVector(geActor *theActor, geVec3d *LeftVector)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	geXForm3d thePosition;
	
	geXForm3d_SetIdentity(&thePosition);		// Initialize
	
	// Perform orienting rotations to properly align model
	
	geVec3d theRotation = pEntry->localRotation;
	
	geXForm3d_RotateZ(&thePosition, theRotation.Z);
	geXForm3d_RotateX(&thePosition, theRotation.X);
	geXForm3d_RotateY(&thePosition, theRotation.Y);
	
	// Finally, translate this mother!
	
	geXForm3d_Translate(&thePosition, pEntry->localTranslation.X, 
		pEntry->localTranslation.Y, pEntry->localTranslation.Z);
	
	geXForm3d_GetLeft(&thePosition, LeftVector);		// Left wing party
	
	return RGF_SUCCESS;
}

//	GetBonePosition
//
//	Return the translation of a bone in an actor

int CActorManager::GetBonePosition(geActor *theActor, char *szBone, geVec3d *thePosition)
{
	geXForm3d BoneXForm;
	
	if(geActor_GetBoneTransform(theActor, szBone, &BoneXForm) != GE_TRUE)
		return RGF_NOT_FOUND;						// No such bone
	
	*thePosition = BoneXForm.Translation;
	
	return RGF_SUCCESS;
}

//	GetBoneRotation
//
//	Return the rotation of a bone in an actor

int CActorManager::GetBoneRotation(geActor *theActor, char *szBone, geVec3d *theRotation)
{
	geXForm3d BoneXForm;
	
	if(!EffectC_IsStringNull(szBone))
	{
		if(geActor_GetBoneTransform(theActor, szBone, &BoneXForm) != GE_TRUE)
			return RGF_NOT_FOUND;						// No such bone
	}
	else
	{
		if(geActor_GetBoneTransform(theActor, NULL, &BoneXForm) != GE_TRUE)
			return RGF_NOT_FOUND;						// No such bone
	}
	
	geXForm3d_GetEulerAngles(&BoneXForm, theRotation);
	
	return RGF_SUCCESS;
}

//	AddTranslation
//
//	Add translation to actors current position

int CActorManager::AddTranslation(geActor *theActor, geVec3d Amount)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->OldTranslation = pEntry->localTranslation;
	
	pEntry->localTranslation.X += Amount.X;
	pEntry->localTranslation.Y += Amount.Y;
	pEntry->localTranslation.Z += Amount.Z;
	
	UpdateActorState(pEntry);
	
	return RGF_SUCCESS;
}

//	AddRotation
//
//	Add rotation to actors current rotation

int CActorManager::AddRotation(geActor *theActor, geVec3d Amount)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->localRotation.X += Amount.X;
	pEntry->localRotation.Y += Amount.Y;
	pEntry->localRotation.Z += Amount.Z;
	
	UpdateActorState(pEntry);
	
	return RGF_SUCCESS;
}

//	RotateToFacePoint
//
//	Rotate the actor to face a specific point

int CActorManager::RotateToFacePoint(geActor *theActor, geVec3d Position)
{
	geVec3d LookAt, LookFrom;
	geVec3d LookRotation;
	geFloat x,l;
	
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	LookAt = Position;
	LookFrom = pEntry->localTranslation;
	
	geVec3d_Subtract(&LookAt, &LookFrom, &LookRotation);
	
	l = geVec3d_Length(&LookRotation);
	
	// protect from Div by Zero
	if(l > 0.0f) 
	{
		x = LookRotation.X;
		LookRotation.X = (float)( GE_PI*0.5 ) - 
			(float)acos(LookRotation.Y / l);
		LookRotation.Y = (float)atan2( x , LookRotation.Z ) + GE_PI;
		// roll is zero - always!!?
		LookRotation.Z = 0.0;
	}
	
	pEntry->localRotation.X = LookRotation.X;
	pEntry->localRotation.Y = LookRotation.Y;
	pEntry->localRotation.Z = LookRotation.Z;
	
	UpdateActorState(pEntry);
	
	return RGF_SUCCESS;
}

//	SetAutoStepUp
//
//	Enable or disable auto step up for this actor

int CActorManager::SetAutoStepUp(geActor *theActor, bool fEnable)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->fAutoStepUp = fEnable;	// Set it up!
	
	return RGF_SUCCESS;
}

//	SetStepHeight
//
//	Set the step-up height for this actor

int CActorManager::SetStepHeight(geActor *theActor, geFloat fMaxStep)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->MaxStepHeight = fMaxStep;
	
	return RGF_SUCCESS;
}

// changed RF063
//	GetStepHeight
//
//	Get the step-up height for this actor

int CActorManager::GetStepHeight(geActor *theActor, geFloat *fMaxStep)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*fMaxStep = pEntry->MaxStepHeight;
	
	return RGF_SUCCESS;
}
// end change RF063

//	MoveForward
//
//	Move the actor forward at some specific velocity

int CActorManager::MoveForward(geActor *theActor, geFloat fSpeed)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	return Move(pEntry, RGF_K_FORWARD, fSpeed);
}

//	MoveBackward
//
//	Move the actor backward at some specific velocity

int CActorManager::MoveBackward(geActor *theActor, geFloat fSpeed)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	return Move(pEntry, RGF_K_BACKWARD, fSpeed);
}

//	MoveLeft
//
//	Move the actor left at some specific velocity

int CActorManager::MoveLeft(geActor *theActor, geFloat fSpeed)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	return Move(pEntry, RGF_K_LEFT, fSpeed);
}

//	MoveRight
//
//	Move the actor right at some specific velocity

int CActorManager::MoveRight(geActor *theActor, geFloat fSpeed)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	return Move(pEntry, RGF_K_RIGHT, fSpeed);
}
//	SetAnimationSpeed
//
//	Set the speed at which the actors animation will run, in percent
//	(1.0 is 100% normal speed).

int CActorManager::SetAnimationSpeed(geActor *theActor, geFloat fSpeed)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->ActorAnimationSpeed = fSpeed;
	
	return RGF_SUCCESS;
}


int CActorManager::SetBoxChange(geActor *theActor, bool Flag)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->BoxChange = Flag;
	
	return RGF_SUCCESS;
}


//	SetMotion
//
//	Assign a new motion to an actor instance.  This replaces
//	..the current animation on the next time cycle.

int CActorManager::SetMotion(geActor *theActor, char *MotionName)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	strcpy(pEntry->szMotionName, MotionName);
	pEntry->AnimationTime = 0.0f;		// Clear animation timing
	pEntry->NeedsNewBB = true;
	pEntry->BlendFlag = false;
// changed RF064
	pEntry->TransitionFlag = false;
	
	return RGF_SUCCESS;
}

//	SetNextMotion
//
//	Assign motion to switch to when the current animation cycle
//	..is complete.

int CActorManager::SetNextMotion(geActor *theActor, char *MotionName)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	strcpy(pEntry->szNextMotionName, MotionName);
	pEntry->BlendNextFlag = false;
	
	return RGF_SUCCESS;
}

//	SetDefaultMotion
//
//	Assign the "default motion" for an actor

int CActorManager::SetDefaultMotion(geActor *theActor, char *MotionName)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	strcpy(pEntry->szDefaultMotionName, MotionName);
	
	return RGF_SUCCESS;
}

//	ClearMotionToDefault
//
//	Clear all current and pending motions for an actor and set the
//	..current motion to the default.

int CActorManager::ClearMotionToDefault(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	strcpy(pEntry->szMotionName, pEntry->szDefaultMotionName);
	pEntry->AnimationTime = 0.0f;		// Clear animation timer
	
	pEntry->NeedsNewBB = true;
	
	return RGF_SUCCESS;
}

//	SetActorDynamicLighting
//

int CActorManager::SetActorDynamicLighting(geActor *theActor, GE_RGBA FillColor, GE_RGBA AmbientColor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!

	geVec3d Fill = {0.0f, 1.0f, 0.0f};
	geVec3d_Normalize(&Fill);
	geXForm3d	Xf;
	geXForm3d	XfT;
	geVec3d NewFillNormal;
	geActor_GetBoneTransform(theActor, NULL, &Xf );
	geXForm3d_GetTranspose( &Xf, &XfT );
	geXForm3d_Rotate( &XfT, &Fill, &NewFillNormal );
	geActor_SetLightingOptions(theActor, GE_TRUE, &NewFillNormal, FillColor.r, FillColor.g, FillColor.b,
						AmbientColor.r, AmbientColor.g, AmbientColor.b, GE_TRUE, 6, NULL, GE_FALSE);

	return RGF_SUCCESS;
}

//	SetForce
//
//	Assign a force to one of the four force slots to affect the actor.

int CActorManager::SetForce(geActor *theActor, int nForceNumber, geVec3d fVector, geFloat InitialValue, geFloat Decay)
{
	if((nForceNumber < 0) || (nForceNumber > 3))
		return RGF_FAILURE;						// Bad argument
	
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->ForceVector[nForceNumber] = fVector;
	pEntry->ForceLevel[nForceNumber] = InitialValue;
	pEntry->ForceDecay[nForceNumber] = Decay;
	
	return RGF_SUCCESS;
}

// changed RF063
int CActorManager::GetForce(geActor *theActor, int nForceNumber, geVec3d *ForceVector, geFloat *ForceLevel, geFloat *ForceDecay)
{
	if((nForceNumber < 0) || (nForceNumber > 3))
		return RGF_FAILURE;						// Bad argument
	
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*ForceVector = pEntry->ForceVector[nForceNumber];
	*ForceLevel = pEntry->ForceLevel[nForceNumber];
	*ForceDecay = pEntry->ForceDecay[nForceNumber];
	
	return RGF_SUCCESS;
}
// end change RF063

//	RemoveForce
//
//	Remove a force from an actor

int CActorManager::RemoveForce(geActor *theActor, int nForceNumber)
{
	if((nForceNumber < 0) || (nForceNumber > 3))
		return RGF_FAILURE;						// Bad argument
	
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->ForceLevel[nForceNumber] = 0.0f;
	pEntry->ForceDecay[nForceNumber] = 0.0f;
	
	return RGF_SUCCESS;
}

//	ForceActive
//
//	Returns GE_TRUE if the given force is active, GE_FALSE otherwise.

geBoolean CActorManager::ForceActive(geActor *theActor, int nForceNumber)
{
	if((nForceNumber < 0) || (nForceNumber > 3))
		return RGF_FAILURE;						// Bad argument
	
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	if(pEntry->ForceLevel[nForceNumber] > 0.0f)
		return GE_TRUE;								// Force is active
	
	return GE_FALSE;								// Force inactive
}

int CActorManager::SetColDetLevel(geActor *theActor, int ColDetLevel)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->CollDetLevel = ColDetLevel;
	
	return RGF_SUCCESS;
}

int CActorManager::GetColDetLevel(geActor *theActor, int *ColDetLevel)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*ColDetLevel = pEntry->CollDetLevel;
	
	return RGF_SUCCESS;
}
// changed RF063
int CActorManager::SetBlendMot(geActor *theActor, char *name1, char *name2, float Amount)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	strcpy(pEntry->szMotionName, name1);
	strcpy(pEntry->szBlendMotionName, name2);
	pEntry->BlendAmount = Amount;
	pEntry->NeedsNewBB = true;
	pEntry->BlendFlag = true;
// changed RF064
	pEntry->TransitionFlag = false;

	return RGF_SUCCESS;
}
// end change RF063
int CActorManager::SetBlendMotion(geActor *theActor, char *name1, char *name2, float Amount)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	strcpy(pEntry->szMotionName, name1);
	strcpy(pEntry->szBlendMotionName, name2);
	pEntry->BlendAmount = Amount;
	pEntry->AnimationTime = 0.0f;		// Clear animation timing
	pEntry->NeedsNewBB = true;
	pEntry->BlendFlag = true;
// changed RF064
	pEntry->TransitionFlag = false;

	return RGF_SUCCESS;
}

int CActorManager::SetBlendNextMotion(geActor *theActor, char *name1, char *name2, float Amount)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	strcpy(pEntry->szNextMotionName, name1);
	strcpy(pEntry->szBlendNextMotionName, name2);
	pEntry->BlendNextAmount = Amount;
	pEntry->BlendNextFlag = true;

	return RGF_SUCCESS;
}

// changed RF064
int CActorManager::SetTransitionMotion(geActor *theActor, char *name1, float Amount, char *name2)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;			// Actor not found?!?!

	if(Amount==0.0f)
	{
		if(!EffectC_IsStringNull(name2))
		{
			SetMotion(theActor, name2);
			SetNextMotion(theActor, name1);
		}
		else
			SetMotion(theActor, name1);
	}
	else
	{
		strcpy(pEntry->szBlendMotionName, pEntry->szMotionName);
		strcpy(pEntry->szMotionName, name1);
		pEntry->BlendAmount = Amount;
// changed RF064
		pEntry->StartTime = pEntry->PrevAnimationTime;
// end change RF064
		pEntry->AnimationTime = 0.0f;		// Clear animation timing
		pEntry->NeedsNewBB = true;
		pEntry->TransitionFlag = true;
	}
	return RGF_SUCCESS;
}

bool CActorManager::CheckTransitionMotion(geActor *theActor, char *name1)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return false;			// Actor not found?!?!

	if(pEntry->TransitionFlag)
	{
		if(!strcmp(pEntry->szBlendMotionName, name1))
			return true;
	}
	return false;
}

//	Get GravityCollision structure
//

int CActorManager::GetGravityCollision(geActor *theActor, GE_Collision *Collision)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*Collision = pEntry->GravityCollision;
	
	return RGF_SUCCESS;
}

//	Set HideRadar
//

int CActorManager::SetHideRadar(geActor *theActor, bool flag)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->HideRadar = flag;
	
	return RGF_SUCCESS;
}

//	Get HideRadar
//

int CActorManager::GetHideRadar(geActor *theActor, bool *flag)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*flag = pEntry->HideRadar;
	
	return RGF_SUCCESS;
}

//	Get Group
//

char *CActorManager::GetGroup(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return NULL;					// Actor not found?!?!

	return pEntry->Group;
}

//	Set Group
//

int CActorManager::SetGroup(geActor *theActor, char *name)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	if(!EffectC_IsStringNull(name))
		strcpy(pEntry->Group, name);
	else
		pEntry->Group[0] = '\0';
	
	return RGF_SUCCESS;
}

// end change RF064

//	SetShadow
//
//	Set the shadow of an actor

int CActorManager::SetShadow(geActor *theActor, geFloat fSize)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!

	if(fSize>0.0f)
	{		
		pEntry->ShadowSize = fSize;
	} 
	return RGF_SUCCESS;
}

//	SetScale
//
//	Set the scale of an actor

int CActorManager::SetScale(geActor *theActor, geFloat fScale)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->Scale = fScale;
	
	geActor_SetScale(pEntry->Actor, fScale, fScale, fScale);		// Scale the actor
	
	UpdateActorState(pEntry);
	
	return RGF_SUCCESS;
}

//	GetScale
//
//	Return an actors current scale

int CActorManager::GetScale(geActor *theActor, geFloat *fScale)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*fScale = pEntry->Scale;
	
	return RGF_SUCCESS;
}


//	SetCollision
//
//	Set collision for the desired actor

int CActorManager::SetCollide(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->NoCollision = false;
	geWorld_SetActorFlags(CCD->World(), pEntry->Actor, GE_ACTOR_RENDER_NORMAL | GE_ACTOR_COLLIDE | GE_ACTOR_RENDER_MIRRORS);
	
	return RGF_SUCCESS;
}

//	SetNoCollision
//
//	Set no collision for the desired actor

int CActorManager::SetNoCollide(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->NoCollision = true;
	geWorld_SetActorFlags(CCD->World(), pEntry->Actor, GE_ACTOR_RENDER_NORMAL | GE_ACTOR_RENDER_MIRRORS);
	
	return RGF_SUCCESS;
}

//	SetAlpha
//
//	Set the alpha blend level for the desired actor

int CActorManager::SetAlpha(geActor *theActor, geFloat fAlpha)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	geActor_SetAlpha(pEntry->Actor, fAlpha);
	
	return RGF_SUCCESS;
}

//	GetAlpha
//
//	Get the current alpha blend level for the desired actor

int CActorManager::GetAlpha(geActor *theActor, geFloat *fAlpha)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*fAlpha = geActor_GetAlpha(pEntry->Actor);
	
	return RGF_SUCCESS;
}

//	GetBoundingBox
//
//	Get the current bounding box for the requested actor.

int CActorManager::GetBoundingBox(geActor *theActor, geExtBox *theBox)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	geActor_GetExtBox(pEntry->Actor, theBox);
	
	theBox->Min.X -= pEntry->localTranslation.X;
	theBox->Min.Y -= pEntry->localTranslation.Y;
	theBox->Min.Z -= pEntry->localTranslation.Z;
	theBox->Max.X -= pEntry->localTranslation.X;
	theBox->Max.Y -= pEntry->localTranslation.Y;
	theBox->Max.Z -= pEntry->localTranslation.Z;
	
	return RGF_SUCCESS;
}

//	DoesBoxHitActor
//
//	EVIL HACK!  This should NOT be necessary but, for some reason,
//	..the geWorld_Collision doesn't always work right for some actors
//	..even when the bounding boxes of two actors clearly intersects.
//
//	This function takes an ExtBox and checks to see if it intersects
//	..the ExtBox of any managed actor, and if so, returns a pointer
//	..to that actor - otherwise NULL.  GE_TRUE is returned if there
//	..was a collision, BTW.  Note that the EXTBOX coming in is assumed
//	..to be in MODEL SPACE, we translate it into WORLD SPACE assuming
//	..that the 'thePoint' is the origin of the ExtBox.

geBoolean CActorManager::DoesBoxHitActor(geVec3d thePoint, geExtBox theBox, 
										 geActor **theActor)
{
	geExtBox Result, Temp;
	
	*theActor = NULL;
	
	for(int nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
	{
		if(MainList[nTemp] == NULL)
			continue;				// Empty slot
		ActorInstanceList *pEntry = MainList[nTemp]->IList;
		while(pEntry != NULL)
		{		
			if(pEntry->NoCollision)
			{
				pEntry = pEntry->pNext;
				continue;
			}
			// Get actor instance bounding box in MODEL SPACE
			GetBoundingBox(pEntry->Actor, &Result);
// changed RF064
			//if(CCD->MenuManager()->GetSEBoundBox() && pEntry->Actor!=CCD->Player()->GetActor())
				//DrawBoundBox(CCD->World(), &pEntry->localTranslation,
				//&Result.Min, &Result.Max);
			Result.Min.X += pEntry->localTranslation.X;
			Result.Min.Y += pEntry->localTranslation.Y;
			Result.Min.Z += pEntry->localTranslation.Z;
			Result.Max.X += pEntry->localTranslation.X;
			Result.Max.Y += pEntry->localTranslation.Y;
			Result.Max.Z += pEntry->localTranslation.Z;
			if(memcmp(&Result, &theBox, sizeof(geExtBox)) == 0)
			{
				// We will skip identical bounding boxes since we can
				// ..assume (HEH - EVIL HACK) that they're really the
				// ..same actor.
				pEntry = pEntry->pNext;
				continue;
			}
			if(geExtBox_Intersection(&Result, &theBox, &Temp) == GE_TRUE)
			{
				// Heh, we hit someone.  Return the actor we ran into.
				*theActor = pEntry->Actor;
				return GE_TRUE;
			}
			pEntry = pEntry->pNext;				// Next instance in list
		}
		// Next master instance
	}
	
	//	No hit, all be hunky-dory.
	
	return GE_FALSE;						// Hey, no collisions!
}

//	DoesBoxHitActor
//
//	EVIL HACK!  This should NOT be necessary but, for some reason,
//	..the geWorld_Collision doesn't always work right for some actors
//	..even when the bounding boxes of two actors clearly intersects.
//
//	This function takes an ExtBox and checks to see if it intersects
//	..the ExtBox of any managed actor, and if so, returns a pointer
//	..to that actor - otherwise NULL.  GE_TRUE is returned if there
//	..was a collision, BTW.  Note that the EXTBOX coming in is assumed
//	..to be in MODEL SPACE, we translate it into WORLD SPACE assuming
//	..that the 'thePoint' is the origin of the ExtBox.  Note that this
//	..overload takes a geActor pointer to exclude from the collision
//	..check.

geBoolean CActorManager::DoesBoxHitActor(geVec3d thePoint, geExtBox theBox, 
										 geActor **theActor, geActor *ActorToExclude)
{
	geExtBox Result, Temp;
	
	*theActor = NULL;
	
	for(int nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
	{
		if(MainList[nTemp] == NULL)
			continue;				// Empty slot
		ActorInstanceList *pEntry = MainList[nTemp]->IList;
		while(pEntry != NULL)
		{
			if(pEntry->Actor == ActorToExclude)
			{
				// We want to ignore this actor, do so.
				pEntry = pEntry->pNext;
				continue;
			}
			if(pEntry->NoCollision)
			{
				pEntry = pEntry->pNext;
				continue;
			}
			// Get actor instance bounding box in MODEL SPACE
			GetBoundingBox(pEntry->Actor, &Result);
// changed RF064
			//if(CCD->MenuManager()->GetSEBoundBox() && pEntry->Actor!=CCD->Player()->GetActor())
				//DrawBoundBox(CCD->World(), &pEntry->localTranslation,
				//&Result.Min, &Result.Max);
			Result.Min.X += pEntry->localTranslation.X;
			Result.Min.Y += pEntry->localTranslation.Y;
			Result.Min.Z += pEntry->localTranslation.Z;
			Result.Max.X += pEntry->localTranslation.X;
			Result.Max.Y += pEntry->localTranslation.Y;
			Result.Max.Z += pEntry->localTranslation.Z;
			if(memcmp(&Result, &theBox, sizeof(geExtBox)) == 0)
			{
				// We will skip identical bounding boxes since we can
				// ..assume (HEH - EVIL HACK) that they're really the
				// ..same actor.
				pEntry = pEntry->pNext;
				continue;
			}
			if(geExtBox_Intersection(&Result, &theBox, &Temp) == GE_TRUE)
			{
				// Heh, we hit someone.  Return the actor we ran into.
				*theActor = pEntry->Actor;
				return GE_TRUE;
			}
			pEntry = pEntry->pNext;				// Next instance in list
		}
		// Next master instance
	}
	
	//	No hit, all be hunky-dory.
	
	return GE_FALSE;						// Hey, no collisions!
}

//	SetHealth
//
//	Set the "health level" of an actor.

int CActorManager::SetHealth(geActor *theActor, geFloat fHealth)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->Health = fHealth;
	
	return RGF_SUCCESS;
}

//	ModifyHealth
//
//	Update the current health of an actor by a specified amount

int CActorManager::ModifyHealth(geActor *theActor, geFloat fAmount)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->Health += fAmount;
	
	return RGF_SUCCESS;
}

//	GetHealth
//
//	Return the current health of an actor.

int CActorManager::GetHealth(geActor *theActor, geFloat *fHealth)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*fHealth = pEntry->Health;
	
	return RGF_SUCCESS;
}

//	SetTilt
//
//	Set the Tilt flag for this actor

int CActorManager::SetTilt(geActor *theActor, bool Flag)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->AllowTilt = Flag;
	
	return RGF_SUCCESS;
}

//	SetSlide
//
//	Set the Slide flag for this actor

int CActorManager::SetSlide(geActor *theActor, bool Flag)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->slide = Flag;
	
	return RGF_SUCCESS;
}

//	SetGravity
//
//	Set the gravity force for this actor

int CActorManager::SetGravity(geActor *theActor, geFloat fGravity)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->Gravity = fGravity;
	
	return RGF_SUCCESS;
}

//	SetGravity
//
//	Set the gravity force for this actor

int CActorManager::SetGravityTime(geActor *theActor, geFloat fGravitytime)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	pEntry->GravityTime = fGravitytime;
	
	return RGF_SUCCESS;
}

//	GetGravity
//
//	Get the gravity force for this actor

int CActorManager::GetGravity(geActor *theActor, geFloat *fGravity)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	*fGravity = pEntry->Gravity;
	
	return RGF_SUCCESS;
}

//	IsInFrontOf
//
//	Determine if one actor is "in front of" another.

bool CActorManager::IsInFrontOf(geActor *theActor, geActor *theOtherActor)
{
	ActorInstanceList *pEntry1 = LocateInstance(theActor);
	if(pEntry1 == NULL)
		return false;					// Actor not found?!?!
	
	ActorInstanceList *pEntry2 = LocateInstance(theOtherActor);
	if(pEntry2 == NULL)
		return false;					// Actor not found?!?!
	
	//	Ok, let's go!
	
	geFloat dotProduct;
	geVec3d temp;
	
	//	Subtract positions to normalize
	
	geVec3d_Subtract(&pEntry2->localTranslation, &pEntry1->localTranslation, &temp);
	
	//	Do dotproduct.  If it's positive, then Actor2 is in front of Actor1
	
	geVec3d theRotation = pEntry2->BaseRotation;
	theRotation.X += pEntry2->localRotation.X;
	theRotation.Y += pEntry2->localRotation.Y;
	theRotation.Z += pEntry2->localRotation.Z;
	
	dotProduct = geVec3d_DotProduct(&temp,&theRotation);
	
	if(dotProduct > 0)
		return true;									// Actor2 in front of Actor1
	else
		return false;									// Actor2 behind Actor1
}

//	DistanceFrom
//
//	Determine how far apart two actors are

geFloat CActorManager::DistanceFrom(geActor *theActor, geActor *theOtherActor)
{
	ActorInstanceList *pEntry1 = LocateInstance(theActor);
	if(pEntry1 == NULL)
		return 0.0f;					// Actor not found?!?!
	
	ActorInstanceList *pEntry2 = LocateInstance(theOtherActor);
	if(pEntry2 == NULL)
		return 0.0f;					// Actor not found?!?!
	
	return geVec3d_DistanceBetween(&pEntry1->localTranslation,
								&pEntry2->localTranslation); 
}

//	DistanceFrom
//
//	Determine how far an actor is from a specific point

geFloat CActorManager::DistanceFrom(geVec3d Point, geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	return geVec3d_DistanceBetween(&Point, &pEntry->localTranslation); 
}

//	GetActorsInRange
//
//	Go through the actor database and fill in a list of all actors that are
//	..within a specific range of a specific point.

int CActorManager::GetActorsInRange(geVec3d Point, geFloat Range, int nListSize, geActor **ActorList)
{
	int nOutputCount = 0;
	
	for(int nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
	{
		if(MainList[nTemp] == NULL)
			continue;									// Nothing here, keep going
		ActorInstanceList *pTemp = MainList[nTemp]->IList;
		while(pTemp != NULL)
		{
			// Scan through all the actors and check 'em out.
			if(DistanceFrom(Point, pTemp->Actor) <= Range)
			{
				// This one is IN MODEL RANGE, return it!
				ActorList[nOutputCount++] = pTemp->Actor;
				if(nOutputCount >= nListSize)
					return nListSize;							// Output full, bail the scan
			}
			pTemp = pTemp->pNext;							// Next one
		}
	}
	
	return nOutputCount;									// How many we ended up with
}

//	GetActorZone
//
//	Return the current ZONE TYPE the desired actor is in

int CActorManager::GetActorZone(geActor *theActor, int *ZoneType)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
// Changed RF063	
	*ZoneType = pEntry->CurrentZone;
	
	return RGF_SUCCESS;
}

// Changed RF063
//	GetActorOldZone
//
//	Return the current ZONE TYPE the desired actor is in

int CActorManager::GetActorOldZone(geActor *theActor, int *ZoneType)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!

	*ZoneType = pEntry->OldZone;
	
	return RGF_SUCCESS;
}

void CActorManager::SetMoving(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return;					// Actor not found?!?!
	pEntry->Moving = true;
}

bool CActorManager::GetMoving(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return false;					// Actor not found?!?!
	return pEntry->Moving;
}

Liquid *CActorManager::GetLiquid(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return NULL;					// Actor not found?!?!
	return pEntry->LQ;
}
// end change RF063

// GetMotion
//
// get the current actor animation sequence

char *CActorManager::GetMotion(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return NULL;					// Actor not found?!?!
	return pEntry->szMotionName;
}

geMotion *CActorManager::GetpMotion(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return NULL;					// Actor not found?!?!
	return pEntry->pMotion;
}

geMotion *CActorManager::GetpBMotion(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return NULL;					// Actor not found?!?!
	if(pEntry->BlendFlag || pEntry->TransitionFlag)
		return pEntry->pBMotion;
	return NULL;
}

float CActorManager::GetBlendAmount(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return NULL;					// Actor not found?!?!
	if(pEntry->BlendFlag || pEntry->TransitionFlag)
		return pEntry->BlendAmount;
	return 0.0f;
}

// changed RF064
float CActorManager::GetStartTime(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return 0.0f;					// Actor not found?!?!
	return pEntry->StartTime;
}

bool CActorManager::GetTransitionFlag(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return false;					// Actor not found?!?!
	return pEntry->TransitionFlag;
}
// end change RF064

//	Falling
//
//	Return GE_TRUE if the actor is falling, GE_FALSE if landed.

geBoolean CActorManager::Falling(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
// changed RF063
	if(pEntry->OldZone > 0)
		return GE_FALSE;
// end change RF063
	if(pEntry->GravityTime != 0.0f)
		return GE_TRUE;					// Falling!
	
	return GE_FALSE;					// Not falling
}

//	SetVehicle
//
//	Tell this actor what model he's riding on, if any

void CActorManager::SetVehicle(geActor *theActor, geActor *theVehicle)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return;					// Actor not found?!?!
	
	pEntry->Vehicle = theVehicle;
	
	return;
}

bool CActorManager::IsActor(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return false;					// Actor not found?!?!
	
	return true;
}

//	GetVehicle
//
//	Tell us what model the actor is on, if any

geActor *CActorManager::GetVehicle(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return NULL;					// Actor not found?!?!
	
	return pEntry->Vehicle;
}

//	SetPassenger
//
//	Given an actor to be a passenger, and an actor that's the vehicle,
//	..set the actor to be "riding" the vehicle.

int CActorManager::SetPassenger(geActor *thePassenger, geActor *theVehicle)
{
	ActorInstanceList *pEntry = LocateInstance(thePassenger);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	ActorInstanceList *pRide = LocateInstance(theVehicle);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	for(int nX = 0; nX < PASSENGER_LIST_SIZE; nX++)
	{
		if(pRide->Passengers[nX] == NULL)
		{
			pRide->Passengers[nX] = thePassenger;
			pEntry->Vehicle = theVehicle;
			return RGF_SUCCESS;			// Done!
		}
	}
	
	return RGF_FAILURE;					// This bus is PACKED!
}

//	RemovePassenger
//
//	Given an actor, remove it from the vehicle it's on.

int CActorManager::RemovePassenger(geActor *thePassenger)
{
	ActorInstanceList *pEntry = LocateInstance(thePassenger);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	ActorInstanceList *pRide = LocateInstance(pEntry->Vehicle);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;					// Actor not found?!?!
	
	for(int nX = 0; nX < PASSENGER_LIST_SIZE; nX++)
	{
		if(pRide->Passengers[nX] == thePassenger)
		{
			pRide->Passengers[nX] = NULL;
			pEntry->Vehicle = NULL;
			return RGF_SUCCESS;			// He's outta there
		}
	}
	
	return RGF_FAILURE;					// Yo, he's not in MY cab
}

//	Inventory
//
//	Returns a pointer to the list of persistent attributes used as the
//	..inventory/attribute list for the desired actor.

CPersistentAttributes *CActorManager::Inventory(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return NULL;					// Actor not found?!?!
	return pEntry->Inventory;
}

//	Tick
//
//	Pass time to each instance of an actor active in the system.

void CActorManager::Tick(geFloat dwTicks)
{
	//	Move vehicles first so that we translate all the actors
	//	..that are passengers FIRST...
	
	for(int nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
	{
		// Ok, something in this entry?
		if(MainList[nTemp] != NULL)
			MoveAllVehicles(MainList[nTemp], dwTicks);
	}
	
	//	Ok, now we handle moving the non-vehicle actors
	
	for(nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
	{
		// Ok, something in this entry?
// changed RF064
		if(MainList[nTemp] != NULL)
		{
			geExtBox Result;
			TimeAdvanceAllInstances(MainList[nTemp], dwTicks);
			ActorInstanceList *pEntry = MainList[nTemp]->IList;
			while(pEntry)
			{
				GetBoundingBox(pEntry->Actor, &Result);
				if(CCD->MenuManager()->GetSEBoundBox() && pEntry->Actor!=CCD->Player()->GetActor())
				{
					//DrawBoundBox(CCD->World(), &pEntry->localTranslation,
					//&Result.Min, &Result.Max);
					int TotalStaticBoneCount = geActor_GetBoneCount(pEntry->Actor);
					geExtBox theStaticBoneBox;
					for(int nStatic = 0; nStatic < TotalStaticBoneCount; nStatic++)
					{
						geXForm3d theTransform;
						// The bone bounding box comes back in worldspace coordinates...
						if(geActor_GetBoneExtBoxByIndex(pEntry->Actor, nStatic, &theStaticBoneBox) != GE_TRUE)
							continue;								// Nothing here, skip it

						geXForm3d Attachment;
						int ParentBoneIndex;
						const char *BoneName;
						geBody_GetBone(geActor_GetBody(geActor_GetActorDef(pEntry->Actor)),
						nStatic, &BoneName, &Attachment, &ParentBoneIndex); 
						if(!strcmp(BoneName, "BIP01 SPINE1") || !strcmp(BoneName, "BIP01 PELVIS"))
						{

						geActor_GetBoneTransformByIndex(pEntry->Actor, nStatic, &theTransform);
						// Ok, convert from worldspace to modelspace for the bounding box
						theStaticBoneBox.Min.X -= theTransform.Translation.X;
						theStaticBoneBox.Min.Y -= theTransform.Translation.Y;
						theStaticBoneBox.Min.Z -= theTransform.Translation.Z;
						theStaticBoneBox.Max.X -= theTransform.Translation.X;
						theStaticBoneBox.Max.Y -= theTransform.Translation.Y;
						theStaticBoneBox.Max.Z -= theTransform.Translation.Z;
						DrawBoundBox(CCD->World(), &theTransform.Translation, &theStaticBoneBox.Min, &theStaticBoneBox.Max);

						}
					}
				}
				pEntry = pEntry->pNext;
			}
		}
// end change RF064
	}
	
	return;
}

//	------------------ PRIVATE MEMBER FUNCTIONS --------------

//	RemoveAllActors
//
//	Delete all the actors in the instance list for this loaded
//	..actor entry.

void CActorManager::RemoveAllActors(LoadedActorList *theEntry)
{
	if(theEntry == NULL)
		return;									// Empty entity, bail function
	
	if(theEntry->IList == NULL)
		return;									// Empty list, bail function
	
	ActorInstanceList *pEntry = theEntry->IList;
	ActorInstanceList *pTemp = NULL;
	
	while(pEntry != NULL)
	{
		pTemp = pEntry->pNext;
		pEntry->Inventory->Clear();
		delete pEntry->Inventory;
		delete pEntry;
		pEntry = pTemp;					// On to the next victim
	}
	
	theEntry->IList = NULL;			// Entire list killed
	
	return;
}

//	AddNewInstance
//
//	Adds a new instance to the instance list for the desired loaded
//	..actor entry.

geActor *CActorManager::AddNewInstance(LoadedActorList *theEntry, geActor *OldActor)
{
	ActorInstanceList *pTemp = theEntry->IList;
	ActorInstanceList *NewEntry = new ActorInstanceList;
	memset(NewEntry, 0, sizeof(ActorInstanceList));
	
	//	Ok, let's fill in the new entry
	
	if(OldActor)
		NewEntry->Actor = OldActor;
	else
		NewEntry->Actor = geActor_Create(theEntry->theActorDef);
	NewEntry->theDef = theEntry->theActorDef;
	NewEntry->BaseRotation = theEntry->BaseRotation;
	NewEntry->fAutoStepUp = false;		// Default no auto step up
	NewEntry->MaxStepHeight = 16.0f;	// Actors max step height, in world units
	NewEntry->pNext = NULL;
	NewEntry->AllowTilt = true;
	NewEntry->slide = true;
	NewEntry->TiltX = 0.0f;
// changed RF063
	NewEntry->ShadowSize = 0.0f;
	NewEntry->Moving = false;
	NewEntry->OldZone = 0;
	NewEntry->LQ = NULL;
// end change RF063
// changed RF064
	NewEntry->PassengerModel = NULL;
	NewEntry->TransitionFlag = false;
	NewEntry->HideRadar = false;
	NewEntry->Group[0] = '\0';
// end change RF064
	NewEntry->BoxChange = true;
	NewEntry->BlendFlag = false;
	NewEntry->HoldAtEnd = false;
	NewEntry->CollDetLevel = RGF_COLLISIONLEVEL_1;
	NewEntry->ActorType = ENTITY_GENERIC;		// Generic, for now
	NewEntry->Inventory = new CPersistentAttributes;
	NewEntry->Inventory->AddAndSet("health",1);
	NewEntry->Inventory->SetValueLimits("health", 1, 1);
	NewEntry->ActorAnimationSpeed = 1.0f;
	
	for(int nX = 0; nX < PASSENGER_LIST_SIZE; nX++)
		NewEntry->Passengers[nX] = NULL;			// No passengers
	
	//	Add the actor to the world
// changed RF063	
	geWorld_AddActor(CCD->World(), NewEntry->Actor, 
		GE_ACTOR_RENDER_NORMAL | GE_ACTOR_COLLIDE | GE_ACTOR_RENDER_MIRRORS, 0xffffffff);

	geActor_SetStaticLightingOptions(NewEntry->Actor, GE_TRUE, GE_TRUE, 6);

	//	Get, and set, the axially-aligned bounding box for this actor
	
	geExtBox ExtBox;
	geActor_GetDynamicExtBox(NewEntry->Actor, &ExtBox);
	ExtBox.Min.X -= NewEntry->localTranslation.X;
	ExtBox.Min.Y -= NewEntry->localTranslation.Y;
	ExtBox.Min.Z -= NewEntry->localTranslation.Z;
	ExtBox.Max.X -= NewEntry->localTranslation.X;
	ExtBox.Max.Y -= NewEntry->localTranslation.Y;
	ExtBox.Max.Z -= NewEntry->localTranslation.Z;
	
	geActor_SetExtBox(NewEntry->Actor, &ExtBox, NULL);
	geActor_SetRenderHintExtBox(NewEntry->Actor, &ExtBox, NULL);
	NewEntry->NeedsNewBB = true;

	if(pTemp == NULL)
	{
		// Making the head of the list
		theEntry->IList = NewEntry;
		theEntry->IList->pNext = NULL;
		return NewEntry->Actor;
	}
	else
	{
		while(pTemp->pNext != NULL)
			pTemp = pTemp->pNext;
		pTemp->pNext = NewEntry;
		pTemp->pNext->pNext = NULL;
	}
	
	return NewEntry->Actor;
}

int CActorManager::CountActors()
{
	ActorInstanceList *pTemp = NULL;
	int Count = 0;
	
	for(int nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
	{
		if(MainList[nTemp]!=NULL)
		{
			pTemp = MainList[nTemp]->IList;
			while(pTemp != NULL)
			{
				if(pTemp->Actor)
					Count+=1;
				pTemp = pTemp->pNext;
			}
		}
	}
	return Count;
}

//	LocateInstance
//
//	Scan through the list of instances for this loaded actor entry and
//	..see if the desired handle is in the list.

ActorInstanceList *CActorManager::LocateInstance(geActor *theActor, 
												 LoadedActorList *theEntry)
{
	if(theEntry==NULL)
		return NULL;

	ActorInstanceList *pTemp = theEntry->IList;
	
	while(pTemp != NULL)
	{
		if(pTemp->Actor == theActor)
			return pTemp;								// Found it!
		pTemp = pTemp->pNext;					// Next?
	}
	
	return NULL;
}

//	LocateInstance
//
//	Search ALL loaded actors for this specific instance

ActorInstanceList *CActorManager::LocateInstance(geActor *theActor)
{
	ActorInstanceList *pTemp = NULL;
	
	for(int nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
	{
		pTemp = LocateInstance(theActor, MainList[nTemp]);
		if(pTemp != NULL)
			return pTemp;								// Found it, bail!
	}
	
	return NULL;
}

//	RemoveInstance
//
//	Search ALL loaded actors for this specific instance and KILL IT DEAD!

int CActorManager::RemoveInstance(geActor *theActor)
{
	ActorInstanceList *pTemp = NULL;
	ActorInstanceList *pPrev = NULL;
	
	for(int nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
	{
		ActorInstanceList *pTemp = MainList[nTemp]->IList;
		pPrev = pTemp;
		while(pTemp != NULL)
		{
			if(pTemp->Actor == theActor)
			{
				geWorld_RemoveActor(CCD->World(), pTemp->Actor);
				pTemp->Inventory->Clear();
				delete pTemp->Inventory;
				if((MainList[nTemp]->IList == pTemp))
					MainList[nTemp]->IList = pTemp->pNext;
				pPrev->pNext = pTemp->pNext;// Patch around current
				delete pTemp;								// ZAP the entry
				return RGF_SUCCESS;					// Entry is DEAD!
			}
			pPrev = pTemp;								// Save previous
			pTemp = pTemp->pNext;					// Next?
		}
	}

	return RGF_FAILURE;
}

//	TimeAdvanceAllInstances
//
//	Advance time for all instances of the specific list entry.

void CActorManager::TimeAdvanceAllInstances(LoadedActorList *theEntry,
											geFloat dwTicks)
{
	ActorInstanceList *pTemp = theEntry->IList;
	
	while(pTemp != NULL)
	{
		if(pTemp->ActorType != ENTITY_VEHICLE)
		{
// changed RF063
			pTemp->CurrentZone = GetCurrentZone(pTemp);
			ProcessForce(pTemp, dwTicks);					// Process any forces
			ProcessGravity(pTemp, dwTicks);				// Process any gravity
			AdvanceInstanceTime(pTemp, dwTicks);	// Make time move
			pTemp->Moving = false;
// end change RF063
		}
		pTemp = pTemp->pNext;					// Next?
	}
	
	return;
}

float CActorManager::GetAnimationTime(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return 0.0f;

	return pEntry->PrevAnimationTime;
}

// start multiplayer
void CActorManager::SetAnimationTime(geActor *theActor, float time)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return;
	pEntry->AnimationTime = time;
}
// end multiplayer

void CActorManager::SetHoldAtEnd(geActor *theActor, bool State)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return;
	pEntry->HoldAtEnd = State;
}

bool CActorManager::EndAnimation(geActor *theActor)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return false;
// changed RF064
	if(pEntry->TransitionFlag)
		return false;
// end change RF064
	geMotion *pMotion;
	geFloat time = (((geFloat)pEntry->AnimationTime) / 1000.0f) * pEntry->ActorAnimationSpeed;
	geFloat tStart, tEnd;

	pMotion = geActor_GetMotionByName(pEntry->theDef, pEntry->szMotionName);			// Get the motion name
	if(!pMotion)
		return false;
	geMotion_GetTimeExtents(pMotion, &tStart, &tEnd);

	if(time >= tEnd)
		return true;
	return false;
}

//	AdvanceInstanceTime
//
//	Advance time for a specific instance of an actor

void CActorManager::AdvanceInstanceTime(ActorInstanceList *theEntry,
										geFloat dwTicks)
{
	geMotion *pMotion;
	geMotion *pBMotion;
	geXForm3d thePosition;
	
	if(EffectC_IsStringNull(theEntry->szMotionName))
		pMotion = NULL;
	else
		pMotion = geActor_GetMotionByName(theEntry->theDef,
			theEntry->szMotionName);			// Get the motion name
	if(!pMotion)
	{
		if(!EffectC_IsStringNull(theEntry->szNextMotionName))
		{
			strcpy(theEntry->szMotionName, theEntry->szNextMotionName);
			if(theEntry->BlendNextFlag)
			{
				theEntry->BlendFlag = true;
				theEntry->BlendAmount = theEntry->BlendNextAmount;
				strcpy(theEntry->szBlendMotionName, theEntry->szBlendNextMotionName);
			}
			else
			{
				theEntry->BlendFlag = false;
			}
			strcpy(theEntry->szNextMotionName, "");		// Zap next motion
			theEntry->NeedsNewBB = true;
		}
		else
		{
			if(EffectC_IsStringNull(theEntry->szDefaultMotionName))
				pMotion = NULL;
			else
			{
				strcpy(theEntry->szMotionName, theEntry->szDefaultMotionName);
				theEntry->BlendFlag = false;
				pMotion = geActor_GetMotionByName(theEntry->theDef, theEntry->szMotionName);
			}
		}
	}
	theEntry->pMotion = pMotion;
// changed Rf064
	theEntry->pBMotion = NULL;
// end change RF064
	geFloat time = theEntry->PrevAnimationTime = (((geFloat)theEntry->AnimationTime) / 1000.0f) * theEntry->ActorAnimationSpeed;
	geFloat tStart, tEnd;
	
	if(pMotion)
		geMotion_GetTimeExtents(pMotion, &tStart, &tEnd);
	
	// Perform orienting rotations to properly world-align model
	
	geXForm3d_SetIdentity(&thePosition);		// Initialize
	geXForm3d_RotateZ(&thePosition, theEntry->BaseRotation.Z + theEntry->localRotation.Z);
	geXForm3d_RotateX(&thePosition, theEntry->BaseRotation.X + theEntry->localRotation.X);
	geXForm3d_RotateY(&thePosition, theEntry->BaseRotation.Y + theEntry->localRotation.Y);
	
	// Finally, translate this mother!
	
	geXForm3d_Translate(&thePosition, theEntry->localTranslation.X, 
		theEntry->localTranslation.Y, theEntry->localTranslation.Z);
	
	// Done, adjust the animation, rotation, and translation
	// ..of our unsuspecting actor
	
	if(pMotion)
	{
// changed RF064
		if(theEntry->TransitionFlag)
		{
			tEnd = theEntry->BlendAmount;
			geActor_SetPose(theEntry->Actor, pMotion, 0.0f, &thePosition);
			pBMotion = geActor_GetMotionByName(theEntry->theDef, theEntry->szBlendMotionName);
			if(pBMotion)
			{
				float BM = (theEntry->BlendAmount - time)/theEntry->BlendAmount;
				if(BM<0.0f)
					BM = 0.0f;
				theEntry->pBMotion = pBMotion;
				geActor_BlendPose(theEntry->Actor, pBMotion, theEntry->StartTime, &thePosition, BM);
			}
		}
		else
		{
			geActor_SetPose(theEntry->Actor, pMotion, time, &thePosition);
			if(pMotion && theEntry->BlendFlag)
			{
				pBMotion = geActor_GetMotionByName(theEntry->theDef, theEntry->szBlendMotionName);
				if(pBMotion)
				{
					theEntry->pBMotion = pBMotion;
					geActor_BlendPose(theEntry->Actor, pBMotion, time, &thePosition, theEntry->BlendAmount);
				}
			}
		}
// end change RF064
	}
	else
	{
		geActor_ClearPose(theEntry->Actor, &thePosition);
		theEntry->AnimationTime = 0.0f;
	}
// changed RF063	
	if(theEntry->ShadowSize>0.0f)
	{
		geVec3d	Pos1, Pos2;
		GE_Collision Collision;
		geVec3d right, up;
		GE_LVertex	vertex[4];
		geVec3d Axis[3];
		int major, i;
		geVec3d Impact, normal;
		#define fab(a) (a > 0 ? a : -a)
		
		Pos1 = thePosition.Translation;
		Pos2 = Pos1;
		Pos2.Y -= 30000.0f;
		geWorld_Collision(CCD->World(), NULL, NULL,
			&Pos1, &Pos2, GE_CONTENTS_SOLID_CLIP | GE_CONTENTS_WINDOW, 
			GE_COLLIDE_MODELS, 0x0, NULL, NULL, &Collision);
		//CCD->Collision()->CheckForWCollision(NULL, NULL,
			//Pos1, Pos2, &Collision, theEntry->Actor);
		Impact = Collision.Impact;
		normal = Collision.Plane.Normal;
		float dist = (float)fabs(geVec3d_DistanceBetween(&Pos1, &Impact));
		if(dist<0.0f)
			dist = 0.0f;
		if(dist<(theEntry->ShadowSize*2.0f))
		{
			for(i = 0; i < 4; i++)
			{
				// texture coordinates
				vertex[i].u = 0.0f;
				vertex[i].v = 0.0f;
				// color
				vertex[i].r = 24.0f;
				vertex[i].g = 24.0f;
				vertex[i].b = 24.0f;
// changed RF064
				vertex[i].a = ShadowAlpha;
// end change RF064
			}
			
			vertex[3].u = 1.0f;
			vertex[2].u = 1.0f;
			vertex[2].v = 1.0f;
			vertex[1].v = 1.0f;
			
			for(i = 0; i < 3; i++)
			{
				Axis[i].X = 0.0f;
				Axis[i].Y = 0.0f;
				Axis[i].Z = 0.0f;
			}
			Axis[0].X = 1.0f;
			Axis[1].Y = 1.0f;
			Axis[2].Z = 1.0f;
			
			major = 0;
			
			if(fab(normal.Y) > fab(normal.X))
			{
				major = 1;
				if(fab(normal.Z) > fab(normal.Y))
					major = 2;
			}
			else
			{
				if(fab(normal.Z) > fab(normal.X))
					major = 2;
			}
			
			if(fab(normal.X)==1.0f || fab(normal.Y)==1.0f || fab(normal.Z)==1.0f)
			{
				if ((major == 0 && normal.X > 0) || major == 1)
				{
					right.X = 0.0f;
					right.Y = 0.0f;
					right.Z = -1.0f;
				}
				else if (major == 0)
				{
					right.X = 0.0f;
					right.Y = 0.0f;
					right.Z = 1.0f;
				}
				else 
				{
					right.X = normal.Z;
					right.Y = 0.0f;
					right.Z = 0.0f;
				}
			}
			else 
				geVec3d_CrossProduct(&Axis[major], &normal, &right);
			
			geVec3d_CrossProduct(&normal, &right, &up);
			geVec3d_Normalize(&up);
			geVec3d_Normalize(&right);
			geVec3d_Scale(&right, (theEntry->ShadowSize-(dist/2.0f))/2.0f, &right); 
			geVec3d_Scale(&up, (theEntry->ShadowSize-(dist/2.0f))/2.0f, &up);
			
			geVec3d_MA(&Impact, 0.4f, &normal, &Impact);
			
			//calculate vertices from corners
			vertex[1].X = Impact.X + ((-right.X - up.X));
			vertex[1].Y = Impact.Y + ((-right.Y - up.Y));
			vertex[1].Z = Impact.Z + ((-right.Z - up.Z));
			
			vertex[2].X = Impact.X + ((right.X - up.X));
			vertex[2].Y = Impact.Y + ((right.Y - up.Y));
			vertex[2].Z = Impact.Z + ((right.Z - up.Z));
			
			vertex[3].X = Impact.X + ((right.X + up.X));
			vertex[3].Y = Impact.Y + ((right.Y + up.Y));
			vertex[3].Z = Impact.Z + ((right.Z + up.Z));
			
			vertex[0].X = Impact.X + ((-right.X + up.X));
			vertex[0].Y = Impact.Y + ((-right.Y + up.Y));
			vertex[0].Z = Impact.Z + ((-right.Z + up.Z));
			
			geWorld_AddPolyOnce(CCD->World(),
				vertex,
				4,
// changed RF064
				ShadowBitmap,
// end change RF064
				GE_TEXTURED_POLY,
				GE_RENDER_DO_NOT_OCCLUDE_OTHERS | GE_RENDER_DEPTH_SORT_BF ,
				1.0f);
		}
	}
// end change RF063

	//	Ok, let's check to see if we've hit the end of this motion,
	//	..If so, and we have a next motion, move to it - otherwise
	//	..just loop the current motion.
	
	if(pMotion)
	{
		if(time > tEnd)
		{
			if(!theEntry->HoldAtEnd)
			{
				// Ok, the animation time has elapsed.  Do we have another queued up?
				if(!EffectC_IsStringNull(theEntry->szNextMotionName))
				{
					strcpy(theEntry->szMotionName, theEntry->szNextMotionName);
					if(theEntry->BlendNextFlag)
					{
						theEntry->BlendFlag = true;
						theEntry->BlendAmount = theEntry->BlendNextAmount;
						strcpy(theEntry->szBlendMotionName, theEntry->szBlendNextMotionName);
					}
					else
					{
						theEntry->BlendFlag = false;
					}
					strcpy(theEntry->szNextMotionName, "");		// Zap next motion
					theEntry->NeedsNewBB = true;
				}
				// Clear out the animation time
// changed RF063
				theEntry->AnimationTime = (time - tEnd);//0.0f;
			}
// changed RF064
			if(theEntry->TransitionFlag)
			{
				theEntry->TransitionFlag = false;
				theEntry->AnimationTime = 0.0f;
			}
// end change RF064
		}
		else
			theEntry->AnimationTime += dwTicks;						// Time moves!
	}
	
	//	Ok, adjust the actor position for the case where the animation advance
	//	..has created an intersection problem.  Don't check, of course,
	//	..if there's no animation.
	
	//	EVIL HACK: Due to lame-o problems with sudden intersections between
	//	..dynamically computed bounding boxes and animated world models,
	//	..we're going to force a SINGLE EXTBOX FOR EACH ANIMATION, this
	//	..will change ONLY when the current animation changes.  This will
	//	..make Level 1 collision checking (overall AABB) less accurate, but
	//	..will NOT affect Level 2 (bone-based AABB) accuracy.
	
	if(theEntry->NeedsNewBB && theEntry->BoxChange)
	{
		//	Adjust the bounding box
		geExtBox ExtBox, maxExtBox;
		geXForm3d OldPlace = thePosition;
		float fTimer = 0.0f;
		// EVIL HACK - in order to form a perfect bounding box that will
		// ..cover every position of this animation, we have to quickly
		// ..fan through the entire timescale for the cycle, creating a
		// .."maximal" bounding box that guarantees our actor won't ever
		// ..end up intersecting world geometry, which makes the G3D
		// ..engine totally blow grits.
		geFloat tStart, tEnd;
		if(!pMotion)
		{
			// No motion?  Just use the current cleared pose OBB
			geActor_GetDynamicExtBox(theEntry->Actor, &ExtBox);
			// GetDynamicExtBox returns the bounding box in WORLDSPACE, so
			// ..we need to subtract the origin from it to correct for
			// ..this little "feature".  SetExtBox takes the box in MODEL
			// ..SPACE, FYI.
			ExtBox.Min.X -= theEntry->localTranslation.X;
			ExtBox.Min.Y -= theEntry->localTranslation.Y;
			ExtBox.Min.Z -= theEntry->localTranslation.Z;
			ExtBox.Max.X -= theEntry->localTranslation.X;
			ExtBox.Max.Y -= theEntry->localTranslation.Y;
			ExtBox.Max.Z -= theEntry->localTranslation.Z;
			ExtBox.Min.X -= 4.0f;
			ExtBox.Min.Y = 0.0f;
			ExtBox.Min.Z -= 4.0f;
			ExtBox.Max.X += 4.0f;
			ExtBox.Max.Z += 4.0f;
			geActor_SetExtBox(theEntry->Actor, &ExtBox, NULL);
			// We're going to set the render hint box to match the new
			// ..animation position to help with clipping
			geActor_SetRenderHintExtBox(theEntry->Actor, &ExtBox, NULL);
		}
		else
		{
			// It's a Real Live Animation, we have to go all the way.
			geMotion_GetTimeExtents(pMotion, &tStart, &tEnd);
			memset(&maxExtBox, 0, sizeof(geExtBox));
			while(fTimer < tEnd)
			{
				geActor_SetPose(theEntry->Actor, pMotion, fTimer, &thePosition);
				geActor_GetDynamicExtBox(theEntry->Actor, &ExtBox);
				// GetDynamicExtBox returns the bounding box in WORLDSPACE, so
				// ..we need to subtract the origin from it to correct for
				// ..this little "feature".  SetExtBox takes the box in MODEL
				// ..SPACE, FYI.
				ExtBox.Min.X -= theEntry->localTranslation.X;
				ExtBox.Min.Y -= theEntry->localTranslation.Y;
				ExtBox.Min.Z -= theEntry->localTranslation.Z;
				ExtBox.Max.X -= theEntry->localTranslation.X;
				ExtBox.Max.Y -= theEntry->localTranslation.Y;
				ExtBox.Max.Z -= theEntry->localTranslation.Z;
				ExtBox.Min.X -= 4.0f;
				ExtBox.Min.Y -= 0.05f;
				ExtBox.Min.Z -= 4.0f;
				ExtBox.Max.X += 4.0f;
				ExtBox.Max.Z += 4.0f;
				geExtBox_Union(&maxExtBox, &ExtBox, &maxExtBox);
				fTimer += 0.5f;			// In half-second increments
			}
			geActor_SetPose(theEntry->Actor, pMotion, time, &OldPlace);
			maxExtBox.Min.Y = 0.0f;
			geActor_SetExtBox(theEntry->Actor, &maxExtBox, NULL);
			// We're going to set the render hint box to match the new
			// ..animation position to help with clipping
			geActor_SetRenderHintExtBox(theEntry->Actor, &ExtBox, NULL);
		}
	}
	
	return;
}

//	UpdateActorState
//
//	Updates the actors position in the world to reflect the internal
//	..object state

void CActorManager::UpdateActorState(ActorInstanceList *theEntry)
{
	geMotion *pMotion;
	geXForm3d thePosition;
	
	if(EffectC_IsStringNull(theEntry->szMotionName))
		pMotion = NULL;
	else
		pMotion = geActor_GetMotionByName(theEntry->theDef,
			theEntry->szMotionName);			// Get the motion name
	geFloat time = ((geFloat)theEntry->AnimationTime) / 1000.0f;
	geFloat tStart, tEnd;
	
	if(pMotion)
		geMotion_GetTimeExtents(pMotion, &tStart, &tEnd);
	
	// Perform orienting rotations to properly world-align model
	
	geXForm3d_SetIdentity(&thePosition);		// Initialize
	geXForm3d_RotateZ(&thePosition, theEntry->BaseRotation.Z + theEntry->localRotation.Z);
	geXForm3d_RotateX(&thePosition, theEntry->BaseRotation.X + theEntry->localRotation.X);
	geXForm3d_RotateY(&thePosition, theEntry->BaseRotation.Y + theEntry->localRotation.Y);
	
	// Finally, translate this mother!
	
	geXForm3d_Translate(&thePosition, theEntry->localTranslation.X, 
		theEntry->localTranslation.Y, theEntry->localTranslation.Z);
	
	
	// Done, adjust the animation, rotation, and translation
	// ..of our unsuspecting actor
	
	if(pMotion)
		geActor_SetPose(theEntry->Actor, pMotion, time, &thePosition);
	else
	{
		geActor_ClearPose(theEntry->Actor, &thePosition);
	}
	
	return;
}

//	ProcessGravity
//
//	Perform gravity processing for a specific actor.

void CActorManager::ProcessGravity(ActorInstanceList *theEntry, geFloat dwTicks)
{
	if(theEntry->Gravity == 0.0f)
		return;												// Don't bother, no gravity for this actor
	
	//	Ok, we have gravity to deal with, let's do it!
	
	geFloat	movespeed;
	geFloat	Slide;
	bool    Result;
	GE_Collision Collision;
	geVec3d Up, oldpos, newpos;

	//	If in Zero-G, don't apply gravity!
// changed RF063	
	int nZoneType = theEntry->CurrentZone;

	if(nZoneType == kNoGravityZone || nZoneType & kClimbLaddersZone)
	{
		theEntry->GravityTime = 0.0f;			// Patch up the timings
		return;														// Woah, games in space...
	}

		theEntry->GravityTime += dwTicks;							// Increase it...
		
		//	If we're in water, well, there's no acceleration towards the bottom
		//	..due to water friction and it's slow, so let's simulate it.
// changed RF063
// changed RF064
		if(nZoneType == kLiquidZone && 
			(theEntry->Actor==CCD->Player()->GetActor() || theEntry->ActorType==ENTITY_NPC))
// end change RF064
		{
			if(theEntry->Moving)
			{
				theEntry->GravityTime = 0.0f;			// Patch up the timings
				return;
			}
			movespeed = (theEntry->Gravity * (theEntry->GravityTime / 800.0f) * ((float)theEntry->GravityCoeff/100.0f));					// Slow sinking to bottom
		}
		else
			movespeed = theEntry->Gravity * (theEntry->GravityTime / 800.0f);
// end change RF063
		if(movespeed == 0.0f)
			return;

	geXForm3d Xform;
	geXForm3d_SetIdentity(&Xform);
	geXForm3d_GetUp(&Xform, &Up);				//get our upward vector
	
	oldpos = theEntry->localTranslation;
	
	geVec3d_AddScaled (&oldpos, &Up, movespeed, &newpos); // Move
	
	//	EVIL HACK:  There is some problem with bone-box collision checking
	//	..when doing gravity processing, so for now (or until someone
	//	..finds the problem and fixes it) gravity collision detection is
	//	..at the overall bounding box stage only.
	
	CCD->Collision()->IgnoreContents(false);
	CCD->Collision()->CheckLevel(RGF_COLLISIONLEVEL_1);

// changed RF064
	if(theEntry->PassengerModel)
	{
		CCD->ModelManager()->RemovePassenger(theEntry->PassengerModel, theEntry->Actor);
		theEntry->PassengerModel = NULL;
	}
// end change RF064
	
	int nHitType = kNoCollision;

		Result = CCD->Collision()->CheckActorCollision(oldpos, newpos, theEntry->Actor,
			&Collision);
		
		if(Result == true)					// Your new position collides with something
		{
// changed RF064
			theEntry->GravityCollision = Collision;
// end change RF064
			// Process the collision event.
			nHitType = CCD->Collision()->ProcessCollision(Collision, theEntry->Actor, true);
//geEngine_Printf(CCD->Engine()->Engine(), 0,10,"HitType = %d",nHitType);
//geEngine_Printf(CCD->Engine()->Engine(), 0,20,"Model = %x",Collision.Model);
			// Weapon
			if(nHitType == kNoCollision || nHitType == kCollideWeapon)
			{
				theEntry->localTranslation = newpos;
				UpdateActorState(theEntry);
				return;
			}
			if(nHitType == kCollideTrigger)
			{
				UpdateActorState(theEntry);
				return;
			}
			if(nHitType == kCollideVehicle)
			{
				SetPassenger(theEntry->Actor, Collision.Actor);
				SetVehicle(theEntry->Actor, Collision.Actor);
				theEntry->GravityTime = 0.0f;
				theEntry->localTranslation = Collision.Impact;
				UpdateActorState(theEntry);
				return;
			}
			if(nHitType == kCollidePlatform)
			{
				CCD->ModelManager()->AddPassenger(Collision.Model, theEntry->Actor);
// changed RF064
				theEntry->PassengerModel = Collision.Model;
// end change RF064
				theEntry->GravityTime = 0.0f;
				theEntry->localTranslation = Collision.Impact;
				UpdateActorState(theEntry);
				return;
			}
			if((nHitType == kCollideWorldModel) || (nHitType == kCollideDoor) || (nHitType == kCollideActor)) 
			{
				// Model hit, and dealt with.
				theEntry->GravityTime = 0.0f;							// Same as "hitting the ground"
				theEntry->localTranslation = Collision.Impact;
				UpdateActorState(theEntry);
				return;		
			}
// changed RF063
			if(Collision.Plane.Normal.Y < 0.80f || (nZoneType & kUnclimbableZone)
				|| (nZoneType == kLiquidZone)) // non-climbable slope
// end change RF063
			{
				geVec3d_AddScaled (&newpos, &Up, movespeed, &newpos); // Make it a faster slide
				Slide = geVec3d_DotProduct (&newpos, &Collision.Plane.Normal) - Collision.Plane.Dist;

				newpos.X -= Collision.Plane.Normal.X * Slide;
				newpos.Y -= Collision.Plane.Normal.Y * Slide;
				newpos.Z -= Collision.Plane.Normal.Z * Slide;

				// Check to see if we can get away with sliding a bit when
				// ..we hit whatever we hit...
				if(CCD->Collision()->CheckActorCollision(oldpos, newpos, theEntry->Actor,
					&Collision) == true)
				{
					newpos = Collision.Impact;	// set new position to the point of collision.
				}
				theEntry->GravityTime = 0.0f;
			}
			else // We are on the ground. 
			{
				theEntry->GravityTime = 0.0f;
				newpos = Collision.Impact;
			}
		}

	theEntry->localTranslation = newpos;
	UpdateActorState(theEntry);
	
	return;
}

//	ProcessForce
//
//	Perform force additions for a specific actor

void CActorManager::ProcessForce(ActorInstanceList *theEntry, geFloat dwTicks)
{
	geXForm3d Xform;
	geVec3d newpos;
	
	for(int nItem = 0; nItem < 4; nItem++)
	{
		if(theEntry->ForceLevel[nItem] == 0.0f)
			continue;						// Nothing in this force
		// Ok, we have a force, process it!  Forces act only on translations, we
		// ..don't have any rotational forces... right now...
		geXForm3d_SetIdentity(&Xform);
		geXForm3d_SetXRotation(&Xform, theEntry->localRotation.X +
			theEntry->BaseRotation.X);
		geXForm3d_SetYRotation(&Xform, theEntry->localRotation.Y +
			theEntry->BaseRotation.Y);
		geXForm3d_SetZRotation(&Xform, theEntry->localRotation.Z +
			theEntry->BaseRotation.Z); 
		geXForm3d_Translate(&Xform, theEntry->localTranslation.X, theEntry->localTranslation.Y,
			theEntry->localTranslation.Z);
		geVec3d_AddScaled (&theEntry->localTranslation, &theEntry->ForceVector[nItem], 
			(theEntry->ForceLevel[nItem] * (dwTicks / 200.0f)), &newpos);
		// We have the new position, do a collision check and all that...
		if(ValidateMotion(theEntry->localTranslation, newpos, theEntry, true, theEntry->slide) == GE_FALSE)
		{
			theEntry->ForceDecay[nItem] = 0.0f;
			theEntry->ForceLevel[nItem] = 0.0f;
			continue;
		}

		// Ok, decay the force
		theEntry->ForceLevel[nItem] -= theEntry->ForceDecay[nItem] * (dwTicks / 900.0f);
		if(theEntry->ForceLevel[nItem] < 0.0f)
			theEntry->ForceLevel[nItem] = 0.0f;					// Turn off the force
	}
	
	UpdateActorState(theEntry);
	
	return;
}

//	Move
//
//	Move the actor forward/backward/left/right at a specific
//	..speed.

int CActorManager::Move(ActorInstanceList *pEntry, int nHow, geFloat fSpeed)
{
	geXForm3d Xform;
	geVec3d In, NewPosition, SavedPosition;
	
	SavedPosition = pEntry->localTranslation;			// Save for later.
	
	//	Note that the only thing we're interested in here is the
	//	..DIRECTION we're facing, so we can move forward or backward.
	//	..Ignore the X and Z rotations so we can still move forward
	//	..when we're facing UP.  **OF COURSE** This isn't true when
	//	..we're in the water or in ZeroG
	
	//	11/04/1999 An interesting note - be careful in checking collisions
	//	..with GE_CONTENTS_CANNOT_OCCUPY, "empty" brushes collide with
	//	..that flag.  Use GE_CONTENTS_SOLID_CLIP, empty models don't trigger
	//	..a collision then.
	
	if((nHow == RGF_K_BACKWARD) || (nHow == RGF_K_RIGHT))
		fSpeed = -(fSpeed);										// Moving backward||right here
	
	//	Ok, what kind of substance we're in affects our motion direction,
	//	..so let's deal with that.
// changed RF063	
	int nZoneType = pEntry->CurrentZone;

	//geEngine_Printf(CCD->Engine()->Engine(), 0,10,"Zone = %x",nZoneType);

	if((nZoneType & kClimbLaddersZone))
	{
		geXForm3d_SetIdentity(&Xform);
		if((nHow != RGF_K_FORWARD))
		{
			geXForm3d_RotateZ(&Xform, pEntry->localRotation.Z);
			if(pEntry->AllowTilt)
				geXForm3d_RotateX(&Xform, pEntry->localRotation.X);
			else
				geXForm3d_RotateX(&Xform, pEntry->TiltX);
			geXForm3d_RotateY(&Xform, pEntry->localRotation.Y);
			geXForm3d_Translate(&Xform, pEntry->localTranslation.X, pEntry->localTranslation.Y,
				pEntry->localTranslation.Z);
			if((nHow == RGF_K_BACKWARD))
				geXForm3d_GetIn(&Xform, &In);						// get forward vector 
			else
				geXForm3d_GetLeft(&Xform, &In);
			NewPosition = pEntry->localTranslation;				// From the old position...
			SavedPosition = pEntry->localTranslation;			// Back this up..
			geVec3d_AddScaled(&SavedPosition, &In, fSpeed, &NewPosition);
		}
		else
		{
			geXForm3d_Translate(&Xform, pEntry->localTranslation.X, pEntry->localTranslation.Y,
			pEntry->localTranslation.Z);
			float Tilt;
			if(pEntry->AllowTilt)
				Tilt = pEntry->localRotation.X;
			else
				Tilt = pEntry->TiltX;
			if(Tilt<0.0)
				fSpeed = -(fSpeed);
			geXForm3d_GetUp(&Xform, &In);
			NewPosition = pEntry->localTranslation;				// From the old position...
			SavedPosition = pEntry->localTranslation;			// Back this up..
			geVec3d_AddScaled(&SavedPosition, &In, fSpeed, &NewPosition);
			if(Tilt<0.0)
			{
				GE_Collision Collision;
				if(CCD->Collision()->CheckActorCollision(SavedPosition, NewPosition, pEntry->Actor, &Collision))
				{
					fSpeed = -(fSpeed);
					geXForm3d_SetIdentity(&Xform);
					geXForm3d_RotateY(&Xform, pEntry->localRotation.Y);
					geXForm3d_Translate(&Xform, pEntry->localTranslation.X, pEntry->localTranslation.Y,
						pEntry->localTranslation.Z);
					geXForm3d_GetIn(&Xform, &In);						// get forward vector 
					NewPosition = pEntry->localTranslation;						// From the old position...
					SavedPosition = pEntry->localTranslation;					// Back this up..
					geVec3d_AddScaled(&SavedPosition, &In, fSpeed, &NewPosition);
				}
			}
		}
	}
// changed RF063
	else if((nZoneType == kLiquidZone) || (nZoneType == kNoGravityZone))
	{
		geXForm3d_SetIdentity(&Xform);
		// In water or zero-G, go the direction the actor is facing
		geXForm3d_RotateZ(&Xform, pEntry->localRotation.Z);
		if(pEntry->AllowTilt)
			geXForm3d_RotateX(&Xform, pEntry->localRotation.X);
		else
		{
			if(pEntry->Actor==CCD->Player()->GetActor())
			{
				if(CCD->Player()->GetViewPoint()==FIRSTPERSON)
					geXForm3d_RotateX(&Xform, CCD->CameraManager()->GetTilt());
				else
					geXForm3d_RotateX(&Xform, pEntry->localRotation.X-CCD->CameraManager()->GetTilt());
			}
			else
				geXForm3d_RotateX(&Xform, pEntry->TiltX);
		}
		geXForm3d_RotateY(&Xform, pEntry->localRotation.Y);
		geXForm3d_Translate(&Xform, pEntry->localTranslation.X, pEntry->localTranslation.Y,
			pEntry->localTranslation.Z);
		if((nHow == RGF_K_FORWARD) || (nHow == RGF_K_BACKWARD))
			geXForm3d_GetIn(&Xform, &In);						// get forward vector 
		else
			geXForm3d_GetLeft(&Xform, &In);
		NewPosition = pEntry->localTranslation;				// From the old position...
		SavedPosition = pEntry->localTranslation;			// Back this up..
		geVec3d_AddScaled(&SavedPosition, &In, fSpeed, &NewPosition);
	}
	else 
	{
		geXForm3d_SetIdentity(&Xform);
		geXForm3d_RotateY(&Xform, pEntry->localRotation.Y);
		geXForm3d_Translate(&Xform, pEntry->localTranslation.X, pEntry->localTranslation.Y,
			pEntry->localTranslation.Z);
		if((nHow == RGF_K_FORWARD) || (nHow == RGF_K_BACKWARD))
			geXForm3d_GetIn(&Xform, &In);						// get forward vector 
		else
			geXForm3d_GetLeft(&Xform, &In);
		NewPosition = pEntry->localTranslation;						// From the old position...
		SavedPosition = pEntry->localTranslation;					// Back this up..
		geVec3d_AddScaled(&SavedPosition, &In, fSpeed, &NewPosition); // Move
	}
	
	//	Do a fast probe ahead/behind to see if we need to go further
	
	GE_Collision theCollision;
	bool fStepUpOK = true;
	
	if(CCD->Collision()->Probe(Xform, fSpeed*10, &theCollision) == GE_TRUE)
	{
		if((theCollision.Plane.Normal.Y < 0.3f) &&
			(theCollision.Plane.Normal.Y > 0.0f))
			fStepUpOK = false;				// No stepping up ahead
	}
	
	//	Ok, we've moved.  Check to see if we just ran into something,
	//	..and if so, don't _really_ move.

	if(ValidateMotion(SavedPosition, NewPosition, pEntry, fStepUpOK, pEntry->slide) == GE_TRUE)
	{
		pEntry->OldTranslation = SavedPosition;		// Update old position
		return RGF_SUCCESS;
	}
	
	return RGF_FAILURE;
}

//	CheckForStepUp
//
//	If, during movement, we have a collision,
//	..we need to see if we're just hitting the front face of something
//	..we can step up on.  If so, we need to step up on it!
//
//	eaa3 03/23/2000 - sheesh, what a debug process!  Couldn't strafe
//	..up onto a legal ramp or step, the code that I had here was too
//	..complex and incorrect - new version is shorter, faster, more
//	..accurate, and you can now strafe on ramps and stairs!  Woo-hoo!

bool CActorManager::CheckForStepUp(ActorInstanceList *pEntry, geVec3d NewPosition)
{
	geFloat tempy = 0.0f;
	geVec3d OldPosition;
	geExtBox theExtBox;
	
	// weapon
	if(pEntry->MaxStepHeight == -1.0f)
		return false;
	
	//	Get AABB

	GetBoundingBox(pEntry->Actor, &theExtBox);
	
	OldPosition = pEntry->localTranslation;
	
	while(tempy <= pEntry->MaxStepHeight)
	{
		tempy += 0.5f;
		OldPosition.Y += 0.5f;
		NewPosition.Y += 0.5f;
		// See if there's a clear path now we're elevated...
		if(CCD->Collision()->CheckActorCollision(OldPosition, NewPosition, pEntry->Actor) == false)
		{
			pEntry->localTranslation = NewPosition;
			return true;
		}
		else
		{
			if(CCD->Collision()->CanOccupyPosition(&NewPosition, &theExtBox, 
				pEntry->Actor) == true)
			{
				pEntry->localTranslation = NewPosition;
				return true;
			}
		}
	}
	
	return false;														// No step up.
	
}

//	GetCurrentZone
//
//	Get the brush contents for the actors bounding box, if any.

int CActorManager::GetCurrentZone(ActorInstanceList *pEntry)
{
	GE_Contents ZoneContents;
	int nZoneType;
	geExtBox theExtBox;
// changed RF063
	geFloat MinY, MaxY;
	Liquid *LQ;
	
	//	Get AABB
	
	geActor_GetExtBox(pEntry->Actor, &theExtBox);
	
	theExtBox.Min.X -= pEntry->localTranslation.X;
	MinY = theExtBox.Min.Y -= pEntry->localTranslation.Y;
	theExtBox.Min.Z -= pEntry->localTranslation.Z;
	theExtBox.Max.X -= pEntry->localTranslation.X;
	theExtBox.Max.Y -= pEntry->localTranslation.Y;
	theExtBox.Max.Z -= pEntry->localTranslation.Z;

	pEntry->OldZone = 0;

	theExtBox.Min.Y += (theExtBox.Max.Y-theExtBox.Min.Y)*0.60f;

	if(CCD->Collision()->GetContentsOf(pEntry->localTranslation, &theExtBox, &ZoneContents)==RGF_SUCCESS)
	//if(geWorld_GetContents(CCD->World(), &pEntry->localTranslation, &theExtBox.Min, 
		//&theExtBox.Max, GE_COLLIDE_MODELS, 0, NULL, NULL, &ZoneContents) == GE_TRUE)
	{
		//geEngine_Printf(CCD->Engine()->Engine(), 0,20,"Model = %x",ZoneContents.Model);
		LQ = CCD->Liquids()->IsLiquid(ZoneContents.Model);
		if(LQ)
		{
			pEntry->LQ = LQ;
			pEntry->OldZone = 1;
		}
	}

	theExtBox.Min.Y = MinY + (theExtBox.Max.Y-MinY)*0.80f;

	if(CCD->Collision()->GetContentsOf(pEntry->localTranslation, &theExtBox, &ZoneContents)==RGF_SUCCESS)
	//if(geWorld_GetContents(CCD->World(), &pEntry->localTranslation, &theExtBox.Min, 
		//&theExtBox.Max, GE_COLLIDE_MODELS, 0, NULL, NULL, &ZoneContents) == GE_TRUE)
	{
		LQ = CCD->Liquids()->IsLiquid(ZoneContents.Model);
		if(LQ)
		{
			pEntry->LQ = LQ;
			pEntry->OldZone = 2;
		}
	}

	//geEngine_Printf(CCD->Engine()->Engine(), 0,20,"OldZone = %x",pEntry->OldZone);

	theExtBox.Min.Y = MinY;
	//	Get the contents of that box!
	MaxY = theExtBox.Min.Y += (theExtBox.Max.Y-theExtBox.Min.Y)*0.70f;

	if(CCD->Collision()->GetContentsOf(pEntry->localTranslation, &theExtBox, &ZoneContents)==RGF_SUCCESS)
	//if(geWorld_GetContents(CCD->World(), &pEntry->localTranslation, &theExtBox.Min, 
		//&theExtBox.Max, GE_COLLIDE_MODELS, 0, NULL, NULL, &ZoneContents) == GE_TRUE)
	{
		LQ = CCD->Liquids()->IsLiquid(ZoneContents.Model);
		//geEngine_Printf(CCD->Engine()->Engine(), 0,20,"Model = %x",ZoneContents.Model);
		if(LQ)
		{
			pEntry->LQ = LQ;
			pEntry->GravityCoeff = LQ->GravityCoeff;
			nZoneType = kLiquidZone;
			return nZoneType;
		}
	}

	nZoneType = 0;
	theExtBox.Max.Y = MaxY;
	theExtBox.Min.Y = MinY;
	if(CCD->Collision()->GetContentsOf(pEntry->localTranslation, &theExtBox, &ZoneContents)==RGF_SUCCESS)
	//if(geWorld_GetContents(CCD->World(), &pEntry->localTranslation, &theExtBox.Min, 
		//&theExtBox.Max, GE_COLLIDE_MODELS, 0, NULL, NULL, &ZoneContents) == GE_TRUE)
	{
		LQ = CCD->Liquids()->IsLiquid(ZoneContents.Model);
		if(LQ)
		{
			pEntry->LQ = LQ;
			nZoneType = kInLiquidZone;
		}
	}

// end change RF063
	geActor_GetExtBox(pEntry->Actor, &theExtBox);
	
	theExtBox.Min.X -= pEntry->localTranslation.X;
	theExtBox.Min.Y -= pEntry->localTranslation.Y;
	theExtBox.Min.Z -= pEntry->localTranslation.Z;
	theExtBox.Max.X -= pEntry->localTranslation.X;
	theExtBox.Max.Y -= pEntry->localTranslation.Y;
	theExtBox.Max.Z -= pEntry->localTranslation.Z;

	theExtBox.Min.Y -= 0.5f;
/*
	if(geWorld_GetContents(CCD->World(), &pEntry->localTranslation, &theExtBox.Min, 
		&theExtBox.Max, GE_COLLIDE_MODELS, 0, NULL, NULL, &ZoneContents) == GE_TRUE)
	{
		geWorld *World = CCD->World();
		int j = ZoneContents.Model->BSPModel->FirstFace;
		j = World->CurrentBSP->BSPData.GFXFaces[j].TexInfo;
		int tex = World->CurrentBSP->BSPData.GFXTexInfo[j].Texture;
	}
*/
	if(geWorld_GetContents(CCD->World(), &pEntry->localTranslation, &theExtBox.Min, 
		&theExtBox.Max, GE_COLLIDE_MODELS, 0, NULL, NULL, &ZoneContents) == GE_TRUE)
	{
		unsigned int Mask = 0x00010000;
		for(int i=0;i<16;i++)
		{
			switch(ZoneContents.Contents & (Mask<<i))
			{
			case Water:
				nZoneType |= kWaterZone;
				break;
			case Lava:
				nZoneType |= kLavaZone;
				break;
			case ToxicGas:
				nZoneType |= kToxicGasZone;
				break;
			case ZeroG:
				nZoneType |= kNoGravityZone;
				break;
			case Frozen:
				nZoneType |= kFrozenZone;
				break;
			case Sludge:
				nZoneType |= kSludgeZone;
				break;
			case SlowMotion:
				nZoneType |= kSlowMotionZone;
				break;
			case FastMotion:
				nZoneType |= kFastMotionZone;
				break;
			case Ladder:
				nZoneType |= kClimbLaddersZone;
				break;
			case Impenetrable:
				nZoneType |= kImpassibleZone;
				break;
// changed RF063
			case Unclimbable:
				nZoneType |= kUnclimbableZone;
				break;
// end change RF063
			default:
				nZoneType |= kNormalZone;
				break;
			} 
		}
	}
	else
	{
		// If we're standing on a HOLLOW MODEL, then contents set in the editor
		// ..ARE NOT STORED IN THE BSP!  Damn! So we make use of the MODEL ATTRIBUTES
		// ..entity and we'll check with the Model Manager and see if there are any
		// ..attributes for the model we're on...
		ModelAttributes *theAttributes =
			CCD->ModelManager()->GetAttributes(ZoneContents.Model);
		if(theAttributes == NULL)
			nZoneType = kNormalZone;						// No contents, no problem!
		else
		{
			// Return the appropriate zone for the attributes
			if(theAttributes->Frozen == GE_TRUE)
				nZoneType = kFrozenZone;
			else if(theAttributes->Lava == GE_TRUE)
				nZoneType = kLavaZone;
			else if(theAttributes->Sludge == GE_TRUE)
				nZoneType = kSludgeZone;
			else if(theAttributes->Water == GE_TRUE)
				nZoneType = kWaterZone;
			else if(theAttributes->Ladder == GE_TRUE)
				nZoneType = kClimbLaddersZone;
			else if(theAttributes->ToxicGas == GE_TRUE)
				nZoneType = kToxicGasZone;
			else if(theAttributes->ZeroG == GE_TRUE)
				nZoneType = kNoGravityZone;
			else if(theAttributes->SlowMotion == GE_TRUE)
				nZoneType = kSlowMotionZone;
			else if(theAttributes->FastMotion == GE_TRUE)
				nZoneType = kFastMotionZone;
			else if(theAttributes->Impassible == GE_TRUE)
				nZoneType = kImpassibleZone;
// changed RF063
			else if(theAttributes->UnClimbable == GE_TRUE)
				nZoneType = kUnclimbableZone;
// end change RF063
			delete theAttributes;
		}
	}

	return nZoneType;											// Type to caller
}

geBoolean CActorManager::ValidateMove(geVec3d StartPos, geVec3d EndPos, geActor *theActor, bool slide)
{
	ActorInstanceList *pEntry = LocateInstance(theActor);
	if(pEntry == NULL)
		return RGF_NOT_FOUND;  // Actor not found?!?!
	bool result = ValidateMotion(StartPos, EndPos, pEntry, true, slide);
	if(result == GE_TRUE)
		pEntry->OldTranslation = StartPos;	
	UpdateActorState(pEntry);
	return result;
}

//	ValidateMotion
//
//	Given a START and END position, check to see if the motion is valid,
//	..that is, if no collision occurs.  If a collision does happen, then
//	..process the collision event.

geBoolean CActorManager::ValidateMotion(geVec3d StartPos, geVec3d EndPos,
										ActorInstanceList *pEntry, bool fStepUpOK, bool slide)
{
	GE_Collision Collision;
	geBoolean fMoveOK = GE_TRUE;
	bool Result, fStepUp;
	
	CCD->Collision()->IgnoreContents(false);
	CCD->Collision()->CheckLevel(RGF_COLLISIONLEVEL_1); // RGF_COLLISIONLEVEL_2
	
	//	Start off with the infamous Collision Check.
	
	int nHitType = kNoCollision;
	do
	{
		Result = CCD->Collision()->CheckActorCollision(StartPos, EndPos, pEntry->Actor,
			&Collision);
		
		if(Result == true)
		{
			// Argh, we hit something!  Check to see if its a world model.
			nHitType = CCD->Collision()->ProcessCollision(Collision, pEntry->Actor, false);
			//if(nHitType == kCollideRecheck)
				//continue;
			if(nHitType == kNoCollision || nHitType == kCollideWeapon)
			{
				pEntry->localTranslation = EndPos;
				return GE_TRUE;
			}
			if(nHitType == kCollideDoor)
				return GE_FALSE;					// If we hit an door, exit.
// changed RF064
			//if(nHitType == kCollideActor)
				//return GE_FALSE;
// end change RF064
			if(nHitType == kCollideTrigger)
				return GE_FALSE;  
			if(nHitType == kCollideVehicle)
				SetVehicle(pEntry->Actor, Collision.Actor);
			if(!pEntry->fAutoStepUp)
				return GE_FALSE;					// No auto step up, stop motion
// changed RF063
			int nZoneType = pEntry->CurrentZone;
			if(nZoneType & kUnclimbableZone)
				return GE_FALSE;
// end change RF063
			if(fStepUpOK)
				fStepUp = CheckForStepUp(pEntry, EndPos);
			else
				fStepUp = false;
			if(fStepUp == false)
			{
				if(!(CCD->MenuManager()->GetNoClip()))
				{
					geFloat Slide = 1.0f;
					if(Collision.Plane.Normal.Y < 0.0f)
					{
						Slide = geVec3d_DotProduct(&EndPos, &Collision.Plane.Normal) +
							Collision.Plane.Dist;
						EndPos.X -= Collision.Plane.Normal.X * Slide;
						EndPos.Y -= Collision.Plane.Normal.Y * Slide;
						EndPos.Z -= Collision.Plane.Normal.Z * Slide;
					}
					else
					{
						Slide = geVec3d_DotProduct(&EndPos, &Collision.Plane.Normal) -
							Collision.Plane.Dist;
						EndPos.X -= Collision.Plane.Normal.X * Slide;
						EndPos.Y -= Collision.Plane.Normal.Y * Slide;
						EndPos.Z -= Collision.Plane.Normal.Z * Slide;
					}
					CCD->Collision()->IgnoreContents(false);
					CCD->Collision()->CheckLevel(RGF_COLLISIONLEVEL_1); // RGF_COLLISIONLEVEL_2
					if(CCD->Collision()->CheckActorCollision(StartPos, EndPos, pEntry->Actor,
						&Collision) == true || !slide)
					{
						// No good move, back to where we came from
						pEntry->localTranslation = StartPos;
						return GE_FALSE;
					}
					else 
					{
						// Motion OK, go for it
						pEntry->localTranslation = EndPos;
					}
				} 
				else 
				{
					pEntry->localTranslation = EndPos;
				}
			}
		}
		else
		{
			// Movement is OK, no collision in motion direction.
			pEntry->localTranslation = EndPos;						// Movement is OK with me.
		}
	} while(nHitType==kCollideRecheck);
	
	return GE_TRUE;											// Motion fine and was done.
}

//	MoveAllVehicles
//
//	We need to move the vehicle actors before any other class of
//	..actor gets moved.  This function goes through and takes
//	..care of moving any vehicle-type actors...

void CActorManager::MoveAllVehicles(LoadedActorList *theEntry,
									float dwTicks)
{
	ActorInstanceList *pTemp = theEntry->IList;
	
	while(pTemp != NULL)
	{
		if(pTemp->ActorType == ENTITY_VEHICLE)
		{
			ProcessForce(pTemp, dwTicks);					// Process any forces
			ProcessGravity(pTemp, dwTicks);				// Process any gravity
			AdvanceInstanceTime(pTemp, dwTicks);	// Make time move
			TranslateAllPassengers(pTemp);				// Move any passengers
		}
		pTemp = pTemp->pNext;					// Next?
	}
	
	return;
}

//	TranslateAllPassengers
//
//	This function translates all actors that are marked as
//	..passengers of this vehicle.  As each passenger is translated,
//	..it's removed from the list as gravity and/or other processing
//	..will reset a passenger elsewhere.

int CActorManager::TranslateAllPassengers(ActorInstanceList *pEntry)
{
	geVec3d Delta = pEntry->localTranslation;
	
	Delta.X -= pEntry->OldTranslation.X;
	Delta.Y -= pEntry->OldTranslation.Y;
	Delta.Z -= pEntry->OldTranslation.Z;
	
	for(int nX = 0; nX < PASSENGER_LIST_SIZE; nX++)
	{
		if(pEntry->Passengers[nX] == NULL)
			continue;											// Skip blanks (doh)
		AddTranslation(pEntry->Passengers[nX], Delta);	// Move it!
		pEntry->Passengers[nX] = NULL;	// Been there, done that
	}
	
	return RGF_SUCCESS;
}

int CActorManager::CheckAnimation(geActor *theActor, char *Animation)
{
	
	ActorInstanceList *theEntry = LocateInstance(theActor);
	if(theEntry == NULL)
		return RGF_NOT_FOUND;	
	
	float Height;
	geExtBox theBox;
	
	if(!GetAnimationHeight(theActor, Animation, &Height)) 
		return RGF_NOT_FOUND;
	GetBoundingBox(theActor, &theBox);
	theBox.Max.Y = Height;
// changed RF064
	theBox.Min.Y = 1.0f;
// end change RF064	
	return CCD->Collision()->CanOccupyPosition(&theEntry->localTranslation, &theBox, theEntry->Actor);
	
}


void CActorManager::SetBBox(geActor *theActor, float SizeX, float SizeY, float SizeZ)
{
	geExtBox ExtBox;
	ExtBox.Min.X = -SizeX/2.0f;
// changed RF063
	if(SizeY>=0.0f)
		ExtBox.Min.Y = -SizeY/2.0f;
	else
		ExtBox.Min.Y = 0.0f;
	ExtBox.Min.Z = -SizeZ/2.0f;
	ExtBox.Max.X = SizeX/2.0f;
	ExtBox.Max.Y = (float)fabs(SizeY/2.0f);
// end change RF063
	ExtBox.Max.Z = SizeZ/2.0f;
	geActor_SetExtBox(theActor, &ExtBox, NULL);
	geActor_SetRenderHintExtBox(theActor, &ExtBox, NULL);
}


void CActorManager::SetBoundingBox(geActor *theActor, char *Animation)
{
	geExtBox ExtBox, maxExtBox;
	float fTimer = 0.0f;
	geFloat tStart, tEnd;
	geXForm3d thePosition;
	
	ActorInstanceList *theEntry = LocateInstance(theActor);
	if(theEntry == NULL)
		return;	
	
	geMotion *pMotion = geActor_GetMotionByName(theEntry->theDef, Animation);			// Get the motion name
	if(!pMotion)
	{
		// No motion?  Just use the current cleared pose OBB
		geActor_GetDynamicExtBox(theEntry->Actor, &ExtBox);
		ExtBox.Min.X -= theEntry->localTranslation.X;
		ExtBox.Min.Y -= theEntry->localTranslation.Y;
		ExtBox.Min.Z -= theEntry->localTranslation.Z;
		ExtBox.Max.X -= theEntry->localTranslation.X;
		ExtBox.Max.Y -= theEntry->localTranslation.Y;
		ExtBox.Max.Z -= theEntry->localTranslation.Z;
		ExtBox.Min.X -= 4.0f;
		ExtBox.Min.Y = 0.0f;
		ExtBox.Min.Z -= 4.0f;
		ExtBox.Max.X += 4.0f;
		ExtBox.Max.Z += 4.0f;
		geActor_SetExtBox(theEntry->Actor, &ExtBox, NULL);
		geActor_SetRenderHintExtBox(theEntry->Actor, &ExtBox, NULL);
		return;
	}
	
	geXForm3d_SetIdentity(&thePosition);
	geXForm3d_RotateZ(&thePosition, theEntry->BaseRotation.Z + theEntry->localRotation.Z);
	geXForm3d_RotateX(&thePosition, theEntry->BaseRotation.X + theEntry->localRotation.X);
	geXForm3d_RotateY(&thePosition, theEntry->BaseRotation.Y + theEntry->localRotation.Y);
	geXForm3d_Translate(&thePosition, theEntry->localTranslation.X, 
		theEntry->localTranslation.Y, theEntry->localTranslation.Z);
	
	geMotion_GetTimeExtents(pMotion, &tStart, &tEnd);
	memset(&maxExtBox, 0, sizeof(geExtBox));
	while(fTimer < tEnd)
	{
		geActor_SetPose(theEntry->Actor, pMotion, fTimer, &thePosition);
		geActor_GetDynamicExtBox(theEntry->Actor, &ExtBox);
		ExtBox.Min.X -= theEntry->localTranslation.X;
		ExtBox.Min.Y -= theEntry->localTranslation.Y;
		ExtBox.Min.Z -= theEntry->localTranslation.Z;
		ExtBox.Max.X -= theEntry->localTranslation.X;
		ExtBox.Max.Y -= theEntry->localTranslation.Y;
		ExtBox.Max.Z -= theEntry->localTranslation.Z;
		ExtBox.Min.X -= 4.0f;
		ExtBox.Min.Y -= 0.05f;
		ExtBox.Min.Z -= 4.0f;
		ExtBox.Max.X += 4.0f;
		ExtBox.Max.Z += 4.0f;
		geExtBox_Union(&maxExtBox, &ExtBox, &maxExtBox);
		fTimer += 0.5f;			// In half-second increments
	}
	if(maxExtBox.Min.X<maxExtBox.Min.Z)
		maxExtBox.Min.X = maxExtBox.Min.Z;
	else
		maxExtBox.Min.Z = maxExtBox.Min.X;
	if(maxExtBox.Max.X<maxExtBox.Max.Z)
		maxExtBox.Max.X = maxExtBox.Max.Z;
	else
		maxExtBox.Max.Z = maxExtBox.Max.X;
	maxExtBox.Min.Y = 0.0f;
	geActor_SetExtBox(theEntry->Actor, &maxExtBox, NULL);
	geActor_SetRenderHintExtBox(theEntry->Actor, &maxExtBox, NULL);
	return;
}

int CActorManager::GetAnimationHeight(geActor *theActor, char *Animation, float *Height)
{
	
	ActorInstanceList *theEntry = LocateInstance(theActor);
	if(theEntry == NULL)
		return RGF_NOT_FOUND;	
	
	geMotion *pMotion = geActor_GetMotionByName(theEntry->theDef, Animation);			// Get the motion name
	if(!pMotion)
		return RGF_NOT_FOUND;
	
	geExtBox ExtBox, maxExtBox;
	float fTimer = 0.0f;
	geFloat tStart, tEnd;
	geXForm3d thePosition;
	
	geXForm3d_SetIdentity(&thePosition);
	geXForm3d_RotateZ(&thePosition, theEntry->BaseRotation.Z + theEntry->localRotation.Z);
	geXForm3d_RotateX(&thePosition, theEntry->BaseRotation.X + theEntry->localRotation.X);
	geXForm3d_RotateY(&thePosition, theEntry->BaseRotation.Y + theEntry->localRotation.Y);
	geXForm3d_Translate(&thePosition, theEntry->localTranslation.X, 
		theEntry->localTranslation.Y, theEntry->localTranslation.Z);
	
	geMotion_GetTimeExtents(pMotion, &tStart, &tEnd);
	memset(&maxExtBox, 0, sizeof(geExtBox));
	while(fTimer < tEnd)
	{
		geActor_SetPose(theEntry->Actor, pMotion, fTimer, &thePosition);
		geActor_GetDynamicExtBox(theEntry->Actor, &ExtBox);
		ExtBox.Min.X -= theEntry->localTranslation.X;
		ExtBox.Min.Y -= theEntry->localTranslation.Y;
		ExtBox.Min.Z -= theEntry->localTranslation.Z;
		ExtBox.Max.X -= theEntry->localTranslation.X;
		ExtBox.Max.Y -= theEntry->localTranslation.Y;
		ExtBox.Max.Z -= theEntry->localTranslation.Z;
		ExtBox.Min.X -= 4.0f;
		ExtBox.Min.Y -= 0.05f;
		ExtBox.Min.Z -= 4.0f;
		ExtBox.Max.X += 4.0f;
		ExtBox.Max.Z += 4.0f;
		geExtBox_Union(&maxExtBox, &ExtBox, &maxExtBox);
		fTimer += 0.5f;			// In half-second increments
	}
	pMotion = geActor_GetMotionByName(theEntry->theDef,
		theEntry->szMotionName);
	geFloat time = (((geFloat)theEntry->AnimationTime) / 1000.0f) * theEntry->ActorAnimationSpeed;
	geActor_SetPose(theEntry->Actor, pMotion, time, &thePosition);
	*Height = maxExtBox.Max.Y;
	return RGF_SUCCESS;
	
}

int CActorManager::SetAnimationHeight(geActor *theActor, char *Animation, bool Camera)
{
	
	ActorInstanceList *theEntry = LocateInstance(theActor);
	if(theEntry == NULL)
		return RGF_NOT_FOUND;	
	
	geMotion *pMotion = geActor_GetMotionByName(theEntry->theDef, Animation);			// Get the motion name
	if(!pMotion)
		return RGF_NOT_FOUND;
	
	geExtBox ExtBox, maxExtBox;
	float fTimer = 0.0f;
	geFloat tStart, tEnd;
	geXForm3d thePosition;
	
	geXForm3d_SetIdentity(&thePosition);
	geXForm3d_RotateZ(&thePosition, theEntry->BaseRotation.Z + theEntry->localRotation.Z);
	geXForm3d_RotateX(&thePosition, theEntry->BaseRotation.X + theEntry->localRotation.X);
	geXForm3d_RotateY(&thePosition, theEntry->BaseRotation.Y + theEntry->localRotation.Y);
	geXForm3d_Translate(&thePosition, theEntry->localTranslation.X, 
		theEntry->localTranslation.Y, theEntry->localTranslation.Z);
	
	geMotion_GetTimeExtents(pMotion, &tStart, &tEnd);
	memset(&maxExtBox, 0, sizeof(geExtBox));
	while(fTimer < tEnd)
	{
		geActor_SetPose(theEntry->Actor, pMotion, fTimer, &thePosition);
		geActor_GetDynamicExtBox(theEntry->Actor, &ExtBox);
		ExtBox.Min.X -= theEntry->localTranslation.X;
		ExtBox.Min.Y -= theEntry->localTranslation.Y;
		ExtBox.Min.Z -= theEntry->localTranslation.Z;
		ExtBox.Max.X -= theEntry->localTranslation.X;
		ExtBox.Max.Y -= theEntry->localTranslation.Y;
		ExtBox.Max.Z -= theEntry->localTranslation.Z;
		ExtBox.Min.X -= 4.0f;
		ExtBox.Min.Y -= 0.05f;
		ExtBox.Min.Z -= 4.0f;
		ExtBox.Max.X += 4.0f;
		ExtBox.Max.Z += 4.0f;
		geExtBox_Union(&maxExtBox, &ExtBox, &maxExtBox);
		fTimer += 0.5f;			// In half-second increments
	}
	pMotion = geActor_GetMotionByName(theEntry->theDef,
		theEntry->szMotionName);
	geFloat time = (((geFloat)theEntry->AnimationTime) / 1000.0f) * theEntry->ActorAnimationSpeed;
	geActor_SetPose(theEntry->Actor, pMotion, time, &thePosition);
	
	GetBoundingBox(theEntry->Actor, &ExtBox);
	ExtBox.Max.Y = maxExtBox.Max.Y;
	geActor_SetExtBox(theEntry->Actor, &ExtBox, NULL);
	geActor_SetRenderHintExtBox(theEntry->Actor, &ExtBox, NULL);
	if(Camera)
	{
// changed RF064
		float DesiredHeight;
		switch(CCD->Player()->PlayerViewPoint())
		{
		case FIRSTPERSON:
			if(CCD->CameraManager()->GetViewHeight()!=-1)
			{
				DesiredHeight = ((ExtBox.Max.Y * 
					CCD->CameraManager()->GetViewHeight())/CCD->Player()->GetHeight())
					* 0.75f;
			}
			else
			{
				DesiredHeight = ExtBox.Max.Y * 0.75f;
			}
			break;
		case THIRDPERSON:
			if(CCD->CameraManager()->GetPlayerHeight()!=-1)
			{
				DesiredHeight = ((ExtBox.Max.Y * 
					CCD->CameraManager()->GetPlayerHeight())/CCD->Player()->GetHeight());
			}
			else
			{
				DesiredHeight = ExtBox.Max.Y; 
			}
			break;
		case ISOMETRIC:
			if(CCD->CameraManager()->GetIsoHeight()!=-1)
			{
				DesiredHeight = ((ExtBox.Max.Y * 
					CCD->CameraManager()->GetIsoHeight())/CCD->Player()->GetHeight());
			}
			else
			{
				DesiredHeight = ExtBox.Max.Y;
			}
			break;
		}
		CCD->CameraManager()->SetDesiredHeight(DesiredHeight);
// end change RF064	
		CCD->CameraManager()->TrackMotion();
	}
	return RGF_SUCCESS;
	
}

// changed RF064
geBoolean CActorManager::DoesRayHitActor(geVec3d OldPosition, geVec3d NewPosition, 
		geActor **theActor, geActor *ActorToExclude, geFloat *Percent, geVec3d *Normal)
{
	geExtBox Result;
	geFloat T;
	geVec3d Norm;
	
	*theActor = NULL;
	
	for(int nTemp = 0; nTemp < ACTOR_LIST_SIZE; nTemp++)
	{
		if(MainList[nTemp] == NULL)
			continue;				// Empty slot
		ActorInstanceList *pEntry = MainList[nTemp]->IList;
		while(pEntry != NULL)
		{
			if(pEntry->Actor == ActorToExclude)
			{
				// We want to ignore this actor, do so.
				pEntry = pEntry->pNext;
				continue;
			}
			if(pEntry->NoCollision)
			{
				pEntry = pEntry->pNext;
				continue;
			}
			// Get actor instance bounding box in MODEL SPACE
			GetBoundingBox(pEntry->Actor, &Result);
			Result.Min.X += pEntry->localTranslation.X;
			Result.Min.Y += pEntry->localTranslation.Y;
			Result.Min.Z += pEntry->localTranslation.Z;
			Result.Max.X += pEntry->localTranslation.X;
			Result.Max.Y += pEntry->localTranslation.Y;
			Result.Max.Z += pEntry->localTranslation.Z;

			if(geExtBox_RayCollision(&Result, &OldPosition, &NewPosition,
				&T, &Norm)==GE_TRUE)
			{
				// Heh, we hit someone.  Return the actor we ran into.
				*theActor = pEntry->Actor;
				*Percent = T;
				*Normal = Norm;
				return GE_TRUE;
			}
			pEntry = pEntry->pNext;				// Next instance in list
		}
		// Next master instance
	}
	
	//	No hit, all be hunky-dory.
	
	return GE_FALSE;						// Hey, no collisions!
}
// end change RF064