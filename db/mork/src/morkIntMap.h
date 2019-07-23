




































#ifndef _MORKINTMAP_
#define _MORKINTMAP_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif



#define morkDerived_kIntMap 0x694D /* ascii 'iM' */

#define morkIntMap_kStartSlotCount 256



class morkIntMap : public morkMap { 


public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkIntMap(); 
  
public: 

  
  morkIntMap(morkEnv* ev, const morkUsage& inUsage, mork_size inValSize,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap, mork_bool inHoldChanges);
  void CloseIntMap(morkEnv* ev); 

public: 
  mork_bool IsIntMap() const
  { return IsNode() && mNode_Derived == morkDerived_kIntMap; }



  virtual mork_bool 
  Equal(morkEnv* ev, const void* inKeyA, const void* inKeyB) const;

  virtual mork_u4 
  Hash(morkEnv* ev, const void* inKey) const;


public: 

  mork_bool  AddInt(morkEnv* ev, mork_u4 inKey, void* ioAddress);
  

  mork_bool  CutInt(morkEnv* ev, mork_u4 inKey);
  
  
  void*      GetInt(morkEnv* ev, mork_u4 inKey);
  
  
  mork_bool  HasInt(morkEnv* ev, mork_u4 inKey);
  

};



#ifdef MORK_POINTER_MAP_IMPL

#define morkDerived_kPointerMap 0x704D /* ascii 'pM' */

#define morkPointerMap_kStartSlotCount 256










class morkPointerMap : public morkMap { 


public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkPointerMap(); 
  
public: 

  
  morkPointerMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap);
  void ClosePointerMap(morkEnv* ev); 

public: 
  mork_bool IsPointerMap() const
  { return IsNode() && mNode_Derived == morkDerived_kPointerMap; }



  virtual mork_bool 
  Equal(morkEnv* ev, const void* inKeyA, const void* inKeyB) const;

  virtual mork_u4 
  Hash(morkEnv* ev, const void* inKey) const;


public: 

  mork_bool  AddPointer(morkEnv* ev, void* inKey, void* ioAddress);
  

  mork_bool  CutPointer(morkEnv* ev, void* inKey);
  
  
  void*      GetPointer(morkEnv* ev, void* inKey);
  
  
  mork_bool  HasPointer(morkEnv* ev, void* inKey);
  

public: 
  static void SlotWeakIntMap(morkIntMap* me,
    morkEnv* ev, morkIntMap** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongIntMap(morkIntMap* me,
    morkEnv* ev, morkIntMap** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }

};
#endif 




#endif 
