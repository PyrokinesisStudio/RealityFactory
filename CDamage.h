/************************************************************************************//**
 * @file CDamage.h
 * @brief Damage class handler
 *
 * This file contains the class declaration for damage handling.
 * @author Ralph Deane
 *//*
 * Copyright (c) 2001 Ralph Deane; All rights reserved.
 ****************************************************************************************/

#ifndef __RGF_CDAMAGE_H_
#define __RGF_CDAMAGE_H_

class CDamage : public CRGFComponent
{
public:
	CDamage();
	~CDamage();

	void Tick(float dwTicks);
// changed RF063
	int SaveTo(FILE *SaveFD, bool type);
	int RestoreFrom(FILE *RestoreFD, bool type);

	void DamageActor(geActor *Actor, float amount, char *Attr, float Altamount, char *AltAttr, char *name);
	void DamageActorInRange(geVec3d Point, geFloat Range, float amount, char *Attr, float Altamount, char *AltAttr, char *name);

	void DamageModel(geWorld_Model *Model, float amount, char *Attr, float Altamount, char *AltAttr);
	void DamageModelInRange(geVec3d Point, geFloat Range, float amount, char *Attr, float Altamount, char *AltAttr);
// end change RF063

	bool IsDestroyable(geWorld_Model *Model, int *Percentage);

	int ReSynchronize();
	int LocateEntity(char *szName, void **pEntityData);
private:

};

#endif

/* ----------------------------------- END OF FILE ------------------------------------ */
