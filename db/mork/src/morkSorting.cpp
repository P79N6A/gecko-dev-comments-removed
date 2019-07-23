




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif

#ifndef _MORKARRAY_
#include "morkArray.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _ORKINSORTING_
#include "orkinSorting.h"
#endif

#ifndef _MORKTABLEROWCURSOR_
#include "morkTableRowCursor.h"
#endif

#ifndef _MORKSORTING_
#include "morkSorting.h"
#endif

#ifndef _MORKQUICKSORT_
#include "morkQuickSort.h"
#endif






 void
morkSorting::CloseMorkNode(morkEnv* ev)  
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseSorting(ev);
    this->MarkShut();
  }
}


morkSorting::~morkSorting()  
{
  MORK_ASSERT(this->IsShutNode());
  MORK_ASSERT(mSorting_Table==0);
}

#define morkSorting_kExtraSlots 2 /* space for two more rows */


morkSorting::morkSorting(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioNodeHeap, morkTable* ioTable,
    nsIMdbCompare* ioCompare,
    nsIMdbHeap* ioSlotHeap, mork_column inCol)
: morkObject(ev, inUsage, ioNodeHeap, morkColor_kNone, (morkHandle*) 0)
, mSorting_Table( 0 )

, mSorting_Compare( 0 )

, mSorting_RowArray(ev, morkUsage::kMember, (nsIMdbHeap*) 0,
  ioTable->GetRowCount() + morkSorting_kExtraSlots, ioSlotHeap)
  
, mSorting_Col( inCol )
{  
  if ( ev->Good() )
  {
    if ( ioTable && ioSlotHeap && ioCompare )
    {
      if ( inCol )
      {
        nsIMdbCompare_SlotStrongCompare(ioCompare, ev, &mSorting_Compare);
        morkTable::SlotWeakTable(ioTable, ev, &mSorting_Table);
        if ( ev->Good() )
        {
          mNode_Derived = morkDerived_kSorting;
        }
      }
      else
        this->ZeroColError(ev);
    }
    else
      ev->NilPointerError();
  }
}

 void
morkSorting::CloseSorting(morkEnv* ev)  
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      nsIMdbCompare_SlotStrongCompare((nsIMdbCompare*) 0, ev,
        &mSorting_Compare);
      morkTable::SlotWeakTable((morkTable*) 0, ev, &mSorting_Table);
      mSorting_RowArray.CloseMorkNode(ev);
      mSorting_Col = 0;
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void
morkSorting::NonSortingTypeError(morkEnv* ev)
{
  ev->NewError("non morkSorting");
}

 void
morkSorting::NonSortingTypeWarning(morkEnv* ev)
{
  ev->NewWarning("non morkSorting");
}

 void
morkSorting::NilTableError(morkEnv* ev)
{
  ev->NewError("nil mSorting_Table");
}

 void
morkSorting::NilCompareError(morkEnv* ev)
{
  ev->NewError("nil mSorting_Compare");
}

 void
morkSorting::ZeroColError(morkEnv* ev)
{
  ev->NewError("zero mSorting_Col");
}

nsIMdbSorting*
morkSorting::AcquireSortingHandle(morkEnv* ev)
{
  nsIMdbSorting* outSorting = 0;
  orkinSorting* s = (orkinSorting*) mObject_Handle;
  if ( s ) 
    s->AddStrongRef(ev->AsMdbEnv());
  else 
  {
    s = orkinSorting::MakeSorting(ev, this);
    mObject_Handle = s;
  }
  if ( s )
    outSorting = s;
  return outSorting;
}


class morkSortClosure {
public:

  morkEnv*     mSortClosure_Env;
  morkSorting* mSortClosure_Sorting;
  
public:
  morkSortClosure(morkEnv* ev, morkSorting* ioSorting);
};

morkSortClosure::morkSortClosure(morkEnv* ev, morkSorting* ioSorting)
  : mSortClosure_Env(ev), mSortClosure_Sorting(ioSorting)
{
}

static mdb_order morkRow_Order(const morkRow* inA, const morkRow* inB, 
  morkSortClosure* ioClosure)
{
  return 0;  
}

void morkSorting::sort_rows(morkEnv* ev)
{
  morkTable* table = mSorting_Table;
  if ( table )
  {
    morkArray* tra = &table->mTable_RowArray;
    mork_count count = mSorting_RowArray.mArray_Fill;
    if ( this->is_seed_stale() || count != tra->mArray_Fill )
      count = this->copy_table_row_array(ev);
    
    if ( ev->Good() )
    {
      void** slots = mSorting_RowArray.mArray_Slots;
      morkSortClosure closure(ev, this);
      
      morkQuickSort((mork_u1*) slots, count, sizeof(morkRow*), 
        (mdbAny_Order) morkRow_Order, &closure);
    }
  }
  else
    this->NilTableError(ev);
}

mork_count morkSorting::copy_table_row_array(morkEnv* ev)
{
  morkArray* tra = &mSorting_Table->mTable_RowArray;
  mork_bool bigEnough = mSorting_RowArray.mArray_Size > tra->mArray_Fill;
  if ( !bigEnough )
    bigEnough = mSorting_RowArray.Grow(ev, tra->mArray_Fill);
    
  if ( ev->Good() && bigEnough )
  {
    mSorting_RowArray.mArray_Fill = tra->mArray_Fill;
    morkRow** src = (morkRow**) tra->mArray_Slots;
    morkRow** dst = (morkRow**) mSorting_RowArray.mArray_Slots;
    morkRow** end = dst + tra->mArray_Fill;
    
    while ( dst < end )
      *dst++ = *src++;

    this->sync_with_table_seed();
  }
    
  return mSorting_RowArray.mArray_Fill;
}

mork_pos
morkSorting::ArrayHasOid(morkEnv* ev, const mdbOid* inOid)
{
  MORK_USED_1(ev); 
  mork_count count = mSorting_RowArray.mArray_Fill;
  mork_pos pos = -1;
  while ( ++pos < (mork_pos)count )
  {
    morkRow* row = (morkRow*) mSorting_RowArray.At(pos);
    MORK_ASSERT(row);
    if ( row && row->EqualOid(inOid) )
    {
      return pos;
    }
  }
  return -1;
}

mork_bool
morkSorting::AddRow(morkEnv* ev, morkRow* ioRow)
{
  MORK_USED_1(ioRow);
  return ev->Good();
}

mork_bool
morkSorting::CutRow(morkEnv* ev, morkRow* ioRow)
{
  MORK_USED_1(ioRow);
  return ev->Good();
}


mork_bool
morkSorting::CutAllRows(morkEnv* ev)
{
  return ev->Good();
}

morkSortingRowCursor*
morkSorting::NewSortingRowCursor(morkEnv* ev, mork_pos inRowPos)
{
  morkSortingRowCursor* outCursor = 0;
  if ( ev->Good() )
  {


      
    
    
    
    
    
    
    
    
    
    
  }
  return outCursor;
}





morkSortingMap::~morkSortingMap()
{
}

morkSortingMap::morkSortingMap(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
  : morkNodeMap(ev, inUsage, ioHeap, ioSlotHeap)
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kSortingMap;
}



