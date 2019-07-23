




































#ifndef _MORKSPACE_
#define _MORKSPACE_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKBEAD_
#include "morkBead.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif



#define morkSpace_kInitialSpaceSlots 1024 /* default */
#define morkDerived_kSpace 0x5370 /* ascii 'Sp' */



class morkSpace : public morkBead { 


  

  
  
  
  
  
  
  
  
  
  

  

public: 

  mork_tid     SpaceScope() const { return mBead_Color; }
  void         SetSpaceScope(mork_scope inScope) { mBead_Color = inScope; }

public: 

  morkStore*  mSpace_Store; 
    
  mork_bool   mSpace_DoAutoIDs;    
  mork_bool   mSpace_HaveDoneAutoIDs; 
  mork_bool   mSpace_CanDirty; 
  mork_u1     mSpace_Pad;    

public: 
  void SetSpaceDirty() { this->SetNodeDirty(); }
  void SetSpaceClean() { this->SetNodeClean(); }
  
  mork_bool IsSpaceClean() const { return this->IsNodeClean(); }
  mork_bool IsSpaceDirty() const { return this->IsNodeDirty(); }


public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkSpace(); 
  
public: 
  
  
  
  morkSpace(morkEnv* ev, const morkUsage& inUsage,mork_scope inScope, 
    morkStore* ioStore, nsIMdbHeap* ioNodeHeap, nsIMdbHeap* ioSlotHeap);
  void CloseSpace(morkEnv* ev); 

public: 
  mork_bool IsSpace() const
  { return IsNode() && mNode_Derived == morkDerived_kSpace; }


public: 
  
  mork_bool MaybeDirtyStoreAndSpace();

  static void NonAsciiSpaceScopeName(morkEnv* ev);
  static void NilSpaceStoreError(morkEnv* ev);

  morkPool* GetSpaceStorePool() const;

public: 
  static void SlotWeakSpace(morkSpace* me,
    morkEnv* ev, morkSpace** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongSpace(morkSpace* me,
    morkEnv* ev, morkSpace** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
