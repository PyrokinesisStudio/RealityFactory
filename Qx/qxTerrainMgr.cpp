// qxTerrainMgr.cpp: implementation of the qxTerrainMgr class.
//
// Author: Jeff Thelen
//
// Copyright 2000 Quixotic, Inc.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include "..\\RabidFramework.h"
#include "qxTerrainUtils.h"
#include "qxTerrainMap.h"
#include "qxTerrainMapBase.h"
//#include "qxPolyQuad.h"
#include "qxColor.h"
#include "qxCloudMachine.h"
#include "qxSun.h"
#include "qxMoon.h"
#include "qxStars.h"
#include "qxEffectTextureFlow.h"
#include "qxTerrainDefinitionFile.h"
#include "qxSkyDome.h"
#include "QxUser.h"

#include "..\\Simkin\\skScriptedExecutable.h"
#include "..\\Simkin\\skRValue.h"
#include "..\\Simkin\\skRValueArray.h"
#include "..\\Simkin\\skRuntimeException.h"
#include "..\\Simkin\\skParseException.h"
#include "..\\Simkin\\skBoundsException.h"
#include "..\\Simkin\\skTreeNode.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


TerrainObject::TerrainObject(char *fileName) : skScriptedExecutable(fileName)
{
	Order[0] = '\0';
	ElapseTime = 0.0f;
}

TerrainObject::~TerrainObject()
{
}

bool TerrainObject::method(const skString& methodName, skRValueArray& arguments,skRValue& returnValue)
{

	return skScriptedExecutable::method(methodName, arguments, returnValue);
}

bool TerrainObject::getValue(const skString& fieldName, const skString& attribute, skRValue& value)
{
	return skScriptedExecutable::getValue(fieldName, attribute, value);
}

bool TerrainObject::setValue(const skString& fieldName, const skString& attribute, const skRValue& value)
{
	return skScriptedExecutable::setValue(fieldName, attribute, value);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


qxTerrainMgr::qxTerrainMgr()
:m_bUpdateLandscape(true)
,m_bRenderWireframe(false)
,m_bRenderLandscape(true)
,m_nHeightFieldSourceType(1)
,m_nPolyIncrement(100)
,m_nDistanceDetail(8)
,m_fNormalDistanceToCamera(1000)
,m_pCloudMachine(0)
,m_pSun(0)
,m_pMoon(0)
,m_pStarField(0)
,m_pTerrainDefinitionFile(0)
,m_nWorldBoundsX(10)
,m_nWorldBoundsZ(10)
,m_pSkyDome(0)
,m_fTwilightPercent(0.0)
,TerrainTime(0.0)
,Initialized(false)
{
	m_fScaleY = 1.0f;
	m_fScaleXZ = 4.0f;
	m_OffsetY = 0;
	m_nDesiredTriangles = 500;
	m_nLandscapeSize = 4096;
	m_nLandscapeSize *= (int)m_fScaleXZ;
	m_nFarPlane = 9999999;
	m_nHeightFieldSourceType = 1;
	m_nShadingMethod = 0;
	m_fNormalDistanceToCamera = 500.0f;
	m_vLightSource.X = -1.0f;
	m_vLightSource.Y = 0.45f;
	m_vLightSource.Z = -1.0f;
	m_fLightAmbient = 0.3f;
	m_fLightIntensity = 1.0f;
	m_PixelFormatFinal = (gePixelFormat)5;
	m_AmbientLightColor.r = 255.0f;
	m_AmbientLightColor.g = 255.0f;
	m_AmbientLightColor.b = 255.0f;
	m_AmbientLightColor.a = 255.0f;
	m_fTwilightDistanceFromHorizon = 0.4f;
	m_TimeScale = 1;
	geVec3d_Normalize(&m_vLightSource);

	strcpy(m_strBmp, "terrain\\clouds_256.bmp");
	SkyMaxHeight = 200;
	fScaleXZ = 1.0f;
	fSkyLengthHeight = 32.0f;
	m_SunScale = 4.0f;
	rgba.r = 164.0f;
	rgba.g = 200.0f;
	rgba.b = 255.0f;
	rgba.a = 255.0f;
	m_fMinBlueSkyColor = 12.0f;
	m_fDistanceFromSunFactor = 900;
	color.r = 255.0f;
	color.g = 0.0f;
	color.b = 0.0f;
	color.a = 255.0f;
	m_fColorUpdateTime = 1.0f;
	TextureFlow = false;

	fPlayerLatitude = 49.0f;
	Hour = 12;
	Day = 24;
	Month = 10;

	Object = NULL;
}

qxTerrainMgr::~qxTerrainMgr()
{
	if(Object)
		delete Object;
	if(Initialized)
		Shutdown();
}

bool qxTerrainMgr::Init()
{
	geEntity_EntitySet* lEntitySet;
	geEntity* lEntity;
	
	lEntitySet = geWorld_GetEntitySet(CCD->World(), "SkyDome");
	
	if(lEntitySet != NULL)
	{
		lEntity = geEntity_EntitySetGetNextEntity(lEntitySet, NULL);
		if(lEntity)
		{	
			SkyDome *pSource = (SkyDome*)geEntity_GetUserData(lEntity);

			if(!pSource->Enable)
				return false;

			if(!EffectC_IsStringNull(pSource->ScriptName))
			{
				char script[128] = "scripts\\";
				strcat(script, pSource->ScriptName);
				try
				{
					Object = new TerrainObject(script);
				}
				catch(skParseException e)
				{
					char szBug[256];
					sprintf(szBug, "Parse Script Error for SkyDome script");
					CCD->ReportError(szBug, false);
					strcpy(szBug, e.toString());
					CCD->ReportError(szBug, false);
					return false;
				}
				catch(skBoundsException e)
				{
					char szBug[256];
					sprintf(szBug, "Bounds Script Error for SkyDome script");
					CCD->ReportError(szBug, false);
					strcpy(szBug, e.toString());
					CCD->ReportError(szBug, false);
					return false;
				}
				catch(skRuntimeException e)
				{
					char szBug[256];
					sprintf(szBug, "Runtime Script Error for SkyDome script");
					CCD->ReportError(szBug, false);
					strcpy(szBug, e.toString());
					CCD->ReportError(szBug, false);
					return false;
				}
				catch(skTreeNodeReaderException e)
				{
					char szBug[256];
					sprintf(szBug, "Reader Script Error for SkyDome script");
					CCD->ReportError(szBug, false);
					strcpy(szBug, e.toString());
					CCD->ReportError(szBug, false);
					return false;
				}
				catch (...)
				{
					char szBug[256];
					sprintf(szBug, "Script Error for SkyDome script");
					CCD->ReportError(szBug, false);
					return false;
				} 
				if(!EffectC_IsStringNull(pSource->InitOrder))
				{
					skRValueArray args(1);
					skRValue ret;

					strcpy(Object->Order, pSource->InitOrder);
					try
					{
						Object->method(skString(Object->Order), args, ret);
					}
					catch(skRuntimeException e)
					{
						char szBug[256];
						sprintf(szBug, "Runtime Script Error for SkyDome script");
						CCD->ReportError(szBug, false);
						strcpy(szBug, e.toString());
						CCD->ReportError(szBug, false);
						return false;
					}
					catch(skParseException e)
					{
						char szBug[256];
						sprintf(szBug, "Parse Script Error for SkyDome script");
						CCD->ReportError(szBug, false);
						strcpy(szBug, e.toString());
						CCD->ReportError(szBug, false);
						return false;
					}
					catch (...)
					{
						char szBug[256];
						sprintf(szBug, "Script Error for SkyDome script");
						CCD->ReportError(szBug, false);
						return false;
					}
				}
				if(!EffectC_IsStringNull(pSource->StartOrder))
					strcpy(Object->Order, pSource->StartOrder);
				else
					Object->Order[0] = '\0';
			}
			
			m_pqxPolyPool = new qxPolyPool;
			m_pqxVertPool = new qxVertPool;

			qxTerrainDefinition Def;
			Def.m_strBmp = m_strBmp;
			Def.m_strAlpha	= m_strBmp;
			Def.m_nFarPlane = 9999999;

			m_pSkyDome = new qxSkyDome(	Def, (int)fSkyLengthHeight, (float)SkyMaxHeight, 
				fScaleXZ);
			
			if( !m_pSkyDome->Init() )
			{
				char szError[256];
				sprintf(szError,"Can't initialize SkyDome");
				CCD->ReportError(szError, false);
				CCD->ShutdownLevel();
				delete CCD;
				MessageBox(NULL, szError,"Terrain Manager", MB_OK);
				exit(-333);
			}
			
			
			m_pSun = new qxSun;
			if( !m_pSun->Init())
			{
				char szError[256];
				sprintf(szError,"Can't initialize Sun");
				CCD->ReportError(szError, false);
				CCD->ShutdownLevel();
				delete CCD;
				MessageBox(NULL, szError,"Terrain Manager", MB_OK);
				exit(-333);
			}
			
			m_pMoon = new qxMoon;
			if( !m_pMoon->Init())
			{
				char szError[256];
				sprintf(szError,"Can't initialize Moon");
				CCD->ReportError(szError, false);
				CCD->ShutdownLevel();
				delete CCD;
				MessageBox(NULL, szError,"Terrain Manager", MB_OK);
				exit(-333);
			}
			
			m_pStarField = new qxStarField;
			if( !m_pStarField->Init())
			{
				char szError[256];
				sprintf(szError,"Can't initialize Stars");
				CCD->ReportError(szError, false);
				CCD->ShutdownLevel();
				delete CCD;
				MessageBox(NULL, szError,"Terrain Manager", MB_OK);
				exit(-333);
			}

			m_pCloudMachine = new qxCloudMachine;
			if( !m_pCloudMachine->Init())
			{
				char szError[256];
				sprintf(szError,"Can't initialize Clouds");
				CCD->ReportError(szError, false);
				CCD->ShutdownLevel();
				delete CCD;
				MessageBox(NULL, szError,"Terrain Manager", MB_OK);
				exit(-333);
			}
			m_pCloudMachine->DoWhiteFluffy();

			//if(LoadTerrainDefinitionFile(""))
				//LoadMap(0, 0);

			TerrainTime = (float)Hour * 3600.0f;
			CalculatePlayerLatitude();
			Initialized = true;
			return true;
		}
	}
	return false;
/*	
	AttrFile.SetPath("terrain.ini");
	if(!AttrFile.ReadFile())
	{
		char szError[256];
		sprintf(szError,"Can't open Terrain initialization file");
		CCD->ReportError(szError, false);
		CCD->ShutdownLevel();
		delete CCD;
		MessageBox(NULL, szError,"Terrain Manager", MB_OK);
		exit(-333);
	}
	bool flag = false;
	CString KeyName = AttrFile.FindFirstKey();
	while(KeyName != "")
	{
		if(!strcmp(KeyName,"Terrain"))
		{
			m_fScaleY = (float)AttrFile.GetValueF(KeyName, "scaley");
			m_fScaleXZ = (float)AttrFile.GetValueF(KeyName, "scalexz");
			m_OffsetY = AttrFile.GetValueI(KeyName, "offsety");
			m_nDesiredTriangles = AttrFile.GetValueI(KeyName, "desiredtriangles");
			m_nLandscapeSize = AttrFile.GetValueI(KeyName, "landscapesize");
			m_nLandscapeSize		*= (int)m_fScaleXZ;

			m_nFarPlane = 9999999;
			geEntity_EntitySet* lEntitySet;
			geEntity* lEntity;
	
			lEntitySet = geWorld_GetEntitySet(CCD->World(), "EnvironmentSetup");
	
			if(lEntitySet != NULL)
			{
				lEntity = geEntity_EntitySetGetNextEntity(lEntitySet, NULL);
				if(lEntity)
				{	
					EnvironmentSetup *theState = (EnvironmentSetup*)geEntity_GetUserData(lEntity);
					if(theState->UseFarClipPlane == GE_TRUE)
					{
						float detailevel = (float)CCD->MenuManager()->GetDetail();
						//m_nFarPlane = (int)(theState->FarClipPlaneDistLow + ((theState->FarClipPlaneDistHigh - theState->FarClipPlaneDistLow)*(detailevel/100)));
					}
				}
			}

			m_nHeightFieldSourceType = AttrFile.GetValueI(KeyName, "source");
			m_nShadingMethod = AttrFile.GetValueI(KeyName, "shadingmode");
			m_fNormalDistanceToCamera = (float)AttrFile.GetValueF(KeyName, "distancetocamera");
			m_vLightSource.X = (float)AttrFile.GetValueF(KeyName, "lightx");
			m_vLightSource.Y = (float)AttrFile.GetValueF(KeyName, "lighty");
			m_vLightSource.Z = (float)AttrFile.GetValueF(KeyName, "lightz");
			m_fLightAmbient = (float)AttrFile.GetValueF(KeyName, "shadingambient");
			m_fLightIntensity = (float)AttrFile.GetValueF(KeyName, "shadingintensity");
			m_PixelFormatFinal = (gePixelFormat)AttrFile.GetValueI(KeyName, "finalformat");
			m_AmbientLightColor.r = (float)AttrFile.GetValueF(KeyName, "ambient_r");
			m_AmbientLightColor.g = (float)AttrFile.GetValueF(KeyName, "ambient_g");
			m_AmbientLightColor.b = (float)AttrFile.GetValueF(KeyName, "ambient_b");
			m_AmbientLightColor.a = (float)AttrFile.GetValueF(KeyName, "ambient_a");
			m_fTwilightDistanceFromHorizon = (float)AttrFile.GetValueF(KeyName, "twilightdistance");
			m_TimeScale = (float)AttrFile.GetValueF(KeyName, "timescale");

			geVec3d_Normalize(&m_vLightSource);
			flag = true;
			break;
			
		}
		KeyName = AttrFile.FindNextKey();
	}
	if(!flag)
	{
		char szError[256];
		sprintf(szError,"Can't find Terrain section");
		CCD->ReportError(szError, false);
		CCD->ShutdownLevel();
		delete CCD;
		MessageBox(NULL, szError,"Terrain Manager", MB_OK);
		exit(-333);
	}


	m_pqxPolyPool = new qxPolyPool;
	m_pqxVertPool = new qxVertPool;

	flag = false;
	KeyName = AttrFile.FindFirstKey();
	while(KeyName != "")
	{
		if(!strcmp(KeyName,"SkyDome"))
		{
			qxTerrainDefinition Def;
			Def.m_strBmp = AttrFile.GetValue(KeyName, "skybmp");
			Def.m_strAlpha	= AttrFile.GetValue(KeyName, "skybmp");
			Def.m_nFarPlane = 9999999;

			int SkyMaxHeight = AttrFile.GetValueI(KeyName, "skymaxheight");
			float fScaleXZ = (float)AttrFile.GetValueF(KeyName, "skyscalexz");
			float fSkyLengthHeight = (float)AttrFile.GetValueF(KeyName, "skylengthheight");
			m_SunScale = (float)AttrFile.GetValueF(KeyName, "skysunscale");

			m_pSkyDome = new qxSkyDome(	Def, (int)fSkyLengthHeight, (float)SkyMaxHeight, 
				fScaleXZ);
			flag = true;
			break;
			
		}
		KeyName = AttrFile.FindNextKey();
	}
	if(!flag)
	{
		char szError[256];
		sprintf(szError,"Can't find SkyDome section");
		CCD->ReportError(szError, false);
		CCD->ShutdownLevel();
		delete CCD;
		MessageBox(NULL, szError,"Terrain Manager", MB_OK);
		exit(-333);
	}

	flag = false;
	KeyName = AttrFile.FindFirstKey();
	while(KeyName != "")
	{
		if(!strcmp(KeyName,"Location"))
		{
			fPlayerLatitude = (float)AttrFile.GetValueF(KeyName, "latitude");
			Hour = AttrFile.GetValueI(KeyName, "hour");
			Day = AttrFile.GetValueI(KeyName, "day");
			Month = AttrFile.GetValueI(KeyName, "month");
			TerrainTime = (float)Hour * 3600.0f;
			CalculatePlayerLatitude();
			flag = true;
		}
		KeyName = AttrFile.FindNextKey();
	}
	if(!flag)
	{
		char szError[256];
		sprintf(szError,"Can't find Location section");
		CCD->ReportError(szError, false);
		CCD->ShutdownLevel();
		delete CCD;
		MessageBox(NULL, szError,"Terrain Manager", MB_OK);
		exit(-333);
	}

	if( !m_pSkyDome->Init() )
	{
		char szError[256];
		sprintf(szError,"Can't initialize SkyDome");
		CCD->ReportError(szError, false);
		CCD->ShutdownLevel();
		delete CCD;
		MessageBox(NULL, szError,"Terrain Manager", MB_OK);
		exit(-333);
	}

	
	m_pSun = new qxSun;
	if( !m_pSun->Init())
	{
		char szError[256];
		sprintf(szError,"Can't initialize Sun");
		CCD->ReportError(szError, false);
		CCD->ShutdownLevel();
		delete CCD;
		MessageBox(NULL, szError,"Terrain Manager", MB_OK);
		exit(-333);
	}

	m_pMoon = new qxMoon;
	if( !m_pMoon->Init())
	{
		char szError[256];
		sprintf(szError,"Can't initialize Moon");
		CCD->ReportError(szError, false);
		CCD->ShutdownLevel();
		delete CCD;
		MessageBox(NULL, szError,"Terrain Manager", MB_OK);
		exit(-333);
	}

	m_pStarField = new qxStarField;
	if( !m_pStarField->Init())
	{
		char szError[256];
		sprintf(szError,"Can't initialize Stars");
		CCD->ReportError(szError, false);
		CCD->ShutdownLevel();
		delete CCD;
		MessageBox(NULL, szError,"Terrain Manager", MB_OK);
		exit(-333);
	}


	if(!LoadTerrainDefinitionFile(""))
		return false;

	
	m_pCloudMachine = new qxCloudMachine;
	if( !m_pCloudMachine->Init())
	{
		char szError[256];
		sprintf(szError,"Can't initialize Clouds");
		CCD->ReportError(szError, false);
		CCD->ShutdownLevel();
		delete CCD;
		MessageBox(NULL, szError,"Terrain Manager", MB_OK);
		exit(-333);
	}

	m_pCloudMachine->DoWhiteFluffy();
	//m_pCloudMachine->DoGreyOvercast();
	//m_pCloudMachine->DoCloudless();

	Initialized = true;
	
	LoadMap(0, 0);

	return true;
*/
}

bool qxTerrainMgr::Load()
{
/*	CString strHeightFieldFile;

	

	if( m_nHeightFieldSourceType == 1 )
		strHeightFieldFile = Config()->getString("TERRAIN_HEIGHTFIELD_BMP");
	else
		strHeightFieldFile = Config()->getString("TERRAIN_HEIGHTFIELD_TER");


	CString strTextureFile = Config()->getString("TERRAIN_BMP");

	LoadMap(0,0);
*/
	return true;

}

void qxTerrainMgr::Shutdown()
{

	delete m_pTerrainDefinitionFile;
	delete m_pSun;
	delete m_pMoon;
	delete m_pStarField;
	delete m_pSkyDome;
	delete m_pCloudMachine;


	for( int i = 0; i < m_pMaps.GetSize(); i++)
	{
		qxTerrainMapBase* p = m_pMaps.GetAt(i);
		delete p;
	}

	m_pMaps.RemoveAll();
	

	delete m_pqxPolyPool;
	delete m_pqxVertPool;

}




bool qxTerrainMgr::Frame()
{
	if(!Initialized)
		return true;

	CalculatePlayerLatitude();
	CCD->CameraManager()->TrackMotion();
	CCD->CameraManager()->RenderView();

	TerrainTime += (CCD->LastElapsedTime_F()*0.001f)*m_TimeScale;
	if(TerrainTime>86400.0f)
	{
		Day += 1;
		switch(Month)
		{
		case 1:
			if(Day>31)
			{
				Day = 1;
				Month +=1;
			}
			break;
		case 2:
			if(Day>28)
			{
				Day = 1;
				Month +=1;
			}
			break;
		case 3:
			if(Day>31)
			{
				Day = 1;
				Month +=1;
			}break;
		case 4:
			if(Day>30)
			{
				Day = 1;
				Month +=1;
			}
			break;
		case 5:
			if(Day>31)
			{
				Day = 1;
				Month +=1;
			}
			break;
		case 6:
			if(Day>30)
			{
				Day = 1;
				Month +=1;
			}
			break;
		case 7:
			if(Day>31)
			{
				Day = 1;
				Month +=1;
			}
			break;
		case 8:
			if(Day>31)
			{
				Day = 1;
				Month +=1;
			}
			break;
		case 9:
			if(Day>30)
			{
				Day = 1;
				Month +=1;
			}
			break;
		case 10:
			if(Day>31)
			{
				Day = 1;
				Month +=1;
			}
			break;
		case 11:
			if(Day>30)
			{
				Day = 1;
				Month +=1;
			}
			break;
		case 12:
			if(Day>31)
			{
				Day = 1;
				Month = 1;
			}
			break;
		}
		TerrainTime -= 86400.0f;
	}

	//
	// Rotate the planet
	//

	SetEarthAxis();

	if( m_pSun )
		m_pSun->Frame();

	UpdateTwilightPercent();

	if( m_pSkyDome )
		m_pSkyDome->Frame();

	if( m_pCloudMachine)
		m_pCloudMachine->Frame();

	if( m_pMoon )
		m_pMoon->Frame();
		
	if( m_pStarField )
		m_pStarField->Frame();

	UpdateFog();


	for( int i = 0; i < m_pMaps.GetSize(); i++)
	{
		qxTerrainMapBase* p = m_pMaps.GetAt(i);
		p->Frame();
	}



	return true;
}



bool qxTerrainMgr::Render()
{
	if(!Initialized)
		return true;

	if(m_pSkyDome)
	{
		if( m_bRenderWireframe )
			m_pSkyDome->RenderWireframe();
		else
			m_pSkyDome->Render();
	}


	if( m_bRenderLandscape)
	{
		
		if(!m_bRenderWireframe)
		{
			for( int i = 0; i < m_pMaps.GetSize(); i++)
			{
				qxTerrainMapBase* p = m_pMaps.GetAt(i);
				p->Render();
			}
		}

		else 
		{
			for( int i = 0; i < m_pMaps.GetSize(); i++)
			{
				qxTerrainMapBase* p = m_pMaps.GetAt(i);
				p->RenderWireframe();
			}
		}
		
		
	}

	return true;
}

void qxTerrainMgr::Draw()
{
	if(!Initialized)
		return;

	m_pCloudMachine->Draw();

	for( int i = 0; i < m_pMaps.GetSize(); i++)
	{
		qxTerrainMapBase* p = m_pMaps.GetAt(i);
		p->Draw();
	}

	if(m_pSun)
		m_pSun->Draw();

	if(m_pSkyDome)
		m_pSkyDome->Draw();
}


void qxTerrainMgr::SetNormalDistanceToCamera( float f )
{
	m_fNormalDistanceToCamera = f;
}

void qxTerrainMgr::PolyCountRaise()
{
	m_nDesiredTriangles += m_nPolyIncrement;
}

void qxTerrainMgr::PolyCountLower()
{
	
	m_nDesiredTriangles -= m_nPolyIncrement;
	
	if(m_nDesiredTriangles < 0)
		m_nDesiredTriangles = m_nPolyIncrement;
}
void qxTerrainMgr::DistanceDetailLower()
{
	
	if(m_nDistanceDetail > 1)
		m_nDistanceDetail--;
}

void qxTerrainMgr::DistanceDetailRaise()
{
	if( m_nDistanceDetail == MAX_DISTANCE_DETAIL)
		return;
	
	m_nDistanceDetail++;
}

void qxTerrainMgr::ToggleRenderWireframe()
{
	m_bRenderWireframe = !m_bRenderWireframe; 
}


//
// Load a new map if it's time
//


qxTerrainMapBase* qxTerrainMgr::GetEastNeighbor(qxTerrainMapBase* p)
{
	int OffsetX = p->GetMapOffsetIndexX();
	int OffsetZ = p->GetMapOffsetIndexZ();
	
	return GetMap(OffsetX, OffsetZ+1);

}

qxTerrainMapBase* qxTerrainMgr::GetWestNeighbor(qxTerrainMapBase* p)
{
	int OffsetX = p->GetMapOffsetIndexX();
	int OffsetZ = p->GetMapOffsetIndexZ();
	
	return GetMap(OffsetX, OffsetZ-1);

}
qxTerrainMapBase* qxTerrainMgr::GetNorthNeighbor(qxTerrainMapBase* p)
{
	int OffsetX = p->GetMapOffsetIndexX();
	int OffsetZ = p->GetMapOffsetIndexZ();
	
	return GetMap(OffsetX+1, OffsetZ);

}
qxTerrainMapBase* qxTerrainMgr::GetSouthNeighbor(qxTerrainMapBase* p)
{
	int OffsetX = p->GetMapOffsetIndexX();
	int OffsetZ = p->GetMapOffsetIndexZ();
	
	return GetMap(OffsetX-1, OffsetZ);

}
qxTerrainMapBase* qxTerrainMgr::GetNorthWestNeighbor(qxTerrainMapBase* p)
{
	int OffsetX = p->GetMapOffsetIndexX();
	int OffsetZ = p->GetMapOffsetIndexZ();
	
	return GetMap(OffsetX+1, OffsetZ-1);

}
qxTerrainMapBase* qxTerrainMgr::GetNorthEastNeighbor(qxTerrainMapBase* p)
{
	int OffsetX = p->GetMapOffsetIndexX();
	int OffsetZ = p->GetMapOffsetIndexZ();
	
	return GetMap(OffsetX+1, OffsetZ+1);

}
qxTerrainMapBase* qxTerrainMgr::GetSouthEastNeighbor(qxTerrainMapBase* p)
{
	int OffsetX = p->GetMapOffsetIndexX();
	int OffsetZ = p->GetMapOffsetIndexZ();
	
	return GetMap(OffsetX-1, OffsetZ+1);

}
qxTerrainMapBase* qxTerrainMgr::GetSouthWestNeighbor(qxTerrainMapBase* p)
{
	int OffsetX = p->GetMapOffsetIndexX();
	int OffsetZ = p->GetMapOffsetIndexZ();
	
	return GetMap(OffsetX-1, OffsetZ-1);

}

void qxTerrainMgr::SetEarthAxis(  )
{
	int nSeconds = (int)TerrainTime;
	float RotateY = RADIANS( .0041667f * nSeconds );
	
	geXForm3d_SetIdentity(&m_matEarthRotation);

	float RotateX  = m_fPlayerLatitudeRotation;

	geXForm3d_RotateY(&m_matEarthRotation, RotateY );
	geXForm3d_RotateX(&m_matEarthRotation, RotateX );

}

qxTerrainMapBase* qxTerrainMgr::GetMap( int OffsetX, int OffsetZ )
{
	if( OffsetX < 0 || OffsetZ < 0)
		return NULL;

	for( int i = 0; i < m_pMaps.GetSize(); i++)
	{
		qxTerrainMapBase* p = m_pMaps.GetAt(i);

		if(	p->GetMapOffsetIndexX() == OffsetX
			&& p->GetMapOffsetIndexZ() == OffsetZ)
			return p;
	}

	return NULL;
}

void qxTerrainMgr::DynamicLoadCheck()
{

	if( !CCD->CameraManager()->GetPositionMoved() )
		return;

	geVec3d vCenter;
	CCD->CameraManager()->GetPosition(&vCenter);
	geVec3d* pCam = &vCenter;

	int Map_X = (int)pCam->X / m_nLandscapeSize;
	int Map_Z = (int)pCam->Z / m_nLandscapeSize;

	LoadMap(Map_X, Map_Z);
}

inline bool qxTerrainMgr::LoadMap( int OffsetX, int OffsetZ )
{

	// See if requested map is beyond the world boundaries
	if( OffsetX > m_nWorldBoundsX || OffsetZ > m_nWorldBoundsZ )
		return true;

	// See if it's loaded already

	for( int i = 0; i < m_pMaps.GetSize(); i++)
	{
		qxTerrainMapBase* p = m_pMaps.GetAt(i);

		if(	p->GetMapOffsetIndexX() == OffsetX
			&& p->GetMapOffsetIndexZ() == OffsetZ)
			return true;
	}

	// Go to the terrain definition file and find the coordinates
	QXASSERT( m_pTerrainDefinitionFile );

	qxTerrainDefinition TerrainDef;
	m_pTerrainDefinitionFile->m_pTerrainDef = &TerrainDef;
	
	if( !m_pTerrainDefinitionFile->Load( OffsetX, OffsetZ ) )
	{
		return false;
	}


	qxTerrainMapBase* pMap = 0;
	
	switch( TerrainDef.m_eType )
	{
	case TT_LAND:
		pMap	= (qxTerrainMapBase*) new qxTerrainMap(	TerrainDef );  
		break;

	default:
		QXASSERT(0);
		return false;
	}

	QXASSERT( pMap );
	
	if( !pMap->Init())
		return false;

	pMap->SetSunLight(true);

	m_pMaps.Add(pMap);

	return true;
}


bool qxTerrainMgr::LoadTerrainDefinitionFile(char* pStr)
{

	QXASSERT(pStr);

	delete m_pTerrainDefinitionFile;
	m_pTerrainDefinitionFile = new qxTerrainDefinitionFile(pStr);
	if( !m_pTerrainDefinitionFile )
		return false;
	return true;
}


void qxTerrainMgr::UpdateFog()
{
	qxSkyDome* pSky = GetSkyDome();

	if( pSky )
	{
		float fFogEnd = CCD->Engine()->GetFogEnd();
		float fFogStart = CCD->Engine()->GetFogStart();

		qxColor	m_CurrentSkyColor = *pSky->GetCurrentSkyColor();

		float v = m_CurrentSkyColor.GetV();
		v *= 0.7f;
		m_CurrentSkyColor.SetV(v);

		float s = m_CurrentSkyColor.GetS();
		float delta = 1.0f-s;
		s += delta * (1.0f-0.5f);
		m_CurrentSkyColor.SetS(s);

		CCD->Engine()->SetFogParameters(m_CurrentSkyColor.rgba, fFogStart, fFogEnd);
		CCD->Player()->ShowFog();
	}
}

float qxTerrainMgr::GetSunPercentToZenith()
{  
	return m_pSun->GetPercentToZenith(); 
}

void qxTerrainMgr::UpdateTwilightPercent()
{
	// -1.0 midnight, 1.0 noon
	float fSunPercentToZenith = GetSunPercentToZenith();

	// distance above and below horizon 
	float fTwilightDist = GetTwilightDistanceFromHorizon();

	// Twilight distance is 0.0 to 1.0
	m_fTwilightPercent = (fSunPercentToZenith+fTwilightDist)/ (fTwilightDist*2.0f) ;
	m_fTwilightPercent = GE_CLAMP(m_fTwilightPercent, 0.0f, 1.0f);

	
}

//
// Player occupies a position on earth.
// We need to calculate a rotation factor for the
// terrain manager in radians from equator.
// This only needs to be done once a day in game time.
//
void qxTerrainMgr::CalculatePlayerLatitude()
{
	// Temp. We need to know where the player is on the planet.
	// 0.0 = equator
	// the Tropic of Cancer (23 1/2�N.)		= 23.5
	// the Tropic of Capricorn (23 1/2�S.)	= -23.5
	// the Arctic Circle (66 1/2�N.),		= 66.5
	// and the Antarctic Circle (66 1/2�S.)	= -66.5

	int nDay;
	switch(Month)
	{
	case 1:
		nDay = 0;
		break;
	case 2:
		nDay = 31;
		break;
	case 3:
		nDay = 59;
		break;
	case 4:
		nDay = 90;
		break;
	case 5:
		nDay = 120;
		break;
	case 6:
		nDay = 151;
		break;
	case 7:
		nDay = 181;
		break;
	case 8:
		nDay = 212;
		break;
	case 9:
		nDay = 243;
		break;
	case 10:
		nDay = 273;
		break;
	case 11:
		nDay = 304;
		break;
	case 12:
		nDay = 334;
		break;
	}
	nDay += Day;
	float fDayRads = RADIANS( nDay );
	float fDayFactor = (float) cos( fDayRads  );


	float f = fDayFactor*23.5f;
	float f2 = fPlayerLatitude + f + 90.0f; // Shift 90 since matrix assumes we are at north pole
	m_fPlayerLatitudeRotation = RADIANS( f2 );

}
