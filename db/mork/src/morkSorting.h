




































#ifndef _MORKSORTING_
#define _MORKSORTING_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKDEQUE_
#include "morkDeque.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif

#ifndef _MORKARRAY_
#include "morkArray.h"
#endif

#ifndef _MORKROWMAP_
#include "morkRowMap.h"
#endif

#ifndef _MORKNODEMAP_
#include "morkNodeMap.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif



class nsIMdbSorting;
#define morkDerived_kSorting 0x536F /* ascii 'So' */

class morkSorting : public morkObject { 

  


  

  
  
  
  
  
  
  
  
  
  

  
  

public: 

  morkTable*        mSorting_Table;    
  
  nsIMdbCompare*    mSorting_Compare;

  morkArray         mSorting_RowArray;  
  
  mork_column       mSorting_Col;       

public: 

  void SetSortingDirty() { this->SetNodeDirty(); }
   
  mork_bool IsSortingClean() const { return this->IsNodeClean(); }
  mork_bool IsSortingDirty() const { return this->IsNodeDirty(); }

public: 
  void* operator new(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev) CPP_THROW_NEW
  { return morkNode::MakeNew(inSize, ioHeap, ev); }
  
 

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkSorting(); 
  
public: 
  morkSorting(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioNodeHeap, morkTable* ioTable,
    nsIMdbCompare* ioCompare,
    nsIMdbHeap* ioSlotHeap, mork_column inCol);
  void CloseSorting(morkEnv* ev); 

private: 
  morkSorting(const morkSorting& other);
  morkSorting& operator=(const morkSorting& other);

public: 
  mork_bool IsSorting() const
  { return IsNode() && mNode_Derived == morkDerived_kSorting; }


public: 
  static void NonSortingTypeError(morkEnv* ev);
  static void NonSortingTypeWarning(morkEnv* ev);
  static void ZeroColError(morkEnv* ev);
  static void NilTableError(morkEnv* ev);
  static void NilCompareError(morkEnv* ev);

public: 

  void sort_rows(morkEnv* ev);
  mork_count copy_table_row_array(morkEnv* ev);

public: 
   
  mork_seed SortingSeed() const { return mSorting_RowArray.mArray_Seed; }
  
  morkRow* SafeRowAt(morkEnv* ev, mork_pos inPos)
  { return (morkRow*) mSorting_RowArray.SafeAt(ev, inPos); }

  nsIMdbSorting* AcquireSortingHandle(morkEnv* ev); 
  
  mork_count GetRowCount() const { return mSorting_RowArray.mArray_Fill; }
  mork_pos  ArrayHasOid(morkEnv* ev, const mdbOid* inOid);

  morkSortingRowCursor* NewSortingRowCursor(morkEnv* ev, mork_pos inRowPos);

  mork_bool AddRow(morkEnv* ev, morkRow* ioRow);

  mork_bool CutRow(morkEnv* ev, morkRow* ioRow);

  mork_bool CutAllRows(morkEnv* ev);

protected: 
   
  mork_bool is_seed_stale() const
  { return mSorting_RowArray.mArray_Seed != mSorting_Table->TableSeed(); }
   
  void sync_with_table_seed()
  { mSorting_RowArray.mArray_Seed = mSorting_Table->TableSeed(); }

public: 
  static void SlotWeakSorting(morkSorting* me,
    morkEnv* ev, morkSorting** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongSorting(morkSorting* me,
    morkEnv* ev, morkSorting** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#define morkDerived_kSortingMap 0x734D /* ascii 'sM' */



class morkSortingMap : public morkNodeMap { 

public:

  virtual ~morkSortingMap();
  morkSortingMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap);

public: 

  mork_bool  AddSorting(morkEnv* ev, morkSorting* ioSorting)
  { return this->AddNode(ev, ioSorting->mSorting_Col, ioSorting); }
  

  mork_bool  CutSorting(morkEnv* ev, mork_column inCol)
  { return this->CutNode(ev, inCol); }
  
  
  morkSorting*  GetSorting(morkEnv* ev, mork_column inCol)
  { return (morkSorting*) this->GetNode(ev, inCol); }
  

  mork_num CutAllSortings(morkEnv* ev)
  { return this->CutAllNodes(ev); }
  
};

class morkSortingMapIter: public morkMapIter{ 

public:
  morkSortingMapIter(morkEnv* ev, morkSortingMap* ioMap)
  : morkMapIter(ev, ioMap) { }
 
  morkSortingMapIter( ) : morkMapIter()  { }
  void InitSortingMapIter(morkEnv* ev, morkSortingMap* ioMap)
  { this->InitMapIter(ev, ioMap); }
   
  mork_change*
  FirstSorting(morkEnv* ev, mork_column* outCol, morkSorting** outSorting)
  { return this->First(ev, outCol, outSorting); }
  
  mork_change*
  NextSorting(morkEnv* ev, mork_column* outCol, morkSorting** outSorting)
  { return this->Next(ev, outCol, outSorting); }
  
  mork_change*
  HereSorting(morkEnv* ev, mork_column* outCol, morkSorting** outSorting)
  { return this->Here(ev, outCol, outSorting); }
  
  
  mork_change*
  CutHereSorting(morkEnv* ev, mork_column* outCol, morkSorting** outSorting)
  { return this->CutHere(ev, outCol, outSorting); }
};




#endif 
