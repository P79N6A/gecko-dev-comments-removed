




































#ifndef _MORKROWMAP_
#define _MORKROWMAP_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKPROBEMAP_
#include "morkProbeMap.h"
#endif



#define morkDerived_kRowMap 0x724D /* ascii 'rM' */



class morkRowMap : public morkMap { 


public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkRowMap(); 
  
public: 
  morkRowMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap, mork_size inSlots);
  void CloseRowMap(morkEnv* ev); 

public: 
  mork_bool IsRowMap() const
  { return IsNode() && mNode_Derived == morkDerived_kRowMap; }



  virtual mork_bool 
  Equal(morkEnv* ev, const void* inKeyA, const void* inKeyB) const;
  

  virtual mork_u4 
  Hash(morkEnv* ev, const void* inKey) const;
  


public: 

  mork_bool AddRow(morkEnv* ev, morkRow* ioRow);
  

  morkRow*  CutOid(morkEnv* ev, const mdbOid* inOid);
  

  morkRow*  CutRow(morkEnv* ev, const morkRow* ioRow);
  
  
  morkRow*  GetOid(morkEnv* ev, const mdbOid* inOid);
  
  
  morkRow*  GetRow(morkEnv* ev, const morkRow* ioRow);
  
  
  

public: 
  static void SlotWeakRowMap(morkRowMap* me,
    morkEnv* ev, morkRowMap** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongRowMap(morkRowMap* me,
    morkEnv* ev, morkRowMap** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};

class morkRowMapIter: public morkMapIter{ 

public:
  morkRowMapIter(morkEnv* ev, morkRowMap* ioMap)
  : morkMapIter(ev, ioMap) { }
 
  morkRowMapIter( ) : morkMapIter()  { }
  void InitRowMapIter(morkEnv* ev, morkRowMap* ioMap)
  { this->InitMapIter(ev, ioMap); }
   
  mork_change* FirstRow(morkEnv* ev, morkRow** outRowPtr)
  { return this->First(ev, outRowPtr,  (void*) 0); }
  
  mork_change* NextRow(morkEnv* ev, morkRow** outRowPtr)
  { return this->Next(ev, outRowPtr,  (void*) 0); }
  
  mork_change* HereRow(morkEnv* ev, morkRow** outRowPtr)
  { return this->Here(ev, outRowPtr,  (void*) 0); }
  
  mork_change* CutHereRow(morkEnv* ev, morkRow** outRowPtr)
  { return this->CutHere(ev, outRowPtr,  (void*) 0); }
};




#define morkDerived_kRowProbeMap 0x726D /* ascii 'rm' */



class morkRowProbeMap : public morkProbeMap { 


public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkRowProbeMap(); 
  
public: 
  morkRowProbeMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap, mork_size inSlots);
  void CloseRowProbeMap(morkEnv* ev); 

public: 
  mork_bool IsRowMap() const
  { return IsNode() && mNode_Derived == morkDerived_kRowMap; }


  
  virtual mork_test 
  MapTest(morkEnv* ev, const void* inMapKey, const void* inAppKey) const;

  virtual mork_u4 
  MapHash(morkEnv* ev, const void* inAppKey) const;

  virtual mork_u4 ProbeMapHashMapKey(morkEnv* ev, const void* inMapKey) const;

  

  
  

  
  
  

  
  
  
  

public: 

  mork_bool AddRow(morkEnv* ev, morkRow* ioRow);
  

  morkRow*  CutOid(morkEnv* ev, const mdbOid* inOid);
  

  morkRow*  CutRow(morkEnv* ev, const morkRow* ioRow);
  
  
  morkRow*  GetOid(morkEnv* ev, const mdbOid* inOid);
  
  
  morkRow*  GetRow(morkEnv* ev, const morkRow* ioRow);
  
  
  

public: 
  static void SlotWeakRowProbeMap(morkRowProbeMap* me,
    morkEnv* ev, morkRowProbeMap** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongRowProbeMap(morkRowProbeMap* me,
    morkEnv* ev, morkRowProbeMap** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};

class morkRowProbeMapIter: public morkProbeMapIter{ 

public:
  morkRowProbeMapIter(morkEnv* ev, morkRowProbeMap* ioMap)
  : morkProbeMapIter(ev, ioMap) { }
 
  morkRowProbeMapIter( ) : morkProbeMapIter()  { }
  void InitRowMapIter(morkEnv* ev, morkRowProbeMap* ioMap)
  { this->InitMapIter(ev, ioMap); }
   
  mork_change* FirstRow(morkEnv* ev, morkRow** outRowPtr)
  { return this->First(ev, outRowPtr,  (void*) 0); }
  
  mork_change* NextRow(morkEnv* ev, morkRow** outRowPtr)
  { return this->Next(ev, outRowPtr,  (void*) 0); }
  
  mork_change* HereRow(morkEnv* ev, morkRow** outRowPtr)
  { return this->Here(ev, outRowPtr,  (void*) 0); }
  
  mork_change* CutHereRow(morkEnv* ev, morkRow** outRowPtr)
  { return this->CutHere(ev, outRowPtr,  (void*) 0); }
};



#endif 
