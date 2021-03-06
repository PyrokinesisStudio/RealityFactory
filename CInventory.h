/************************************************************************************//**
 * @file CInventory.h
 * @brief Inventory class handler
 *
 * This file contains the class declaration for Inventory handling.
 * @author Ralph Deane
 *//*
 * Copyright (c) 2000 Ralph Deane; All rights reserved.
 ****************************************************************************************/

#ifndef __RGF_CINVENTORY_H_
#define __RGF_CINVENTORY_H_

#define MAXENTRIES 128

typedef enum
{
	WEAPONINV = 0,  //don't change this order
	ITEMINV,
	NUMOFSUB,
	MAININV,
	INVWEAPON,
	INVUSE,
	INVSTATIC,
	INVLOAD,
	INVSAVE,
	INVGRAPHIC,
	INVMODIFY
};

/**
 * @brief Inventory item definition
 */
typedef struct InvItem
{
	//InvItem *next;
	//InvItem *prev;
	int			index;
	int			catagory;
	char		Attribute[64];
	geBitmap	*Image;
	int			Amount;
	char		Modify[64];
	char		text[64];
	bool		Decrease;
	geBitmap	*Graphic;

} InvItem;

/**
 * @brief CInventory handles Inventory display
 */
class CInventory : public CRGFComponent
{
public:
	CInventory();
	~CInventory();

	void Display();
// changed RF064
	void SetActive(bool flag);
	bool GetStopTime()	{ return !isactive || (isactive && !stoptime); }
// end change RF064

private:

	void LoadInventoryIni();

	void DisplaySubInv(int menu);
	void Blit(int menu);

	geBitmap *GetImage(int index, int menu);
	geBitmap *GetGraphic(int index, int menu);

	int GetAmount(int index, int menu);
	int GetCatagory(int index, int menu);

	char *GetAttribute(int index, int menu);
	char *GetModified(int index, int menu);

	int GetModifiedAmount(int index, int menu);

	bool GetDecrease(int index, int menu);

	char *GetText(int index, int menu);

	void GenerateList(int menu);

private:
	bool active;
	geBitmap *background;
	geBitmap *highlight;
	geBitmap *arrowr;
	geBitmap *arrowl;
	geBitmap *arrowrhighlight;
	geBitmap *arrowlhighlight;
	int backgroundx, backgroundy;
	int leftx;//, lefty;
	int middlex, middley;
	int rightx;//, righty;
	int topy;
	int bottomy;
	int arrowrx, arrowry;
	int arrowlx, arrowly;
	int font;
	int txtfont;
//	InvItem *Bottom;
//	InvItem *ZBottom;
//	int ZMaxItems;
	InvItem *SubInv[NUMOFSUB][MAXENTRIES];
	InvItem *Main[4];
	int Selected;
	int MaxItems;
	int MaxWeapons;
	int MaxMain;
	int page;
	geSound_Def *keyclick;
// changed RF064
	bool isactive;
	bool weaponactive;
	bool itemactive;
	bool keyreturn;
	bool keyesc;
//	bool keylb;
//	bool keyrb;
	bool keyup;
	bool keydown;
	bool keyleft;
	bool keyright;
	bool stoptime;
	bool hudactive;
// end change RF064
};

#endif

/* ----------------------------------- END OF FILE ------------------------------------ */
