/*
	Collider.h	:	Collision and Contents Handling Class


	This file contains the class declaration for the Collider collision
and contents checking and handling class.  This is a utility class
to be used by other RGF classes.
*/

#ifndef __COLLIDER_H_
#define __COLLIDER_H_

typedef struct TraceData
{
	float	fraction;
	geVec3d	endpos;
	bool startsolid;
	bool allsolid;
} TraceData;

class Collider
{
public:
  Collider();						// Default constructor
	~Collider();					// Default destructor
	void IgnoreContents(bool bFlag);
	void CheckLevel(int nLevel);
	// GetContentsOf gets the contents of an ExtBox zone
  int GetContentsOf(geVec3d Position, geExtBox *theBox, GE_Contents *theContents);
  int GetContentsOf(geVec3d Position, geExtBox *theBox, 
				geActor *theActor, GE_Contents *theContents);
	// CheckActorCollision performs a detailed collision check on the
	// ..actor at its current position to see if its in collision with
	// ..anything.
  bool CheckActorCollision(geActor *theActor);
  bool CheckActorCollision(geVec3d *Position, geActor *theActor);
  bool CheckActorCollision(geVec3d Start, geVec3d End, geActor *theActor);
  bool CheckActorCollision(geVec3d Start, geVec3d End, geActor *theActor,
							GE_Collision *Collision);
	// CheckCollision is a general-purpose collision checking system that tests
	// ..both the geWorld_Collision and GetContents.  The second version will
	// ..return the collision results to the caller, the third version will
	// ..return the contents to the caller, the fourth version will return
	// ..BOTH collision and contents data
  bool CheckForCollision(geVec3d *Min, geVec3d *Max,geVec3d OldPosition, 
					geVec3d NewPosition);
  bool CheckForCollision(geVec3d *Min, geVec3d *Max, geVec3d OldPosition, 
			geVec3d NewPosition, GE_Collision *Collision);
  bool CheckForCollision(geVec3d *Min, geVec3d *Max, geVec3d OldPosition, 
			geVec3d NewPosition, GE_Contents *Contents);
  bool CheckForCollision(geVec3d *Min, geVec3d *Max, geVec3d OldPosition, 
			geVec3d NewPosition, GE_Collision *Collision,	GE_Contents *Contents);
  bool CheckForCollision(geVec3d *Min, geVec3d *Max,
			geVec3d OldPosition, geVec3d NewPosition, GE_Collision *Collision,
			geActor *Actor);
  bool CheckForWCollision(geVec3d *Min, geVec3d *Max,
			geVec3d OldPosition, geVec3d NewPosition, GE_Collision *Collision,
			geActor *Actor);
	// CheckModelMotion checks to see if an animated world model can move
	// ..from Point A to Point B, given a potential obstructing object
	// ..of size 'ObstructionSize' at position 'ObstructionPosition'.
	// ..The second override takes a min/max pair instead of an ExtBox.
  bool CheckModelMotion(geWorld_Model *theModel, geXForm3d *ModelPosition,
			geExtBox *ObstructionSize, geVec3d *ObstructionPosition,	
			geVec3d *OutputObstructionPosition);
  bool CheckModelMotion(geWorld_Model *theModel, geXForm3d *ModelPosition,
			geVec3d *ObstructionMin, geVec3d *ObstructionMax, geVec3d *ObstructionPosition,	
			geVec3d *OutputObstructionPosition);
  bool CheckModelMotionActor(geWorld_Model *theModel, geXForm3d *ModelPosition,
			geActor *theActor, geVec3d *ActorPosition, geVec3d *OutputActorPosition);
	// CanOccupyPosition checks to see if a specific zone in the level is
	// ..occupiable (??) by something of the specified size 'theBox'.
	bool CanOccupyPosition(geVec3d *thePoint, geExtBox *theBox);
  bool CanOccupyPosition(geVec3d *thePoint, geExtBox *theBox, geActor *Actor);
  bool CanOccupyPosition(geVec3d *thePoint, geExtBox *theBox,
		geActor *Actor, GE_Contents *Contents);
	// This flavor of CheckSubCollision checks to see which two bones of
	// ..a set of actors, one moving and one static, have intersected.
	// ..Note you have to have already MOVED the moving actor for this
	// ..function to operate.
  geBoolean CheckSubCollision(geWorld *World, geActor *Actor, geActor *StaticActor,
			int *MovingBoneHit, int *StaticBoneHit);
  void GetMoveBox(const geVec3d *Mins, const geVec3d *Maxs, const geVec3d *Front, const geVec3d *Back, geVec3d *OMins, geVec3d *OMaxs);
  geBoolean CheckBoneCollision(geWorld *World, geActor *StaticActor, geVec3d *Position, geVec3d *Min, geVec3d *Max);
  // Probe allows collision look-ahead to a certain extent, point to point.
	geBoolean Probe(geXForm3d theXForm, float fDistance, GE_Collision *theCollision);
	geBoolean Probe(geXForm3d theXForm, float fDistance, GE_Collision *theCollision,
				geActor *theActor);
	// ProcessCollision handles dispatching collisions to entities
  int ProcessCollision(GE_Collision theCollision, geActor *theActor, bool Gravity);
  int GetLastCollisionCause() { return LastCollisionReason;};
// Debug Routines
  bool CheckActorCollisionD(geVec3d Start, geVec3d End, geActor *theActor,
							GE_Collision *Collision);
  bool CheckForCollisionD(geVec3d *Min, geVec3d *Max,
			geVec3d OldPosition, geVec3d NewPosition, GE_Collision *Collision,
			geActor *Actor);
  bool CanOccupyPositionD(geVec3d *thePoint, geExtBox *theBox,
		geActor *Actor, GE_Contents *Contents);
  void Debug();
// changed RF064
  bool CheckForBoneCollision(geVec3d *Min, geVec3d *Max,
			geVec3d OldPosition, geVec3d NewPosition, GE_Collision *Collision,
			geActor *Actor, char *BoneHit, bool BoneLevel);
// end change RF064
  TraceData Trace(geVec3d *Start, geVec3d *End, geVec3d *Min, geVec3d *Max,
	  geActor *Actor, GE_Collision *Collision);
private:
  geBoolean CheckIntersection(geActor *Actor, geWorld *World);
  geBoolean CheckIntersection(geVec3d *Position, geActor *Actor, geWorld *World);
  geBoolean CheckSubCollision(geVec3d *Start, geVec3d *End, geActor *Actor, 
			geWorld *World);
  geBoolean CheckSubCollision(geVec3d *Start, geVec3d *End, geActor *Actor, 
			geWorld *World, GE_Collision *Collision);
	int LastCollisionReason;					// Reason for last collision
	int m_CheckLevel;									// Collision check level
	bool m_IgnoreContents;						// Ignore collision zone contents
  char szLastBoneCollision[256];		// Name of last bone collided, if any
// Debug Variables
  char Flag1[256];
  int Flag2, Flag3;
};

#endif
