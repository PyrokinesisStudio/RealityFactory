#ifndef __RGF_CPAWN_H__ 
#define __RGF_CPAWN_H__

#include "Simkin\\skScriptedExecutable.h" // Update Simkin

#define DEBUGLINES	8
#define MAXFLAGS  500

#define PARMCHECK(x) CCD->Pawns()->ParmCheck(arguments.entries(), x, Order, szName, methodName)

typedef struct ActionList
{
	ActionList *next;
	ActionList *prev;
	char AnimName[64];
	int Action;
	float Value1, Value2, Value3, Value4;
	float Speed;
	bool Flag;
	char SoundName[256];
	char TriggerName[256];
} ActionList;

typedef struct ActionStack
{
	ActionStack *next;
	ActionStack *prev;
	ActionList *Top;
	ActionList *Bottom;
	ActionList *Index;
	char Order[64];
	char Point[64];
	char NextOrder[64];
	geVec3d CurrentPoint;
	bool ActionActive;
	bool ValidPoint;
	geVec3d	Vec2Point;
	bool DistActive;
	float MinDistance;
	char DistOrder[64];
} ActionStack;

typedef enum
{
	PTRIGGER = 0,
	PFLAG,
	PTIMER,
	PDIST,
	PSOUND,
	PCOLLIDE,
	PEND
};

typedef struct TriggerStack
{
	TriggerStack *next;
	TriggerStack *prev;
	char OrderName[64];
	float Delay;
	int Type;
	bool Flag;
	int PFlg;
	float Time;
	float Low;
	float High;
	char TriggerName[128];
} TriggerStack;

typedef struct EventStack
{
	EventStack *next;
	EventStack *prev;
	bool State;
	char EventName[128];
} EventStack;

//
// ScriptConverse class
//

class ScriptedConverse : public skScriptedExecutable
{
public:
	ScriptedConverse(char *fileName);
	~ScriptedConverse();
	bool getValue (const skString& fieldName, const skString& attribute, skRValue& value);
    bool setValue (const skString& fieldName, const skString& attribute, const skRValue& value);
    bool method (const skString& methodName, skRValueArray& arguments,skRValue& returnValue,skExecutableContext &ctxt);
	int DoConversation(int charpersec);

//not commented, from picklses newsource.zip, nouts soundconversation...
	int DoSoundConversation(int charpersec);

	void TextDisplay(char *Text, int Width, int Font);
	int TextOut(int startline, int Height, int Font, int X, int Y);
	void CreateCamera()
	{ M_Camera = geCamera_Create(2.0f, &M_CameraRect); }
	void DestroyCamera()
	{ geCamera_Destroy(&M_Camera); }

	char	Order[64];
	geBitmap	*Background;

//not commented, from picklses newsource.zip, nouts soundconversation...
	geBitmap	*rBackground;

	geBitmap	*Icon;

//not commented, from picklses newsource.zip, nouts soundconversation...
	geBitmap	*rIcon;

	
//begin add 16/09/2003 Nout
	int SpeachWindowX;
	int SpeachWindowY;
	int ReplyWindowX;
	int ReplyWindowY;
	int rBackgroundX;
	int rBackgroundY;
	bool DrawBackground;
	bool DrawrBackground;
	bool ReplyInSpeakWindow;
	StreamingAudio *m_Streams_reply;
	CString ReplySoundFileName[9];
	bool BlackClear;
	geVec3d	 PawnPos;
//end add 16/09/2003 Nout

//Sound Conversation 
//Begin add Nout 23092003
	int SpeakShowTime;
	int ReplyShowTime;
//End add Nout 23092003

	int BackgroundX;
	int BackgroundY;
	int IconX;
	int IconY;
	int SpeachX;
	int SpeachY;
	int SpeachWidth;
	int SpeachHeight;
	int SpeachFont;
	int ReplyX;
	int ReplyY;
	int ReplyWidth;
	int ReplyHeight;
	int ReplyFont;
	bool replyflg[9];
	CString Text;
	StreamingAudio *m_Streams;

//Changed nout 16/09/2003 Nout
/* not commented, from picklses newsource.zip, nouts soundconversation...
	StreamingAudio *m_Streams_reply;*/

	LPDIRECTSOUND m_dsPtr;
	CString Reply[9];

//Changed nout 16/09/2003 Nout
/* not commented, from picklses newsource.zip, nouts soundconversation...
	CString ReplySoundFileName[9];
	bool BlackClear;*/

	CStringArray TextLines;
	bool ConvFlag;
private:
	geCamera *M_Camera;
	geRect 	 M_CameraRect;
	
};


//
// ScriptedObject class
//

class ScriptedObject : public skScriptedExecutable
{
public:
	ScriptedObject(char *fileName);
	~ScriptedObject();
	bool getValue (const skString& fieldName, const skString& attribute, skRValue& value);
    bool setValue (const skString& fieldName, const skString& attribute, const skRValue& value);
    bool method (const skString& methodName, skRValueArray& arguments,skRValue& returnValue,skExecutableContext &ctxt);//change simkin
	bool highmethod(const skString& methodName, skRValueArray& arguments,skRValue& returnValue,skExecutableContext &ctxt);
	bool lowmethod(const skString& methodName, skRValueArray& arguments,skRValue& returnValue,skExecutableContext &ctxt);
	void Push();
	void Pop();
	void RemoveTriggerStack(TriggerStack *tpool);
	void GetAngles(bool flag);
// Quake 2 movement functions
	bool CheckBottom();
	bool MoveStep(geVec3d move);
	bool StepDirection(float yaw, float dist);
	void ChangeYaw();
	void ChangePitch();
	void NewChaseDir(float dist);
	float anglemod(float a);
	bool CloseEnough(float dist);
	void MoveToGoal(float dist);
	bool		active;
	bool		alive;
	bool		highlevel;
	char		ActorName[64];
	float		ActorAlpha;
	char		szName[128];
	float		AnimSpeed;
	geVec3d		Rotation;
	float		YRotation;
	geFloat		Scale;
	GE_RGBA		FillColor;
	GE_RGBA		AmbientColor;
// changed QD 07/21/04
	bool		AmbientLightFromFloor;
// end change
	bool		EnvironmentMapping;
	bool		AllMaterial;
	float		PercentMapping;
	float		PercentMaterial;
	float		ShadowSize;
// changed QD 06/26/04
	geFloat		ShadowAlpha;
	char		ShadowBitmap[64];
	char		ShadowAlphamap[64];
// end change
// begin change gekido 
	// projected shadows configurable per pawn type
	bool		ProjectedShadows;
// end change
// changed QD Shadows
	geBoolean	StencilShadows;
// end change
	bool		HideFromRadar;
	char		ChangeMaterial[64];
	char		Attribute[64];
	int			OldAttributeAmount;
	float		Gravity;
	char		BoxAnim[64];
	geVec3d		Location;
	char		RootBone[64];
	char		Order[64];
	char		Point[64];
	bool		RunOrder;
	geVec3d		CurrentPoint;
	bool		ValidPoint;
	bool		ActionActive;
	bool		StartAction;
	geVec3d		TempPoint;
	geVec3d		Vec2Point;
	char		NextOrder[64];
	float		Time;
	bool		DistActive;
	float		MinDistance;
	char		DistOrder[64];
	char		PainOrder[64];
	bool		PainActive;
	int			PainPercent;
	char		AvoidOrder[64];
	char		DeadOrder[64];
	geVec3d		DeadPos;
	int			Direction;
	float		TotalDist;
	bool		AvoidMode;
	bool		TriggerWait;
	float		TriggerTime;
	char		TriggerOrder[64];
	int			SoundIndex;
	float		AudibleRadius;
	bool		FacePlayer;
	bool		FaceAxis;
	bool		UseKey;
	float		FOV;
	char		FOVBone[64];
	char		Group[64];
	bool		HostilePlayer;
	bool		HostileDiff;
	bool		HostileSame;
	bool		TargetFind;
	char		TargetOrder[64];
	char		TargetAttr[64];
	float		TargetDistance;
	bool		TargetDisable;
	char		TargetGroup[64];
	geVec3d		LastTargetPoint;
	geVec3d		SavePoint;
	bool		collision;
	bool		pushable;
	bool		console;
	char		DamageAttr[64];
	geVec3d		UpdateTargetPoint;
	char		*ConsoleHeader;
	char		*ConsoleError;
	char		*ConsoleDebug[DEBUGLINES];
	char		Indicate[2];
	geVec3d		WRotation;
	geFloat		WScale;
	bool		SoundLoop;
	float		Circle;

    //FindPointOrder code.
	//added quest of dreams
	bool PointFind;
	char PointOrder[64];

// Low Level variables

	float		lowTime;
	float		ThinkTime;
	float		ElapseTime;
	geActor		*TargetActor;
	char		thinkorder[64];
	float		attack_finished;
	int			attack_state;
	float		yaw_speed;
	float		ideal_yaw;
	float		actoryaw;
	float		targetyaw;
	float		actorpitch;
	float		targetpitch;
	float		pitch_speed;
	float		ideal_pitch;
	geActor		*Actor;
	geActor		*WeaponActor;
	ActionList	*Top;
	ActionList	*Bottom;
	ActionList	*Index;
	ActionStack *Stack;
	TriggerStack *Trigger;
	geBitmap	*Icon;
	ScriptedConverse *Converse;

private:
	void AddAction(int Action, char *AnimName, float Speed, bool Flag,
		float Value1, float Value2, float Value3, float Value4,
		char *Sound, char *Trigger);

};

typedef struct ConversationText
{
	CString Text;
	CString Name;
} ConversationText;

typedef struct PreCache
{
	CString Name;
	geBitmap *Bitmap;
} PreCache;

typedef struct WeaponPreCache
{
	CString		Name;
	char		ActorName[64];
	geVec3d		Rotation;
	geFloat		Scale;
	GE_RGBA		FillColor;
	GE_RGBA		AmbientColor;
// changed QD 07/21/04
	bool		AmbientLightFromFloor;
// end change
	bool		EnvironmentMapping;
	bool		AllMaterial;
	float		PercentMapping;
	float		PercentMaterial;
} WeaponPreCache;

class CPawn : public CRGFComponent
{
public:
	CPawn();	// Constructor
	~CPawn();
	void Tick(float dwTicks);
	int HandleCollision(geActor *pActor, geActor *theActor, bool Gravity);
	int SaveTo(FILE *SaveFD, bool type);
	int RestoreFrom(FILE *RestoreFD, bool type);
	bool Converse(geActor *pActor);
	void RunConverse(ScriptedConverse *Converse, char *szName, geBitmap *OIcon);
	int LocateEntity(char *szName, void **pEntityData);
	void AddEvent(char *Event, bool State);
	bool GetEventState(char *Event);
	CString GetText(char *Name);
	geBitmap *GetCache(char *Name);
	void AnimateWeapon();
	bool GetConvFlag()
	{ return ConvFlag; }
	void SetConvFlag(bool flag)
	{ ConvFlag = flag; }
	bool GetPawnFlag(int index)
	{ return PawnFlag[index]; }
	void SetPawnFlag(int index, bool flag)
	{ PawnFlag[index] = flag; }
	bool CanSee(float FOV, geActor *Actor, geActor *TargetActor, char *Bone);
	//FindPointOrder code
	//Added QuestOfDreams RF07E
	bool CanSeePoint(float FOV, geActor *Actor, geVec3d *TargetPoint, char *Bone);
	int GetBlock()
	{ return ConsoleBlock; }
	void IncBlock()
	{ ConsoleBlock += 1; }
	void ParmCheck(int Entries, int Desired, char *Order, char *szName, const skString& methodname);
	void LoadConv(char *convtxt);
private:
	void TickHigh(Pawn *pSource, ScriptedObject *Object, float dwTicks);
	void TickLow(Pawn *pSource, ScriptedObject *Object, float dwTicks);
	void Spawn(void *Data);
	bool RotateToPoint(void *Data, float dwTicks);
	bool RotateAroundPoint(void *Data, float dwTicks);
	bool RotateToAlign(void *Data, float dwTicks);
	bool Rotate(void *Data, float dwTicks);
	bool MoveToPoint(void *Data, float dwTicks);
	bool Move(void *Data, float dwTicks);
	bool RotateMoveToPoint(void *Data, float dwTicks);
	bool RotateMove(void *Data, float dwTicks);
	bool NextPoint(void *Data, float dwTicks);
	bool NextOrder(void *Data, float dwTicks);
	bool NextPath(void *Data, float dwTicks);
	bool Jump(void *Data, float dwTicks);
	bool AddTriggerOrder(void *Data, float dwTicks);
	bool DelTriggerOrder(void *Data, float dwTicks);
	bool RotateToPlayer(void *Data, float dwTicks);
	void PreLoad(char *filename);
	void PreLoadC(char *filename);
	bool PlayerDistance(float FOV, float distance, geActor *Actor, geVec3d DeadPos, char *Bone);



	CIniFile AttrFile;
	geBitmap	*Background;

//not commented, from picklses newsource.zip, nouts soundconversation...
	geBitmap	*rBackground;

	geBitmap	*Icon;

//not commented, from picklses newsource.zip, nouts soundconversation...
	geBitmap	*rIcon;

//begin add 16/09/2003 Nout
	int SpeachWindowX;
	int SpeachWindowY;
	int ReplyWindowX;
	int ReplyWindowY;
	int rBackgroundX;
	int rBackgroundY;
//begin add 16/09/2003 Nout

	int BackgroundX;
	int BackgroundY;
	int IconX;
	int IconY;
	int SpeachX;
	int SpeachY;
	int SpeachWidth;
	int SpeachHeight;
	int SpeachFont;
	int ReplyX;
	int ReplyY;
	int ReplyWidth;
	int ReplyHeight;
	int ReplyFont;
	bool ConvFlag;
	bool PawnFlag[MAXFLAGS];
	CArray<ConversationText, ConversationText> Text;
	EventStack *Events;
	CArray<WeaponPreCache, WeaponPreCache> WeaponCache;
	CArray<PreCache, PreCache> Cache;
	int ConsoleBlock;



private:

};

#endif