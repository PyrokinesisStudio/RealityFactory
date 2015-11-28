
#include "RabidFramework.h"

CArmour::CArmour() 
{
	ListPtr = -1;
	CIniFile AttrFile("armour.ini");
	if(!AttrFile.ReadFile())
		return;
	CString KeyName = AttrFile.FindFirstKey();
	CString Type, Value;
	geVec3d Vector;
	char szName[64];
	while(KeyName != "")
	{
		ListPtr +=1;
		if(ListPtr>=MAXARMOUR)
		{
			ListPtr = MAXARMOUR-1;
			break;
		}
		List[ListPtr].Name = strdup(KeyName);
		for(int i=0;i<MAXDAMAGEBY;i++)
			List[ListPtr].DamageBy[i] = NULL;
		List[ListPtr].Attribute = NULL;
		Type = AttrFile.FindFirstName(KeyName);
		Value = AttrFile.FindFirstValue();
		int j = 0;
		while(Type!="")
		{
			if(j>=MAXDAMAGEBY)
				break;
			if(Type=="attribute")
			{
				List[ListPtr].Attribute = strdup(Value);
			}
			else
			{
				List[ListPtr].DamageBy[j] = strdup(Type);
				strcpy(szName,Value);
				Vector = Extract(szName);
				List[ListPtr].Protection[j] = (int)Vector.X;
				List[ListPtr].DamageTo[j] = (int)Vector.Y;
				j+=1;
			}
			Type = AttrFile.FindNextName();
			Value = AttrFile.FindNextValue();
		}

		KeyName = AttrFile.FindNextKey();
	}
}

CArmour::~CArmour()
{
	if(ListPtr != -1)
	{
		for(int i=0;i<=ListPtr;i++)
		{
			free(List[i].Name);
			if(List[i].Attribute)
				free(List[i].Attribute);
			for(int j=0;j<MAXDAMAGEBY;j++)
			{
				if(List[i].DamageBy[j])
					free(List[i].DamageBy[j]);
			}
		}
	}
}

void CArmour::DisableHud(char *Attr)
{
	int i;

	if(ListPtr != -1)
	{
		for(i=0;i<=ListPtr;i++)
		{
			if(!strcmp(Attr, List[i].Name))
			{
				for(int j=0;j<=ListPtr;j++)
				{
					if(CCD->Player()->GetUseAttribute(List[j].Name))
					{
						CCD->Player()->DelUseAttribute(List[j].Name);
						CCD->HUD()->ActivateElement(List[j].Name, false);
					}
				}
				break;
			}
		}
	}
}

int CArmour::AdjustDamage(int Amount, char *name, char *Attr)
{
	int i, Actual;

	if(ListPtr == -1)
		return Amount;
	for(i=0;i<=ListPtr;i++)
	{
		if(CCD->Player()->GetUseAttribute(List[i].Name) && List[i].Attribute)
		{
			if(!strcmp(Attr, List[i].Attribute))
			{
				for(int j=0;j<MAXDAMAGEBY;j++)
				{
					if(List[i].DamageBy[j])
					{
						if(!strcmp(name, List[i].DamageBy[j]))
						{
							CPersistentAttributes *theInv = CCD->ActorManager()->Inventory(CCD->Player()->GetActor());
							int Protect = ((Amount*List[i].Protection[j])/100);
							if(theInv->Value(List[i].Name)<Protect)
								Protect = theInv->Value(List[i].Name);
							Actual = Amount - Protect;
							if(Actual<0)
								Actual = 0;

							theInv->Modify(List[i].Name, -((Amount*List[i].DamageTo[j])/100));
							if(theInv->Value(List[i].Name)<=theInv->Low(List[i].Name))
							{
								CCD->Player()->DelUseAttribute(List[i].Name);
								CCD->HUD()->ActivateElement(List[i].Name, false);
							}
							return Actual;
						}
					}
				}
			}
		}
	}
	return Amount;
}



CLiftBelt::CLiftBelt()
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	Change = false;

	pSet = geWorld_GetEntitySet(CCD->World(), "LiftBelt");
	if(pSet)
	{
		for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	    pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
		{
			pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL);
			LiftBelt *pSource = (LiftBelt*)geEntity_GetUserData(pEntity);
			if(EffectC_IsStringNull(pSource->szEntityName))
			{
				char szName[128];
				geEntity_GetName(pEntity, szName, 128);
				pSource->szEntityName = szName;
			}
			// Ok, put this entity into the Global Entity Registry
			CCD->EntityRegistry()->AddEntity(pSource->szEntityName, "LiftBelt");
			
			pSource->Powerlevel = 0.0f;
			pSource->active = false;
			pSource->Fuel = 0.0f;
			pSource->bState = false;
		}
	}
}

CLiftBelt::~CLiftBelt()
{
}

void CLiftBelt::Tick(float dwTicks)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	geVec3d fVector = {0.0f, 1.0f, 0.0f};

	pSet = geWorld_GetEntitySet(CCD->World(), "LiftBelt");
	if(pSet)
	{
		for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	    pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
		{
			pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL);
			LiftBelt *pSource = (LiftBelt*)geEntity_GetUserData(pEntity);
			
			CPersistentAttributes *theInv = CCD->ActorManager()->Inventory(CCD->Player()->GetActor());
			
			pSource->bState = false;
			if(!EffectC_IsStringNull(pSource->EnableAttr) || pSource->EnableAlways)
			{
				if(CCD->Player()->GetUseAttribute(pSource->EnableAttr) || pSource->EnableAlways)
				{
					if(!pSource->active)
					{
						if(!EffectC_IsStringNull(pSource->PowerAttr))
						{
							pSource->Fuel = (float)theInv->Value(pSource->PowerAttr);
							CCD->HUD()->ActivateElement(pSource->PowerAttr, true);
							if(pSource->Fuel>0)
							{
								pSource->active = true;
								pSource->Powerlevel = 0.0f;
							}
						}
						else
						{
							pSource->active = true;
							pSource->Powerlevel = 0.0f;
						}
					}
					if(pSource->active)
					{
						if(!EffectC_IsStringNull(pSource->PowerAttr))
						{
							if(pSource->Fuel==0.0f)
							{
								pSource->active = false;
								CCD->ActorManager()->RemoveForce(CCD->Player()->GetActor(), 3);
								Change = false;
								return;
							}
							if(pSource->Powerlevel>0.0f)
							{
								pSource->Fuel -= (pSource->PowerUse*dwTicks*0.001f);
								if(pSource->Fuel<0.0f)
									pSource->Fuel = 0.0f;
								theInv->Set(pSource->PowerAttr, (int)pSource->Fuel);
							}
						}
						pSource->bState = true;
						if(Change)
						{
							if(Increase)
							{
								if(pSource->Powerlevel<pSource->LiftForce)
								{
									pSource->Powerlevel += (pSource->AccelRate*dwTicks*0.001f);
									if(pSource->Powerlevel>pSource->LiftForce)
										pSource->Powerlevel = pSource->LiftForce;
									CCD->ActorManager()->SetForce(CCD->Player()->GetActor(), 3, fVector, pSource->Powerlevel, pSource->Powerlevel/2.0f);
								}
							}
							else
							{
								pSource->Powerlevel = 0.0f;
								if(pSource->DropFast)
									CCD->ActorManager()->RemoveForce(CCD->Player()->GetActor(), 3);
							}
						}

						if(pSource->Powerlevel>0.0f)
							CCD->ActorManager()->SetGravityTime(CCD->Player()->GetActor(), 0.0f);
						//geEngine_Printf(CCD->Engine()->Engine(), 0,30,"Force = %f",pSource->Powerlevel);
					}
					Change = false;
					return;
				}
				else
				{
					if(pSource->active)
					{
						pSource->active = false;
						CCD->ActorManager()->RemoveForce(CCD->Player()->GetActor(), 3);
					}
				}
			}
		}
	}
	Change = false;
}

void CLiftBelt::ChangeLift(bool increase)
{
	Change = true;
	Increase = increase;
}

void CLiftBelt::DisableHud(char *Attr)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	bool IsLift = false;

	pSet = geWorld_GetEntitySet(CCD->World(), "LiftBelt");
	if(pSet)
	{
		for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	    pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
		{
			pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL);
			LiftBelt *pSource = (LiftBelt*)geEntity_GetUserData(pEntity);
			if(!EffectC_IsStringNull(pSource->EnableAttr))
			{
				if(!strcmp(Attr, pSource->EnableAttr))
					IsLift = true;
			}
		}
		if(IsLift)
		{
			pSet = geWorld_GetEntitySet(CCD->World(), "LiftBelt");
			for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
			pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
			{
				pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL);
				LiftBelt *pSource = (LiftBelt*)geEntity_GetUserData(pEntity);
				if(!EffectC_IsStringNull(pSource->EnableAttr))
				{
					if(CCD->Player()->GetUseAttribute(pSource->EnableAttr))
					{
						CCD->Player()->DelUseAttribute(pSource->EnableAttr);
						CCD->HUD()->ActivateElement(pSource->EnableAttr, false);
						if(pSource->active)
						{
							pSource->active = false;
							CCD->ActorManager()->RemoveForce(CCD->Player()->GetActor(), 3);
						}
					}
				}
			}
		}
	}
}

//	LocateEntity
//
//	Given a name, locate the desired item in the currently loaded level
//	..and return it's user data.

int CLiftBelt::LocateEntity(char *szName, void **pEntityData)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

//	Ok, check to see if there are liftbelts in this world

	pSet = geWorld_GetEntitySet(CCD->World(), "LiftBelt");

	if(!pSet)
		return RGF_NOT_FOUND;									// No 3D audio sources

//	Ok, we have liftbelts somewhere.  Dig through 'em all.

	for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	    pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity))
		{
		LiftBelt *pSource = (LiftBelt*)geEntity_GetUserData(pEntity);

		if(strcmp(pSource->szEntityName, szName) == 0)
		  {
			*pEntityData = (void *)pSource;
			return RGF_SUCCESS;
			}
		}

  return RGF_NOT_FOUND;								// Sorry, no such entity here
}