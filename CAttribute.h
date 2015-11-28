/************************************************************************************//**
 * @file CAttribute.h
 * @brief Attribute class handler
 *
 * This file contains the class declaration for Attribute handling.
 * @author Ralph Deane
 *//*
 * Copyright (c) 2001 Ralph Deane; All rights reserved.
 ****************************************************************************************/

#ifndef __RGF_CATTRIBUTE_H_
#define __RGF_CATTRIBUTE_H_

class CAttribute : public CRGFComponent
{
public:
	CAttribute();
	~CAttribute();

	void Tick(float dwTicks);
// changed QD 08/13/03 added UseKey
	bool HandleCollision(geActor *theTarget, geActor *pActor, bool UseKey);
// end change 08/13/03
// changed RF063
	int SaveTo(FILE *SaveFD, bool type);
	int RestoreFrom(FILE *RestoreFD, bool type);
// end change RF063
	int ReSynchronize();
	int LocateEntity(char *szName, void **pEntityData);
};

#endif

/* ----------------------------------- END OF FILE ------------------------------------ */
