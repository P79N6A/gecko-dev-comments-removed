




































#ifndef _MORKARRAY_
#define _MORKARRAY_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif



#define morkDerived_kArray 0x4179 /* ascii 'Ay' */

class morkArray : public morkNode { 


  
  
  
  
  
  
  
  
  

public: 
  void**       mArray_Slots; 
  nsIMdbHeap*  mArray_Heap;  
  mork_fill    mArray_Fill;  
  mork_size    mArray_Size;  
  mork_seed    mArray_Seed;  
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkArray(); 
  
public: 
  morkArray(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, mork_size inSize, nsIMdbHeap* ioSlotHeap);
  void CloseArray(morkEnv* ev); 

private: 
  morkArray(const morkArray& other);
  morkArray& operator=(const morkArray& other);

public: 
  mork_bool IsArray() const
  { return IsNode() && mNode_Derived == morkDerived_kArray; }


public: 
  static void NonArrayTypeError(morkEnv* ev);
  static void IndexBeyondEndError(morkEnv* ev);
  static void NilSlotsAddressError(morkEnv* ev);
  static void FillBeyondSizeError(morkEnv* ev);

public: 

  mork_fill  Length() const { return mArray_Fill; }
  mork_size  Capacity() const { return mArray_Size; }
  
  mork_bool  Grow(morkEnv* ev, mork_size inNewSize);
  
  
  void*      At(mork_pos inPos) const { return mArray_Slots[ inPos ]; }
  void       AtPut(mork_pos inPos, void* ioSlot)
  { mArray_Slots[ inPos ] = ioSlot; }
  
  void*      SafeAt(morkEnv* ev, mork_pos inPos);
  void       SafeAtPut(morkEnv* ev, mork_pos inPos, void* ioSlot);
  
  mork_pos   AppendSlot(morkEnv* ev, void* ioSlot);
  void       AddSlot(morkEnv* ev, mork_pos inPos, void* ioSlot);
  void       CutSlot(morkEnv* ev, mork_pos inPos);
  void       CutAllSlots(morkEnv* ev);

public: 
  static void SlotWeakArray(morkArray* me,
    morkEnv* ev, morkArray** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongArray(morkArray* me,
    morkEnv* ev, morkArray** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
