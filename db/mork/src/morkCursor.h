




































#ifndef _MORKCURSOR_
#define _MORKCURSOR_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif



#define morkDerived_kCursor 0x4375 /* ascii 'Cu' */

class morkCursor : public morkObject, public nsIMdbCursor{ 


  
  
  
  
  
  
  
  
  

  
  

public: 
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD IsFrozenMdbObject(nsIMdbEnv* ev, mdb_bool* outIsReadonly);
  
  

  
  NS_IMETHOD GetWeakRefCount(nsIMdbEnv* ev, 
    mdb_count* outCount);  
  NS_IMETHOD GetStrongRefCount(nsIMdbEnv* ev, 
    mdb_count* outCount);

  NS_IMETHOD AddWeakRef(nsIMdbEnv* ev);
  NS_IMETHOD AddStrongRef(nsIMdbEnv* ev);

  NS_IMETHOD CutWeakRef(nsIMdbEnv* ev);
  NS_IMETHOD CutStrongRef(nsIMdbEnv* ev);
  
  NS_IMETHOD CloseMdbObject(nsIMdbEnv* ev); 
  NS_IMETHOD IsOpenMdbObject(nsIMdbEnv* ev, mdb_bool* outOpen);
  
  




  
  NS_IMETHOD GetCount(nsIMdbEnv* ev, mdb_count* outCount); 
  NS_IMETHOD GetSeed(nsIMdbEnv* ev, mdb_seed* outSeed);    
  
  NS_IMETHOD SetPos(nsIMdbEnv* ev, mdb_pos inPos);   
  NS_IMETHOD GetPos(nsIMdbEnv* ev, mdb_pos* outPos);
  
  NS_IMETHOD SetDoFailOnSeedOutOfSync(nsIMdbEnv* ev, mdb_bool inFail);
  NS_IMETHOD GetDoFailOnSeedOutOfSync(nsIMdbEnv* ev, mdb_bool* outFail);
  


    
  

  mork_seed  mCursor_Seed;
  mork_pos   mCursor_Pos;
  mork_bool  mCursor_DoFailOnSeedOutOfSync;
  mork_u1    mCursor_Pad[ 3 ]; 
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkCursor(); 
  
public: 
  morkCursor(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap);
  void CloseCursor(morkEnv* ev); 

private: 
  morkCursor(const morkCursor& other);
  morkCursor& operator=(const morkCursor& other);

public: 
  mork_bool IsCursor() const
  { return IsNode() && mNode_Derived == morkDerived_kCursor; }


public: 

public: 
  static void SlotWeakCursor(morkCursor* me,
    morkEnv* ev, morkCursor** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongCursor(morkCursor* me,
    morkEnv* ev, morkCursor** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};




#endif 
