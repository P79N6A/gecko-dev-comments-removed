




































#ifndef _MORKATOMMAP_
#define _MORKATOMMAP_ 1

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

#ifndef _MORKINTMAP_
#include "morkIntMap.h"
#endif



#define morkDerived_kAtomAidMap 0x6141 /* ascii 'aA' */

#define morkAtomAidMap_kStartSlotCount 23



#ifdef MORK_ENABLE_PROBE_MAPS
class morkAtomAidMap : public morkProbeMap { 
#else
class morkAtomAidMap : public morkMap { 
#endif


public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkAtomAidMap(); 
  
public: 
  morkAtomAidMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap);
  void CloseAtomAidMap(morkEnv* ev); 

public: 
  mork_bool IsAtomAidMap() const
  { return IsNode() && mNode_Derived == morkDerived_kAtomAidMap; }


public:
#ifdef MORK_ENABLE_PROBE_MAPS
  
  virtual mork_test 
  MapTest(morkEnv* ev, const void* inMapKey, const void* inAppKey) const;

  virtual mork_u4 
  MapHash(morkEnv* ev, const void* inAppKey) const;

  virtual mork_u4 ProbeMapHashMapKey(morkEnv* ev, const void* inMapKey) const;

  

  
  

  
  
  

  
  
  
  
#else 

  virtual mork_bool 
  Equal(morkEnv* ev, const void* inKeyA, const void* inKeyB) const;
  

  virtual mork_u4 
  Hash(morkEnv* ev, const void* inKey) const;
  

#endif 

public: 

  mork_bool      AddAtom(morkEnv* ev, morkBookAtom* ioAtom);
  

  morkBookAtom*  CutAtom(morkEnv* ev, const morkBookAtom* inAtom);
  
  
  morkBookAtom*  GetAtom(morkEnv* ev, const morkBookAtom* inAtom);
  

  morkBookAtom*  GetAid(morkEnv* ev, mork_aid inAid);
  

  

public: 
  static void SlotWeakAtomAidMap(morkAtomAidMap* me,
    morkEnv* ev, morkAtomAidMap** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongAtomAidMap(morkAtomAidMap* me,
    morkEnv* ev, morkAtomAidMap** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};

#ifdef MORK_ENABLE_PROBE_MAPS
class morkAtomAidMapIter: public morkProbeMapIter { 
#else
class morkAtomAidMapIter: public morkMapIter { 
#endif

public:
#ifdef MORK_ENABLE_PROBE_MAPS
  morkAtomAidMapIter(morkEnv* ev, morkAtomAidMap* ioMap)
  : morkProbeMapIter(ev, ioMap) { }
 
  morkAtomAidMapIter( ) : morkProbeMapIter()  { }
#else 
  morkAtomAidMapIter(morkEnv* ev, morkAtomAidMap* ioMap)
  : morkMapIter(ev, ioMap) { }
 
  morkAtomAidMapIter( ) : morkMapIter()  { }
#endif 

  void InitAtomAidMapIter(morkEnv* ev, morkAtomAidMap* ioMap)
  { this->InitMapIter(ev, ioMap); }
   
  mork_change* FirstAtom(morkEnv* ev, morkBookAtom** outAtomPtr)
  { return this->First(ev, outAtomPtr,  (void*) 0); }
  
  mork_change* NextAtom(morkEnv* ev, morkBookAtom** outAtomPtr)
  { return this->Next(ev, outAtomPtr,  (void*) 0); }
  
  mork_change* HereAtom(morkEnv* ev, morkBookAtom** outAtomPtr)
  { return this->Here(ev, outAtomPtr,  (void*) 0); }
  
  mork_change* CutHereAtom(morkEnv* ev, morkBookAtom** outAtomPtr)
  { return this->CutHere(ev, outAtomPtr,  (void*) 0); }
};






#define morkDerived_kAtomBodyMap 0x6142 /* ascii 'aB' */

#define morkAtomBodyMap_kStartSlotCount 23



#ifdef MORK_ENABLE_PROBE_MAPS
class morkAtomBodyMap : public morkProbeMap { 
#else
class morkAtomBodyMap : public morkMap { 
#endif


public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkAtomBodyMap(); 
  
public: 
  morkAtomBodyMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap);
  void CloseAtomBodyMap(morkEnv* ev); 

public: 
  mork_bool IsAtomBodyMap() const
  { return IsNode() && mNode_Derived == morkDerived_kAtomBodyMap; }


public:
#ifdef MORK_ENABLE_PROBE_MAPS
  
  virtual mork_test 
  MapTest(morkEnv* ev, const void* inMapKey, const void* inAppKey) const;

  virtual mork_u4 
  MapHash(morkEnv* ev, const void* inAppKey) const;

  virtual mork_u4 ProbeMapHashMapKey(morkEnv* ev, const void* inMapKey) const;

  

  
  

  
  
  

  
  
  
  
#else 

  virtual mork_bool 
  Equal(morkEnv* ev, const void* inKeyA, const void* inKeyB) const;
  

  virtual mork_u4 
  Hash(morkEnv* ev, const void* inKey) const;
  

#endif 

public: 

  mork_bool      AddAtom(morkEnv* ev, morkBookAtom* ioAtom);
  

  morkBookAtom*  CutAtom(morkEnv* ev, const morkBookAtom* inAtom);
  
  
  morkBookAtom*  GetAtom(morkEnv* ev, const morkBookAtom* inAtom);
  

  

public: 
  static void SlotWeakAtomBodyMap(morkAtomBodyMap* me,
    morkEnv* ev, morkAtomBodyMap** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongAtomBodyMap(morkAtomBodyMap* me,
    morkEnv* ev, morkAtomBodyMap** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};

#ifdef MORK_ENABLE_PROBE_MAPS
class morkAtomBodyMapIter: public morkProbeMapIter{ 
#else
class morkAtomBodyMapIter: public morkMapIter{ 
#endif

public:
#ifdef MORK_ENABLE_PROBE_MAPS
  morkAtomBodyMapIter(morkEnv* ev, morkAtomBodyMap* ioMap)
  : morkProbeMapIter(ev, ioMap) { }
 
  morkAtomBodyMapIter( ) : morkProbeMapIter()  { }
#else 
  morkAtomBodyMapIter(morkEnv* ev, morkAtomBodyMap* ioMap)
  : morkMapIter(ev, ioMap) { }
 
  morkAtomBodyMapIter( ) : morkMapIter()  { }
#endif 
  
  void InitAtomBodyMapIter(morkEnv* ev, morkAtomBodyMap* ioMap)
  { this->InitMapIter(ev, ioMap); }
   
  mork_change* FirstAtom(morkEnv* ev, morkBookAtom** outAtomPtr)
  { return this->First(ev, outAtomPtr,  (void*) 0); }
  
  mork_change* NextAtom(morkEnv* ev, morkBookAtom** outAtomPtr)
  { return this->Next(ev, outAtomPtr,  (void*) 0); }
  
  mork_change* HereAtom(morkEnv* ev, morkBookAtom** outAtomPtr)
  { return this->Here(ev, outAtomPtr,  (void*) 0); }
  
  mork_change* CutHereAtom(morkEnv* ev, morkBookAtom** outAtomPtr)
  { return this->CutHere(ev, outAtomPtr,  (void*) 0); }
};



#define morkDerived_kAtomRowMap 0x6152 /* ascii 'aR' */



class morkAtomRowMap : public morkIntMap { 

public:
  mork_column mAtomRowMap_IndexColumn; 

public:

  virtual ~morkAtomRowMap();
  morkAtomRowMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap, mork_column inIndexColumn);

public: 

  void  AddRow(morkEnv* ev, morkRow* ioRow);
  

  void  CutRow(morkEnv* ev, morkRow* ioRow);
  

public: 

  mork_bool  AddAid(morkEnv* ev, mork_aid inAid, morkRow* ioRow)
  { return this->AddInt(ev, inAid, ioRow); }
  

  mork_bool  CutAid(morkEnv* ev, mork_aid inAid)
  { return this->CutInt(ev, inAid); }
  
  
  morkRow*   GetAid(morkEnv* ev, mork_aid inAid)
  { return (morkRow*) this->GetInt(ev, inAid); }
  
  
public: 
  mork_bool IsAtomRowMap() const
  { return IsNode() && mNode_Derived == morkDerived_kAtomRowMap; }

public: 
  static void SlotWeakAtomRowMap(morkAtomRowMap* me,
    morkEnv* ev, morkAtomRowMap** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongAtomRowMap(morkAtomRowMap* me,
    morkEnv* ev, morkAtomRowMap** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }

};

class morkAtomRowMapIter: public morkMapIter{ 

public:
  morkAtomRowMapIter(morkEnv* ev, morkAtomRowMap* ioMap)
  : morkMapIter(ev, ioMap) { }
 
  morkAtomRowMapIter( ) : morkMapIter()  { }
  void InitAtomRowMapIter(morkEnv* ev, morkAtomRowMap* ioMap)
  { this->InitMapIter(ev, ioMap); }
   
  mork_change*
  FirstAtomAndRow(morkEnv* ev, morkAtom** outAtom, morkRow** outRow)
  { return this->First(ev, outAtom, outRow); }
  
  mork_change*
  NextAtomAndRow(morkEnv* ev, morkAtom** outAtom, morkRow** outRow)
  { return this->Next(ev, outAtom, outRow); }
  
  mork_change*
  HereAtomAndRow(morkEnv* ev, morkAtom** outAtom, morkRow** outRow)
  { return this->Here(ev, outAtom, outRow); }
  
  mork_change*
  CutHereAtomAndRow(morkEnv* ev, morkAtom** outAtom, morkRow** outRow)
  { return this->CutHere(ev, outAtom, outRow); }
};




#endif
