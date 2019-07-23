




































#ifndef _MORKOBJECT_
#define _MORKOBJECT_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKBEAD_
#include "morkBead.h"
#endif

#ifndef _MORKCONFIG_
#include "morkConfig.h"
#endif

#ifndef _ORKINHEAP_
#include "orkinHeap.h"
#endif



#define morkDerived_kObject 0x6F42 /* ascii 'oB' */





class morkObject : public morkBead, public nsIMdbObject { 


  

  
  
  
  
  
  
  
  
  
  

  
  
public: 

  morkHandle*      mObject_Handle;   

  morkEnv * mMorkEnv; 

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkObject(); 
#ifdef MORK_DEBUG_HEAP_STATS
  void operator delete(void* ioAddress, size_t size)
  { 
    mork_u4* array = (mork_u4*) ioAddress;
    array -= 3;
    orkinHeap *heap = (orkinHeap *) *array;
    if (heap)
      heap->Free(nsnull, ioAddress);
  }
#endif

  NS_DECL_ISUPPORTS

    
  NS_IMETHOD IsFrozenMdbObject(nsIMdbEnv* ev, mdb_bool* outIsReadonly);
  
  

  
  NS_IMETHOD GetMdbFactory(nsIMdbEnv* ev, nsIMdbFactory** acqFactory); 
  

  
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
  
  

protected: 
  morkObject(const morkUsage& inUsage, nsIMdbHeap* ioHeap,
    mork_color inBeadColor);
  
public: 
  morkObject(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap, 
     mork_color inBeadColor, morkHandle* ioHandle); 
  void CloseObject(morkEnv* ev); 

private: 
  morkObject(const morkObject& other);
  morkObject& operator=(const morkObject& other);

public: 
  mork_bool IsObject() const
  { return IsNode() && mNode_Derived == morkDerived_kObject; }


  
  
public: 
  static void SlotWeakObject(morkObject* me,
    morkEnv* ev, morkObject** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongObject(morkObject* me,
    morkEnv* ev, morkObject** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
