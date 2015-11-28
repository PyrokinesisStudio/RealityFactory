
#include "RabidFramework.h"

extern geSound_Def *SPool_Sound(char *SName);

#include "Simkin\\skScriptedExecutable.h"
#include "Simkin\\skRValue.h"
#include "Simkin\\skRValueArray.h"


//not commented, from picklses newsource.zip, nouts soundconversation...
// #define BACKCLEAR	GE_TRUE
#define BACKCLEAR	GE_FALSE


//
// ScriptConverse class
//

ScriptedConverse::ScriptedConverse(char *fileName) : skScriptedExecutable(fileName,CCD->GetskContext()) //update simkin
{
	M_CameraRect.Left = 0;
	M_CameraRect.Right = CCD->Engine()->Width() - 1;
	M_CameraRect.Top = 0;
	M_CameraRect.Bottom = CCD->Engine()->Height() - 1;
	m_Streams = NULL;
}

ScriptedConverse::~ScriptedConverse()
{
	if(m_Streams)
	{
		m_Streams->Delete();
		delete m_Streams;
	}
	m_Streams = NULL;
}

bool ScriptedConverse::getValue(const skString& fieldName, const skString& attribute, skRValue& value)
{
/*
	if (fieldName == "m_param")
	{
		// copy the value
		value = m_param;

		// we found the variable
		return true;
	}
	else */
	{
		return skScriptedExecutable::getValue(fieldName, attribute, value);
	}

}

// sets a field's value
bool ScriptedConverse::setValue(const skString& fieldName, const skString& attribute, const skRValue& value)
{
/*
	if (fieldName == "m_param")
	{
		// assign the new value
		m_param = value.intValue();

		// we found the variable
		return true;
	}
	else */
	{
		return skScriptedExecutable::setValue(fieldName, attribute, value);
	}
}

// calls a method in this object
bool ScriptedConverse::method(const skString& methodName, skRValueArray& arguments,skRValue& returnValue,skExecutableContext &ctxt)
{
	char param0[128];
	int param1;
	bool param2;

//not commented, from picklses newsource.zip, nouts soundconversation...
	char param3[128];

	param0[0] = '\0';

//not commented, from picklses newsource.zip, nouts soundconversation...
	param3[0] = '\0';

	if (IS_METHOD(methodName, "random"))
	{
		if(arguments[0].floatValue()<=arguments[1].floatValue())
			returnValue = (int)EffectC_Frand(arguments[0].floatValue(), arguments[1].floatValue());
		else
			returnValue = (int)EffectC_Frand(arguments[1].floatValue(), arguments[0].floatValue());
		return true;
	}
	else if (IS_METHOD(methodName, "Speak"))
	{
		strcpy(param0, arguments[0].str());
		Text = CCD->Pawns()->GetText(param0);
		strcpy(param0, arguments[1].str());
		if(!EffectC_IsStringNull(param0))
		{
			char music[256];
			strcpy(music, CCD->GetDirectory(kAudioStreamFile));
			strcat(music, "\\");
			strcat(music, param0);
			m_dsPtr = (LPDIRECTSOUND)geSound_GetDSound();
			m_Streams = new StreamingAudio(m_dsPtr);
			if(m_Streams)
			{
				if(!m_Streams->Create(music))
				{
					delete m_Streams;
					m_Streams = NULL;
				}
			}
		}
		return true;
	}
	else if (IS_METHOD(methodName, "Reply"))
	{
		strcpy(param0, arguments[1].str());
		param1 = arguments[0].intValue();
		if(param1>9 || param1<1)
			return true;
		Reply[param1-1].Format("%d : ", param1);
		Reply[param1-1] += CCD->Pawns()->GetText(param0);
		Reply[param1-1].TrimRight();
		Reply[param1-1].TrimLeft();
		replyflg[param1-1] = true;
		return true;
	}
// Nout begin 20-8-2003
	//Sound Conversation 
	//Begin add Nout 23092003
	else if (IS_METHOD(methodName, "SpeakShowTime"))
	{
		SpeakShowTime = arguments[0].intValue();
		return true;
	}
	else if (IS_METHOD(methodName, "ReplyShowTime"))
	{
		ReplyShowTime = arguments[0].intValue();
		return true;
	}
	//End add Nout 23092003

	else if (IS_METHOD(methodName, "SoundReply"))
	{
		strcpy(param0, arguments[1].str());
		param1 = arguments[0].intValue();
		strcpy(param3, arguments[2].str());
		if(param1>9 || param1<1)
			return true;
		Reply[param1-1].Format("%d : ", param1);
		Reply[param1-1] += CCD->Pawns()->GetText(param0);
		Reply[param1-1].TrimRight();
		Reply[param1-1].TrimLeft();
		ReplySoundFileName[param1-1].Format("%s", param3);
		ReplySoundFileName[param1-1].TrimRight();
		ReplySoundFileName[param1-1].TrimLeft();
        replyflg[param1-1] = true;
		return true;
	}
	else if (IS_METHOD(methodName, "SoundConversation"))
	{
		returnValue = DoSoundConversation(arguments[0].intValue());
		return true;
	}

// Nout begin 16-9-2003
    else if (IS_METHOD(methodName, "SoundCustomSpeakBackground"))
	{
		strcpy(param0, arguments[0].str());
		if(strlen(param0)>0) //if no .BMP name,then use no background
		{	
			geBitmap *TIcon = CCD->Pawns()->GetCache(param0);
			if(TIcon)
			{
				Background = TIcon;
				if(Background)
					geBitmap_SetColorKey(Background, GE_TRUE, 255, GE_FALSE);
			}
			else
			{
				sprintf(param3, "%s: The bitmap name may not include capitals!", param0);
				CCD->ReportError(param3, false);
			}
		}
		else	
		DrawBackground=false;
		param1 = arguments[1].intValue();
		BackgroundX = param1;
		param1 = arguments[2].intValue();
		BackgroundY = param1;
		return true;
	}
	else if (IS_METHOD(methodName, "SoundCustomReplyBackground"))
	{
		strcpy(param0, arguments[0].str());
		if(strlen(param0)>0) //if no .BMP name,then use no background
		{	
			geBitmap *TIcon = CCD->Pawns()->GetCache(param0);
			if(TIcon)
			{
				rBackground = TIcon;
				if(rBackground)
					geBitmap_SetColorKey(rBackground, GE_TRUE, 255, GE_FALSE);
			}
			else
			{
				sprintf(param3, "%s: The bitmap name may not include capitals!", param0);
				CCD->ReportError(param3, false);
			}
		}
		else
		DrawrBackground=false;
		param1 = arguments[1].intValue();
		rBackgroundX = param1;
		param1 = arguments[2].intValue();
		rBackgroundY = param1;
		return true;
	}
	else if (IS_METHOD(methodName, "ReplyInSpeakWindow"))
	{
		ReplyInSpeakWindow = arguments[0].boolValue();
		return true;
	}
// Nout end 16-9-2003

	else if (IS_METHOD(methodName, "TransparentBackground"))
	{
		BlackClear = !arguments[0].boolValue();
		return true;
	}
	else if (IS_METHOD(methodName, "SetAttribute"))
	{
		strcpy(param0, arguments[0].str());
		param1 = arguments[1].intValue();
		CPersistentAttributes *theInv = CCD->ActorManager()->Inventory(CCD->Player()->GetActor());
		returnValue = (int)theInv->Set(param0, param1);
		return true;
	}
	else if (IS_METHOD(methodName, "GetEventState"))
	{
		strcpy(param0, arguments[0].str());
		returnValue = (bool)GetTriggerState(param0);
		return true;
	}
	else if (IS_METHOD(methodName, "SetEventState"))
	{
		strcpy(param0, arguments[0].str());
		param2 = arguments[1].boolValue();
		CCD->Pawns()->AddEvent(param0, param2);
		return true;
	}
// Nout end 20-8-2003
	else if (IS_METHOD(methodName, "Conversation"))
	{
		returnValue = DoConversation(arguments[0].intValue());
		return true;
	}
	else if (IS_METHOD(methodName, "NewConversation"))
	{
		strcpy(param0, arguments[0].str());
		param2 = arguments[1].boolValue();
		strcpy(Order, param0);
		ConvFlag = !param2;
		return true;
	}
	else if (IS_METHOD(methodName, "CustomIcon"))
	{
		strcpy(param0, arguments[0].str());
		param1 = arguments[1].intValue();
		if(param1>=0)
			IconX = param1;
		param1 = arguments[2].intValue();
		if(param1>=0)
			IconY = param1;
		geBitmap *TIcon = CCD->Pawns()->GetCache(param0);
		if(TIcon)
		{
			Icon = TIcon;

//begin add Nout 16/09/2003
//Changed by Nout 16/09/2003
/*			if(Icon)
				geBitmap_SetColorKey(Icon, GE_TRUE, 0x00ff00fe, GE_TRUE);*/
		}
//begin add Nout 16/09/2003
		else
		{
			sprintf(param3, "%s: The bitmap name may not include capitals!", param0);
			CCD->ReportError(param3, false);

		}
//end add Nout 16/09/2003		

		return true;
	}
	else if (IS_METHOD(methodName, "CustomBackground"))
	{
		strcpy(param0, arguments[0].str());
		geBitmap *TIcon = CCD->Pawns()->GetCache(param0);

		if(TIcon)
			Background = TIcon;
//begin add Nout 16/09/2003
		else
		{
			sprintf(param3, "%s: The bitmap name may not include capitals!", param0);
			CCD->ReportError(param3, false);
		}
//end add Nout 16/09/2003		

		return true;
	}
	else if (IS_METHOD(methodName, "CustomSpeak"))
	{
		param1 = arguments[0].intValue();
		if(param1>=0)
			SpeachX = param1;
		param1 = arguments[1].intValue();
		if(param1>=0)
			SpeachY = param1;
		param1 = arguments[2].intValue();
		if(param1>=0)
			SpeachWidth = param1;
		param1 = arguments[3].intValue();
		if(param1>=0)
			SpeachHeight = param1;
		param1 = arguments[4].intValue();
		if(param1>=0)
			SpeachFont = param1;
		return true;
	}
	else if (IS_METHOD(methodName, "CustomReply"))
	{
		param1 = arguments[0].intValue();
		if(param1>=0)
			ReplyX = param1;
		param1 = arguments[1].intValue();
		if(param1>=0)
			ReplyY = param1;
		param1 = arguments[2].intValue();
		if(param1>=0)
			ReplyWidth = param1;
		param1 = arguments[3].intValue();
		if(param1>=0)
			ReplyHeight = param1;
		param1 = arguments[4].intValue();
		if(param1>=0)
			ReplyFont = param1;
		return true;
	}
	else if (IS_METHOD(methodName, "SetFlag"))
	{
		param1 = arguments[0].intValue();
		if(param1>=MAXFLAGS)
			return true;
		param2 = arguments[1].boolValue();
		CCD->Pawns()->SetPawnFlag(param1, param2);
		return true;
	}
	else if (IS_METHOD(methodName, "GetFlag"))
	{
		param1 = arguments[0].intValue();
		if(param1>=MAXFLAGS)
			return true;
		returnValue = CCD->Pawns()->GetPawnFlag(param1);
		return true;
	}
	else if (IS_METHOD(methodName, "GetAttribute"))
	{
		strcpy(param0, arguments[0].str());
		CPersistentAttributes *theInv = CCD->ActorManager()->Inventory(CCD->Player()->GetActor());
		returnValue = theInv->Value(param0);
		return true;
	}
	else if (IS_METHOD(methodName, "ModifyAttribute"))
	{
		strcpy(param0, arguments[0].str());
		param1 = arguments[1].intValue();
		CPersistentAttributes *theInv = CCD->ActorManager()->Inventory(CCD->Player()->GetActor());
		theInv->Modify(param0, param1);
		return true;
	}
	else
	{
		return skScriptedExecutable::method(methodName, arguments, returnValue,ctxt);//change simkin
	}
}

int ScriptedConverse::DoConversation(int charpersec)
{
	int choice;
	char *temp = new char[Text.GetLength()+1];
	char *temp1 = NULL;
	DWORD OldTime = CCD->FreeRunningCounter();
	int counter = 1;
	if(charpersec==0)
		counter = Text.GetLength();
	DWORD ElapsedTime;
	int startline = 1;
	int replyline = 1;
	strcpy(temp, Text.Left(counter));

	CString Reply1 = "";
	for(int j=0;j<9;j++)
	{
		if(replyflg[j])
		{
			if(Reply1!="")
			{
				Reply1+=" ";
				Reply1.SetAt(Reply1.GetLength()-1, (char)1);
			}
			Reply1 += Reply[j];
		}
	}
	if(Reply1!="")
	{
		temp1 = new char[Reply1.GetLength()+1];
		strcpy(temp1, Reply1);
	}
//
// display scrolling text
//
	if(m_Streams)
	{
		m_Streams->Play(false);
	}

	if(CCD->GetHasFocus())
	{
//not commented, from picklses newsource.zip, nouts soundconversation...
		geEngine_BeginFrame(CCD->Engine()->Engine(), M_Camera, BlackClear);

		geEngine_EndFrame(CCD->Engine()->Engine());
	}

	if(charpersec!=0)
	{
		while(counter<Text.GetLength())
		{
			MSG msg;
			while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
			{
				GetMessage(&msg, NULL, 0, 0 );
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if(CCD->GetHasFocus())
			{

//not commented, from picklses newsource.zip, nouts soundconversation...
				geEngine_BeginFrame(CCD->Engine()->Engine(), M_Camera, BlackClear);

				geEngine_DrawBitmap(CCD->Engine()->Engine(), Background, NULL, BackgroundX, BackgroundY );
				if(Icon)
					geEngine_DrawBitmap(CCD->Engine()->Engine(), Icon, NULL, BackgroundX+IconX, BackgroundY+IconY );
				ElapsedTime = (DWORD)(CCD->FreeRunningCounter() - OldTime);
				if(ElapsedTime > (DWORD)(1000/charpersec))
				{
					OldTime = CCD->FreeRunningCounter();
					counter +=1;
					strcpy(temp, Text.Left(counter));
				}
				TextDisplay(temp, SpeachWidth, SpeachFont);
				startline = TextOut(0, SpeachHeight, SpeachFont, SpeachX, SpeachY);
				
				geEngine_EndFrame(CCD->Engine()->Engine());
			}
		}
	}
//
// display text and allow scrolling
//
	int key = -1;
	bool keyup = false;
	bool keydwn = false;
	bool ckeyup = false;
	bool ckeydwn = false;
	for(;;)
	{
		MSG msg;
		while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
		{
			GetMessage(&msg, NULL, 0, 0 );
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if(!CCD->GetHasFocus())
			continue;

		bool kup = false;
		bool kdwn = false;
		bool ckup = false;
		bool ckdwn = false;

//not commented, from picklses newsource.zip, nouts soundconversation...
		geEngine_BeginFrame(CCD->Engine()->Engine(), M_Camera, BlackClear);

		geEngine_DrawBitmap(CCD->Engine()->Engine(), Background, NULL, BackgroundX, BackgroundY );
		if(Icon)
			geEngine_DrawBitmap(CCD->Engine()->Engine(), Icon, NULL, BackgroundX+IconX, BackgroundY+IconY );
		if(key==KEY_PAGEUP && startline!=0)
		{
			kup = true;
			if(!keyup)
			{
				keyup = true;
				if(startline>1)
					startline -=1;
			}
		}
		if(key==KEY_PAGEDOWN && startline!=0)
		{
			kdwn = true;
			if(!keydwn)
			{
				keydwn = true;
				startline +=1;
			}
		}
		TextDisplay(temp, SpeachWidth, SpeachFont);
		startline = TextOut(startline, SpeachHeight, SpeachFont, SpeachX, SpeachY);

		if(Reply1!="")
		{
			if(key==KEY_UP && replyline!=0)
			{
				ckup = true;
				if(!ckeyup)
				{
					ckeyup = true;
					if(replyline>1)
						replyline -=1;
				}
			}
			if(key==KEY_DOWN && replyline!=0)
			{
				ckdwn = true;
				if(!ckeydwn)
				{
					ckeydwn = true;
					replyline +=1;
				}
			}
			TextDisplay(temp1, ReplyWidth, ReplyFont);
			replyline = TextOut(replyline, ReplyHeight, ReplyFont, ReplyX, ReplyY);
		}

		geEngine_EndFrame(CCD->Engine()->Engine());

		if(Reply1!="")
		{
			if(key==KEY_1 && replyflg[0])
			{
				choice = 1;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_2 && replyflg[1])
			{
				choice = 2;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_3 && replyflg[2])
			{
				choice = 3;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_4 && replyflg[3])
			{
				choice = 4;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_5 && replyflg[4])
			{
				choice = 5;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_6 && replyflg[5])
			{
				choice = 6;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_7 && replyflg[6])
			{
				choice = 7;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_8 && replyflg[7])
			{
				choice = 8;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_9 && replyflg[8])
			{
				choice = 9;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
		}
		else
		{
			if(key==KEY_ESC)
			{
				choice = -1;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
		}
		if(!kup)
			keyup = false;
		if(!kdwn)
			keydwn = false;
		if(!ckup)
			ckeyup = false;
		if(!ckdwn)
			ckeydwn = false;
		
		
		key = CCD->Input()->GetKeyboardInputNoWait();
	}
	if(m_Streams)
	{
		m_Streams->Delete();
		delete m_Streams;
		m_Streams = NULL;
	}
	delete temp;
	if(temp1)
		delete temp1;

	return choice;
}

//begin add Nout 16/09/2003
int ScriptedConverse::DoSoundConversation(int charpersec)
{
	int key = -1;
	int choice;
	char *temp = new char[1000];
	char *temp1 = NULL;
	DWORD OldTime = CCD->FreeRunningCounter();
	DWORD ElapsedTime;
	int startline = 1;
	int replyline = 1;
	int counter = 1;
	if(charpersec==0)
		counter = Text.GetLength();
	strcpy(temp,Text.Left(counter));
	CString Reply1 = "";
	for(int j=0;j<9;j++)
	{
		if(replyflg[j])
		{
			if(Reply1!="")
			{
				Reply1+=" ";
				Reply1.SetAt(Reply1.GetLength()-1, (char)1);
			}
			Reply1 += Reply[j];
		}
	}
	if(Reply1!="")
	{
		temp1 = new char[Reply1.GetLength()+1];
		strcpy(temp1, Reply1);
	}
// display scrolling text
	if(m_Streams)
		m_Streams->Play(false);

	if(CCD->GetHasFocus())
	{
		geEngine_BeginFrame(CCD->Engine()->Engine(), M_Camera, BlackClear);
		geEngine_EndFrame(CCD->Engine()->Engine());
	}

	if(charpersec!=0)
	{
		while(counter<Text.GetLength())
		{
			MSG msg;
			while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
			{
				GetMessage(&msg, NULL, 0, 0 );
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if(CCD->GetHasFocus())
			{
				geEngine_BeginFrame(CCD->Engine()->Engine(), M_Camera, BlackClear);
				if(DrawBackground)
				{
					geEngine_DrawBitmap(CCD->Engine()->Engine(), Background, NULL, BackgroundX, BackgroundY );
					if(Icon)
						geEngine_DrawBitmap(CCD->Engine()->Engine(), Icon, NULL, BackgroundX+IconX, BackgroundY+IconY );
				}
				ElapsedTime = (DWORD)(CCD->FreeRunningCounter() - OldTime);
				if(ElapsedTime > (DWORD)(1000/charpersec))
				{
					OldTime = CCD->FreeRunningCounter();
					counter +=1+int(charpersec/20); //print more characters to make a faster speed
					strcpy(temp,Text.Left(counter));
				}
				TextDisplay(temp, SpeachWidth, SpeachFont);
				startline = TextOut(0, SpeachHeight, SpeachFont, SpeachX, SpeachY);
				
				geEngine_EndFrame(CCD->Engine()->Engine());
                // force fast displaying
				key = CCD->Input()->GetKeyboardInputNoWait();
    			if(key==KEY_SPACE && counter<Text.GetLength()-1)
				{
    				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
					counter = Text.GetLength() - 1;
				}
			}
		}
	}
//
// display text and allow scrolling
//
	key = -1;
	bool keyup = false;
	bool keydwn = false;
	bool ckeyup = false;
	bool ckeydwn = false;
	for(;;)
	{
		MSG msg;
		while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
		{
			GetMessage(&msg, NULL, 0, 0 );
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if(!CCD->GetHasFocus())
			continue;

		bool kup = false;
		bool kdwn = false;
		bool ckup = false;
		bool ckdwn = false;
		geEngine_BeginFrame(CCD->Engine()->Engine(), M_Camera, BlackClear);
		if(DrawBackground)
		{
			geEngine_DrawBitmap(CCD->Engine()->Engine(), Background, NULL, BackgroundX, BackgroundY );
			if(Icon)
				geEngine_DrawBitmap(CCD->Engine()->Engine(), Icon, NULL, BackgroundX+IconX, BackgroundY+IconY );
		}
		if(key==KEY_UP && startline!=0)
		{
			kup = true;
			if(!keyup)
			{
				keyup = true;
				if(startline>1)
					startline -=1;
			}
		}
		if(key==KEY_DOWN && startline!=0)
		{
			kdwn = true;
			if(!keydwn)
			{
				keydwn = true;
				startline +=1;
			}
		}
		TextDisplay(temp, SpeachWidth, SpeachFont);
		startline = TextOut(startline, SpeachHeight, SpeachFont, SpeachX, SpeachY);

		if(Reply1!="" && replyflg[1])
		{
			if(DrawrBackground)
				geEngine_DrawBitmap(CCD->Engine()->Engine(), rBackground, NULL, rBackgroundX, rBackgroundY);
			if(key==KEY_PAGEUP && replyline!=0)
			{
				ckup = true;
				if(!ckeyup)
				{
					ckeyup = true;
					if(replyline>1)
						replyline -=1;
				}
			}
			if(key==KEY_PAGEDOWN && replyline!=0)
			{
				ckdwn = true;
				if(!ckeydwn)
				{
					ckeydwn = true;
					replyline +=1;
				}
			}
			TextDisplay(temp1, ReplyWidth, ReplyFont);
			replyline = TextOut(replyline, ReplyHeight, ReplyFont, rBackgroundX+ReplyX, rBackgroundY+ReplyY);
		}

		geEngine_EndFrame(CCD->Engine()->Engine());
		//Sound Conversation 
		//Begin add Nout 23092003
		ElapsedTime = (DWORD)(CCD->FreeRunningCounter() - OldTime);
		if((ElapsedTime > (DWORD)(SpeakShowTime)) && SpeakShowTime>0)
		{
			OldTime = CCD->FreeRunningCounter();
			choice = 1;
			key = KEY_1;
			break;
		}
		//End add Nout 23092003
		if(Reply1!="" && replyflg[0])
		{
			if(key==KEY_SPACE && replyflg[0] && !replyflg[1] && !replyflg[2] && !replyflg[3] && !replyflg[4] && !replyflg[5] && !replyflg[6] && !replyflg[7] && !replyflg[8])
			{
				choice = 1;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				key = KEY_1;
				break;
			}
			if(key==KEY_1 && replyflg[0])
			{
				choice = 1;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_2 && replyflg[1])
			{
				choice = 2;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_3 && replyflg[2])
			{
				choice = 3;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_4 && replyflg[3])
			{
				choice = 4;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_5 && replyflg[4])
			{
				choice = 5;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_6 && replyflg[5])
			{
				choice = 6;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_7 && replyflg[6])
			{
				choice = 7;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_8 && replyflg[7])
			{
				choice = 8;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
			if(key==KEY_9 && replyflg[8])
			{
				choice = 9;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}
		}
		else
		{
			if(key==KEY_SPACE)
			{
				choice = -1;
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				key=-1;
				break;
			}
		}
		if(!kup)
			keyup = false;
		if(!kdwn)
			keydwn = false;
		if(!ckup)
			ckeyup = false;
		if(!ckdwn)
			ckeydwn = false;
				
		key = CCD->Input()->GetKeyboardInputNoWait();
	}
	if(m_Streams)
	{
		m_Streams->Delete();
		delete m_Streams;
		m_Streams = NULL;
	}

	// play the reply sound file
    if(Reply1!="")
	{
		char music[256];
        strcpy(music, CCD->GetDirectory(kAudioStreamFile));
		strcat(music, "\\");
		strcat(music, ReplySoundFileName[choice-1]);
		m_dsPtr = (LPDIRECTSOUND)geSound_GetDSound();
		m_Streams_reply = new StreamingAudio(m_dsPtr);
		if(m_Streams_reply)
		{
			if(!m_Streams_reply->Create(music))
			{
				delete m_Streams_reply;
				m_Streams_reply = NULL;
			}
		}
		if(m_Streams_reply)
		{
			m_Streams_reply->Play(false);
		}

//		if(ReplyInSpeakWindow)
//			geEngine_RemoveBitmap(CCD->Engine()->Engine(), rBackground); 

// show selected reply text
		key = -1;
		keyup = false;
		keydwn = false; 
		ckeyup = false;
		ckeydwn = false;
		if(choice>0)
		{
			Text = Reply[choice-1];
    		Text = Text.Right(Text.GetLength()-4);
		}
		else
			Text=" ";
		counter = 1;
		if(charpersec==0)
			counter = Text.GetLength();
		strcpy(temp,Text.Left(counter));
		startline = 1;
		if(charpersec!=0)
		{
			while(counter<Text.GetLength())
			{
				MSG msg;
 				while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
				{
					GetMessage(&msg, NULL, 0, 0 );
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				if(CCD->GetHasFocus())
				{
					geEngine_BeginFrame(CCD->Engine()->Engine(), M_Camera, BlackClear);
                    if(ReplyInSpeakWindow)
					{
						if(DrawBackground)
						{
							geEngine_DrawBitmap(CCD->Engine()->Engine(), Background, NULL, BackgroundX, BackgroundY );
							if(Icon)
								geEngine_DrawBitmap(CCD->Engine()->Engine(), Icon, NULL, BackgroundX+IconX, BackgroundY+IconY );
						}
						ElapsedTime = (DWORD)(CCD->FreeRunningCounter() - OldTime);
						if(ElapsedTime > (DWORD)(1000/charpersec))
						{
							OldTime = CCD->FreeRunningCounter();
							counter +=1+int(charpersec/20); //print more characters to make a faster speed
							strcpy(temp, Text.Left(counter));
						}
						TextDisplay(temp, SpeachWidth, SpeachFont);
						startline = TextOut(0, SpeachHeight, SpeachFont, SpeachX, SpeachY);
					}
					else
					{
						if(DrawrBackground)
						{
							geEngine_DrawBitmap(CCD->Engine()->Engine(), rBackground, NULL, rBackgroundX, rBackgroundY );
							if(Icon)
								geEngine_DrawBitmap(CCD->Engine()->Engine(), rIcon, NULL, rBackgroundX+IconX, rBackgroundY+IconY );
						}
						ElapsedTime = (DWORD)(CCD->FreeRunningCounter() - OldTime);
						if(ElapsedTime > (DWORD)(1000/charpersec))
						{
							OldTime = CCD->FreeRunningCounter();
							counter +=1+int(charpersec/20); //print more characters to make a faster speed
							strcpy(temp, Text.Left(counter));
						}
						TextDisplay(temp, ReplyWidth, ReplyFont);
						startline = TextOut(0, ReplyHeight, ReplyFont, ReplyX, ReplyY);
					}
					geEngine_EndFrame(CCD->Engine()->Engine());
					// force fast displaying when SPACE pressed
					key = CCD->Input()->GetKeyboardInputNoWait();
					if(key==KEY_SPACE && counter<Text.GetLength()-1)
					{
    					while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
						counter = Text.GetLength() - 1;
					}	
				}
			}
		}
		key = -1;
		for(;;)
		{
			MSG msg;
			while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
			{
				GetMessage(&msg, NULL, 0, 0 );
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
	
			if(!CCD->GetHasFocus())
				continue;

			bool kup = false;
			bool kdwn = false;
			bool ckup = false;
			bool ckdwn = false;
			if(ReplyInSpeakWindow)
			{
				geEngine_BeginFrame(CCD->Engine()->Engine(), M_Camera, BlackClear);
				if(DrawBackground)
				{
					geEngine_DrawBitmap(CCD->Engine()->Engine(), Background, NULL, BackgroundX, BackgroundY );
					if(Icon)
						geEngine_DrawBitmap(CCD->Engine()->Engine(), Icon, NULL, BackgroundX+IconX, BackgroundY+IconY );
				}
			}
			else
			{
				geEngine_BeginFrame(CCD->Engine()->Engine(), M_Camera, BlackClear);
				if(DrawrBackground)
				{
					geEngine_DrawBitmap(CCD->Engine()->Engine(), rBackground, NULL, rBackgroundX, rBackgroundY );
					if(Icon)
						geEngine_DrawBitmap(CCD->Engine()->Engine(), rIcon, NULL, rBackgroundX+IconX, rBackgroundY+IconY );
				}
			}
			if(key==KEY_UP && startline!=0)
			{
				kup = true;
				if(!keyup)
				{
					keyup = true;
					if(startline>1)
						startline -=1;
				}
			}
			if(key==KEY_DOWN && startline!=0)
			{
				kdwn = true;
				if(!keydwn)
				{
					keydwn = true;
					startline +=1;
				}
			}
			if(ReplyInSpeakWindow)
			{
				TextDisplay(temp, SpeachWidth, SpeachFont);
				startline = TextOut(startline, SpeachHeight, SpeachFont, SpeachX, SpeachY);
			}
			else
			{
				TextDisplay(temp, ReplyWidth, ReplyFont);
				startline = TextOut(startline, ReplyHeight, ReplyFont, ReplyX, ReplyY);
			}
			geEngine_EndFrame(CCD->Engine()->Engine());

            //Sound Conversation 
			//Begin add Nout 23092003
			ElapsedTime = (DWORD)(CCD->FreeRunningCounter() - OldTime);
			if((ElapsedTime > (DWORD)(ReplyShowTime)) && ReplyShowTime>0)
			{
				OldTime = CCD->FreeRunningCounter();
				choice = 1;
				key = KEY_1;
				break;
			}
			//End add Nout 23092003

			if(key==KEY_SPACE)
			{
				while(CCD->Input()->GetKeyboardInputNoWait()!=-1){};
				break;
			}

			if(!kup)
				keyup = false;
			if(!kdwn)
				keydwn = false;
			if(!ckup)
				ckeyup = false;
			if(!ckdwn)
				ckeydwn = false;

			key = CCD->Input()->GetKeyboardInputNoWait();
		}
    
		if(m_Streams_reply)
		{
			m_Streams_reply->Delete();
			delete m_Streams_reply;
			m_Streams_reply = NULL;
		}
//		if(ReplyInSpeakWindow)
//			geEngine_AddBitmap(CCD->Engine()->Engine(), rBackground); 
	}
	if(temp)
    	delete temp;
	if(temp1)
		delete temp1;

	return choice;
}
//end add Nout 16-9-2003

void ScriptedConverse::TextDisplay(char *Text, int Width, int Font)
{
	CString WText = Text;
	CString Line, Line1;
	int i, width, width1;
	char *temp = new char[strlen(Text)+1];
	int Index = 0;
	TextLines.RemoveAll();
	while(1)
	{
		Line = WText.Mid(Index);
		strcpy(temp, Line);
		width = CCD->MenuManager()->FontWidth(Font, temp);
		if(width<=Width)
		{
			if(Line.Find((char)1)==-1)
			{
				TextLines.Add(Line);
				break;
			}
		}
		i = 1;
		while(1)
		{
			Line1 = Line.Left(i);
			strcpy(temp, Line1);
			if(Line1[Line1.GetLength()-1]==1)
			{
				Line1 = Line.Left(i-1);
				TextLines.Add(Line1);
				Index += i;
				break;
			}
			width1 = CCD->MenuManager()->FontWidth(Font, temp);
			if(width1>Width)
			{
				if(Line1.Right(1)==" ")
				{
					Line1 = Line.Left(i-1);
					TextLines.Add(Line1);
					Index += i;
					break;
				}
				Line1 = Line.Left(i-1);
				int space = Line1.ReverseFind(' ');
				if(space==-1)
				{
					Line1 = Line.Left(i-1);
					TextLines.Add(Line1);
					Index += i;
					break;
				}
				Line1 = Line.Left(space);
				TextLines.Add(Line1);
				Index += (space+1);
				break;
			}
			i+=1;
		}
	}
	delete temp;
}

int ScriptedConverse::TextOut(int startline, int Height, int Font, int X, int Y)
{
	int sline = -1, eline;
	int THeight;
	int size = TextLines.GetSize();
	if(size>0)
	{
		if(startline==0)
		{
			sline = 0;
			eline = size;
			THeight = (size-sline) * (CCD->MenuManager()->FontHeight(Font)+2);
			while(THeight>Height)
			{
				sline += 1;
				THeight = (size-sline) * (CCD->MenuManager()->FontHeight(Font)+2);
			}
		}
		else
		{
			sline = startline-1;
			eline = size;
			THeight = (eline-sline) * (CCD->MenuManager()->FontHeight(Font)+2);
			if(sline>0 && THeight<Height)
			{
				while(THeight<Height)
				{
					sline -= 1;
					if(sline<0)
						break;
					THeight = (eline-sline) * (CCD->MenuManager()->FontHeight(Font)+2);
				}
				sline += 1;
			}
			else
			{
				while(THeight>Height)
				{
					eline -= 1;
					THeight = (eline-sline) * (CCD->MenuManager()->FontHeight(Font)+2);
				}
			}
		}
		int YOffset = Y;
		for(int i=sline; i<eline;i++)
		{
			CString Text = TextLines.GetAt(i);
			char *temp = new char[Text.GetLength()+1];
			strcpy(temp, Text);
			CCD->MenuManager()->FontRect(temp, Font, BackgroundX+X, BackgroundY+YOffset);
			YOffset += (CCD->MenuManager()->FontHeight(Font)+2);
			delete temp;
		}
	}
	return sline+1;
}


void CPawn::PreLoadC(char *filename)
{
	FILE *fdInput = NULL;
	char szInputString[1024] = {""};
	CString str;
	int i, j;
	char file[256];

	fdInput = fopen(filename, "rt");
	if(!fdInput)
		return;
	while(fgets(szInputString, 1024, fdInput) != NULL)
	{
		if(szInputString[0] == ';' || strlen(szInputString) <= 1)
			continue;
		str = szInputString;
		str.TrimRight();
		str.MakeLower();
		i = str.Find(".bmp");
		if(i>=0 && i<str.GetLength())
		{
			j=i-1;
			while(!(str.GetAt(j)=='"' || str.GetAt(j)=='[') && j>=0)
			{
				j-=1;
			}
			if(j>=0)
			{
				strcpy(file, "conversation\\");
				strcat(file, str.Mid(j+1,i-j+3));
				Cache.SetSize(Cache.GetSize()+1);
				int keynum = Cache.GetSize()-1;
				Cache[keynum].Name = str.Mid(j+1,i-j+3);
				Cache[keynum].Bitmap = CreateFromFileName(file);
				geEngine_AddBitmap(CCD->Engine()->Engine(), Cache[keynum].Bitmap);
			}
		} 
		else
		{
			i = str.Find(".tga");
			if(i>=0 && i<str.GetLength())
			{
				j=i-1;
				while(!(str.GetAt(j)=='"' || str.GetAt(j)=='[') && j>=0)
				{
					j-=1;
				}
				if(j>=0)
				{
					strcpy(file, "conversation\\");
					strcat(file, str.Mid(j+1,i-j+3));
					Cache.SetSize(Cache.GetSize()+1);
					int keynum = Cache.GetSize()-1;
					Cache[keynum].Name = str.Mid(j+1,i-j+3);
					Cache[keynum].Bitmap = CreateFromFileName(file);
					geEngine_AddBitmap(CCD->Engine()->Engine(), Cache[keynum].Bitmap);
				}
			} 
		}
	}
	fclose(fdInput);
}

void CPawn::LoadConv(char *convtxt)
{
	geVFile *MainFS;
	char szInputLine[256];
	CString readinfo, keyname, text;

	Text.SetSize(0);

// changed QuestOfDreams Language Menu
	if(!CCD->OpenRFFile(&MainFS, kInstallFile, convtxt, GE_VFILE_OPEN_READONLY))
		return;

	keyname = "";
	text = "";

	while(geVFile_GetS(MainFS, szInputLine, 256)==GE_TRUE)
	{
		if(strlen(szInputLine) <= 1)
			readinfo = "";
		else
			readinfo = szInputLine;

		if (readinfo != "" && readinfo[0] != ';')
		{
			readinfo.TrimRight();
			if (readinfo[0] == '[' && readinfo[readinfo.GetLength()-1] == ']')
			{
				if(keyname!="" && text!="")
				{
					Text.SetSize(Text.GetSize()+1);
					int keynum = Text.GetSize()-1;
					Text[keynum].Name = keyname;
					Text[keynum].Text = text;
				}
				keyname = readinfo;
				keyname.TrimLeft('[');
				keyname.TrimRight(']');
				text = "";
			}
			else
			{
				if(readinfo=="<CR>")
				{
					text+=" ";
					text.SetAt(text.GetLength()-1, (char)1);
				}
				else
				{
					if(text!="" && text[text.GetLength()-1]!=1)
						text+=" ";
					text+=readinfo;
				}
			}
		}
	}
	if(keyname!="" && text!="")
	{
		Text.SetSize(Text.GetSize()+1);
		int keynum = Text.GetSize()-1;
		Text[keynum].Name = keyname;
		Text[keynum].Text = text;
	}
	geVFile_Close(MainFS);
}

CString CPawn::GetText(char *Name)
{
	int size = Text.GetSize();
	if(size<1)
		return "";
	for(int i=0;i<size;i++)
	{
		if(Text[i].Name==Name)
			return Text[i].Text;
	}
	return "";
}


void CPawn::RunConverse(ScriptedConverse *Converse, char *szName, geBitmap *OIcon)
{
	skRValueArray args;// change simkin
	skRValue ret;

	if(!EffectC_IsStringNull(Converse->Order))
	{
		bool methoderror = false;
		try
		{
			Converse->CreateCamera();
			do
			{
				Converse->Background = Background;
				Converse->Icon = OIcon;
				Converse->BackgroundX = BackgroundX;
				Converse->BackgroundY = BackgroundY;
				Converse->IconX = IconX;
				Converse->IconY = IconY;
				Converse->SpeachX = SpeachX;
				Converse->SpeachY = SpeachY;
				Converse->SpeachWidth = SpeachWidth;
				Converse->SpeachHeight = SpeachHeight;
				Converse->SpeachFont = SpeachFont;
				Converse->ReplyX = ReplyX;
				Converse->ReplyY = ReplyY;
				Converse->ReplyWidth = ReplyWidth;
				Converse->ReplyHeight = ReplyHeight;
				Converse->ReplyFont = ReplyFont;
				Converse->ConvFlag = true;

				//begin add Nout 16/09/2003
				Converse->rBackground = rBackground;
				Converse->SpeachWindowX = SpeachWindowX;
				Converse->SpeachWindowY = SpeachWindowY;
				Converse->ReplyWindowX = ReplyWindowX;
				Converse->ReplyWindowY = ReplyWindowY;
				Converse->rBackgroundX = rBackgroundX;
				Converse->rBackgroundY = rBackgroundY;
		                Converse->BlackClear = false;
                		Converse->DrawBackground = true;
		                Converse->DrawrBackground = true;
				Converse->ReplyInSpeakWindow = true;
				//end add Nout 16/09/2003

				//Sound Conversation 
				//Begin add Nout 23092003
				Converse->SpeakShowTime = 0;
				Converse->ReplyShowTime = 0;
				//End add Nout 23092003

				for(int i=0;i<9;i++)
					Converse->replyflg[i] = false;
				Converse->method(skString(Converse->Order), args, ret,CCD->GetskContext()); // change simkin
			} while (!Converse->ConvFlag);
		}
		catch (...)
		{
			char szBug[256];
			sprintf(szBug, "Conversation Script Error for %s in Order %s", szName, Converse->Order);
			CCD->ReportError(szBug, false);
			methoderror = true;
		}
		Converse->DestroyCamera();
		CCD->Engine()->ResetSystem();
	}
}


bool CPawn::Converse(geActor *pActor)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	skRValueArray args;// change simkin
	skRValue ret;

	//	Ok, check to see if there are Pawns in this world
	
	pSet = geWorld_GetEntitySet(CCD->World(), "Pawn");
	
	if(pSet) 
	{
		for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
		{
			Pawn *pSource = (Pawn*)geEntity_GetUserData(pEntity);
			if(pSource->Data)
			{
				ScriptedObject *Object = (ScriptedObject *)pSource->Data;
				if(Object->Actor==pActor && Object->active && Object->UseKey)
				{
					if(pSource->Converse)
					{
						ScriptedConverse *Converse = (ScriptedConverse *)pSource->Converse;
						RunConverse(Converse, Object->szName, Object->Icon);
					}
					return true;
				}
			}
		}
	}
	return false;
}