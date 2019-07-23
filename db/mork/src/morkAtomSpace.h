




































#ifndef _MORKATOMSPACE_
#define _MORKATOMSPACE_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKSPACE_
#include "morkSpace.h"
#endif

#ifndef _MORKATOMMAP_
#include "morkAtomMap.h"
#endif

#ifndef _MORKNODEMAP_
#include "morkNodeMap.h"
#endif











#define morkAtomSpace_kMinUnderId 0x80   /* low 7 bits mean byte tokens */

#define morkAtomSpace_kMaxSevenBitAid 0x7F  /* low seven bit integer ID */








#define morkAtomSpace_kMinOverId 0x1000  /* using at least four hex bytes */

#define morkDerived_kAtomSpace 0x6153 /* ascii 'aS' */

#define morkAtomSpace_kColumnScope ((mork_scope) 'c') /* column scope is forever */



class morkAtomSpace : public morkSpace { 


  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

public: 

  mork_aid         mAtomSpace_HighUnderId; 
  mork_aid         mAtomSpace_HighOverId;  
  
  morkAtomAidMap   mAtomSpace_AtomAids; 
  morkAtomBodyMap  mAtomSpace_AtomBodies; 

public: 
  void SetAtomSpaceDirty() { this->SetNodeDirty(); }
  void SetAtomSpaceClean() { this->SetNodeClean(); }
  
  mork_bool IsAtomSpaceClean() const { return this->IsNodeClean(); }
  mork_bool IsAtomSpaceDirty() const { return this->IsNodeDirty(); }


public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkAtomSpace(); 
  
public: 
  morkAtomSpace(morkEnv* ev, const morkUsage& inUsage, mork_scope inScope, 
    morkStore* ioStore, nsIMdbHeap* ioNodeHeap, nsIMdbHeap* ioSlotHeap);
  void CloseAtomSpace(morkEnv* ev); 

public: 
  mork_bool IsAtomSpace() const
  { return IsNode() && mNode_Derived == morkDerived_kAtomSpace; }


public: 
  void NonAtomSpaceTypeError(morkEnv* ev);

public: 

  mork_bool MarkAllAtomSpaceContentDirty(morkEnv* ev);
  
  
  
  
  
  
  
  

public: 

  
  
  
  
  

  mork_num CutAllAtoms(morkEnv* ev, morkPool* ioPool);
  
  
  morkBookAtom* MakeBookAtomCopyWithAid(morkEnv* ev,
     const morkFarBookAtom& inAtom,  mork_aid inAid);
  
  
  morkBookAtom* MakeBookAtomCopy(morkEnv* ev, const morkFarBookAtom& inAtom);
  

  mork_aid MakeNewAtomId(morkEnv* ev, morkBookAtom* ioAtom);
  

public: 
  static void SlotWeakAtomSpace(morkAtomSpace* me,
    morkEnv* ev, morkAtomSpace** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongAtomSpace(morkAtomSpace* me,
    morkEnv* ev, morkAtomSpace** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#define morkDerived_kAtomSpaceMap 0x615A /* ascii 'aZ' */



class morkAtomSpaceMap : public morkNodeMap { 

public:

  virtual ~morkAtomSpaceMap();
  morkAtomSpaceMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap);

public: 

  mork_bool  AddAtomSpace(morkEnv* ev, morkAtomSpace* ioAtomSpace)
  { return this->AddNode(ev, ioAtomSpace->SpaceScope(), ioAtomSpace); }
  

  mork_bool  CutAtomSpace(morkEnv* ev, mork_scope inScope)
  { return this->CutNode(ev, inScope); }
  
  
  morkAtomSpace*  GetAtomSpace(morkEnv* ev, mork_scope inScope)
  { return (morkAtomSpace*) this->GetNode(ev, inScope); }
  

  mork_num CutAllAtomSpaces(morkEnv* ev)
  { return this->CutAllNodes(ev); }
  
};

class morkAtomSpaceMapIter: public morkMapIter{ 

public:
  morkAtomSpaceMapIter(morkEnv* ev, morkAtomSpaceMap* ioMap)
  : morkMapIter(ev, ioMap) { }
 
  morkAtomSpaceMapIter( ) : morkMapIter()  { }
  void InitAtomSpaceMapIter(morkEnv* ev, morkAtomSpaceMap* ioMap)
  { this->InitMapIter(ev, ioMap); }
   
  mork_change*
  FirstAtomSpace(morkEnv* ev, mork_scope* outScope, morkAtomSpace** outAtomSpace)
  { return this->First(ev, outScope, outAtomSpace); }
  
  mork_change*
  NextAtomSpace(morkEnv* ev, mork_scope* outScope, morkAtomSpace** outAtomSpace)
  { return this->Next(ev, outScope, outAtomSpace); }
  
  mork_change*
  HereAtomSpace(morkEnv* ev, mork_scope* outScope, morkAtomSpace** outAtomSpace)
  { return this->Here(ev, outScope, outAtomSpace); }
  
  mork_change*
  CutHereAtomSpace(morkEnv* ev, mork_scope* outScope, morkAtomSpace** outAtomSpace)
  { return this->CutHere(ev, outScope, outAtomSpace); }
};



#endif 

