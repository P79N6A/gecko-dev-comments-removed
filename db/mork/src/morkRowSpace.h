




































#ifndef _MORKROWSPACE_
#define _MORKROWSPACE_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKSPACE_
#include "morkSpace.h"
#endif

#ifndef _MORKNODEMAP_
#include "morkNodeMap.h"
#endif

#ifndef _MORKROWMAP_
#include "morkRowMap.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKARRAY_
#include "morkArray.h"
#endif

#ifndef _MORKDEQUE_
#include "morkDeque.h"
#endif



#define morkDerived_kRowSpace 0x7253 /* ascii 'rS' */

#define morkRowSpace_kStartRowMapSlotCount 11

#define morkRowSpace_kMaxIndexCount 8 /* no more indexes than this */
#define morkRowSpace_kPrimeCacheSize 17 /* should be prime number */

class morkAtomRowMap;



class morkRowSpace : public morkSpace { 


  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

public: 

  nsIMdbHeap*  mRowSpace_SlotHeap;

#ifdef MORK_ENABLE_PROBE_MAPS
  morkRowProbeMap   mRowSpace_Rows;   
#else 
  morkRowMap   mRowSpace_Rows;   
#endif 
  morkTableMap mRowSpace_Tables; 

  mork_tid     mRowSpace_NextTableId;  
  mork_rid     mRowSpace_NextRowId;    
  
  mork_count   mRowSpace_IndexCount; 
    
  
  morkAtomRowMap* mRowSpace_IndexCache[ morkRowSpace_kPrimeCacheSize ];

  morkDeque    mRowSpace_TablesByPriority[ morkPriority_kCount ];

public: 
  void SetRowSpaceDirty() { this->SetNodeDirty(); }
  void SetRowSpaceClean() { this->SetNodeClean(); }
  
  mork_bool IsRowSpaceClean() const { return this->IsNodeClean(); }
  mork_bool IsRowSpaceDirty() const { return this->IsNodeDirty(); }


public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkRowSpace(); 
  
public: 
  morkRowSpace(morkEnv* ev, const morkUsage& inUsage, mork_scope inScope,
    morkStore* ioStore, nsIMdbHeap* ioNodeHeap, nsIMdbHeap* ioSlotHeap);
  void CloseRowSpace(morkEnv* ev); 

public: 
  mork_bool IsRowSpace() const
  { return IsNode() && mNode_Derived == morkDerived_kRowSpace; }


public: 
  static void NonRowSpaceTypeError(morkEnv* ev);
  static void ZeroScopeError(morkEnv* ev);
  static void ZeroKindError(morkEnv* ev);
  static void ZeroTidError(morkEnv* ev);
  static void MinusOneRidError(morkEnv* ev);

  
  

public: 

  mork_num CutAllRows(morkEnv* ev, morkPool* ioPool);
  
  
  morkTable* NewTable(morkEnv* ev, mork_kind inTableKind,
    mdb_bool inMustBeUnique, const mdbOid* inOptionalMetaRowOid);
  
  morkTable* NewTableWithTid(morkEnv* ev, mork_tid inTid,
    mork_kind inTableKind, const mdbOid* inOptionalMetaRowOid);
  
  morkTable* FindTableByKind(morkEnv* ev, mork_kind inTableKind);
  morkTable* FindTableByTid(morkEnv* ev, mork_tid inTid)
  { return mRowSpace_Tables.GetTable(ev, inTid); }

  mork_tid MakeNewTableId(morkEnv* ev);
  mork_rid MakeNewRowId(morkEnv* ev);

  
  

  morkRow* NewRowWithOid(morkEnv* ev, const mdbOid* inOid);
  morkRow* NewRow(morkEnv* ev);

  morkRow* FindRow(morkEnv* ev, mork_column inColumn, const mdbYarn* inYarn);

  morkAtomRowMap* ForceMap(morkEnv* ev, mork_column inColumn);
  morkAtomRowMap* FindMap(morkEnv* ev, mork_column inColumn);

protected: 
  morkAtomRowMap* make_index(morkEnv* ev, mork_column inColumn);

public: 
  static void SlotWeakRowSpace(morkRowSpace* me,
    morkEnv* ev, morkRowSpace** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongRowSpace(morkRowSpace* me,
    morkEnv* ev, morkRowSpace** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#define morkDerived_kRowSpaceMap 0x725A /* ascii 'rZ' */



class morkRowSpaceMap : public morkNodeMap { 

public:

  virtual ~morkRowSpaceMap();
  morkRowSpaceMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap);

public: 

  mork_bool  AddRowSpace(morkEnv* ev, morkRowSpace* ioRowSpace)
  { return this->AddNode(ev, ioRowSpace->SpaceScope(), ioRowSpace); }
  

  mork_bool  CutRowSpace(morkEnv* ev, mork_scope inScope)
  { return this->CutNode(ev, inScope); }
  
  
  morkRowSpace*  GetRowSpace(morkEnv* ev, mork_scope inScope)
  { return (morkRowSpace*) this->GetNode(ev, inScope); }
  

  mork_num CutAllRowSpaces(morkEnv* ev)
  { return this->CutAllNodes(ev); }
  
};

class morkRowSpaceMapIter: public morkMapIter{ 

public:
  morkRowSpaceMapIter(morkEnv* ev, morkRowSpaceMap* ioMap)
  : morkMapIter(ev, ioMap) { }
 
  morkRowSpaceMapIter( ) : morkMapIter()  { }
  void InitRowSpaceMapIter(morkEnv* ev, morkRowSpaceMap* ioMap)
  { this->InitMapIter(ev, ioMap); }
   
  mork_change*
  FirstRowSpace(morkEnv* ev, mork_scope* outScope, morkRowSpace** outRowSpace)
  { return this->First(ev, outScope, outRowSpace); }
  
  mork_change*
  NextRowSpace(morkEnv* ev, mork_scope* outScope, morkRowSpace** outRowSpace)
  { return this->Next(ev, outScope, outRowSpace); }
  
  mork_change*
  HereRowSpace(morkEnv* ev, mork_scope* outScope, morkRowSpace** outRowSpace)
  { return this->Here(ev, outScope, outRowSpace); }
  
  mork_change*
  CutHereRowSpace(morkEnv* ev, mork_scope* outScope, morkRowSpace** outRowSpace)
  { return this->CutHere(ev, outScope, outRowSpace); }
};



#endif 
