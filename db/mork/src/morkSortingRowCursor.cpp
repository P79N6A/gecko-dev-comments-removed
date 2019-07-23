




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKCURSOR_
#include "morkCursor.h"
#endif

#ifndef _MORKSORTINGROWCURSOR_
#include "morkSortingRowCursor.h"
#endif

#ifndef _ORKINTABLEROWCURSOR_
#include "orkinTableRowCursor.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKSORTING_
#include "morkSorting.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif






 void
morkSortingRowCursor::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseSortingRowCursor(ev);
    this->MarkShut();
  }
}


morkSortingRowCursor::~morkSortingRowCursor() 
{
  MORK_ASSERT(this->IsShutNode());
}


morkSortingRowCursor::morkSortingRowCursor(morkEnv* ev,
  const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, morkTable* ioTable, mork_pos inRowPos,
  morkSorting* ioSorting)
: morkTableRowCursor(ev, inUsage, ioHeap, ioTable, inRowPos)
, mSortingRowCursor_Sorting( 0 )
{
  if ( ev->Good() )
  {
    if ( ioSorting )
    {
      morkSorting::SlotWeakSorting(ioSorting, ev, &mSortingRowCursor_Sorting);
      if ( ev->Good() )
      {
        
        
      }
    }
    else
      ev->NilPointerError();
  }
}

 void
morkSortingRowCursor::CloseSortingRowCursor(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      mCursor_Pos = -1;
      mCursor_Seed = 0;
      morkSorting::SlotWeakSorting((morkSorting*) 0, ev, &mSortingRowCursor_Sorting);
      this->CloseTableRowCursor(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void
morkSortingRowCursor::NonSortingRowCursorTypeError(morkEnv* ev)
{
  ev->NewError("non morkSortingRowCursor");
}

orkinTableRowCursor*
morkSortingRowCursor::AcquireUniqueRowCursorHandle(morkEnv* ev)
{
  return this->AcquireTableRowCursorHandle(ev);
}

mork_bool
morkSortingRowCursor::CanHaveDupRowMembers(morkEnv* ev)
{
  return morkBool_kFalse; 
}

mork_count
morkSortingRowCursor::GetMemberCount(morkEnv* ev)
{
  morkTable* table = mTableRowCursor_Table;
  if ( table )
    return table->mTable_RowArray.mArray_Fill;
  else
    return 0;

  
  
  
  
  
}

morkRow*
morkSortingRowCursor::NextRow(morkEnv* ev, mdbOid* outOid, mdb_pos* outPos)
{
  morkRow* outRow = 0;
  mork_pos pos = -1;
  
  morkSorting* sorting = mSortingRowCursor_Sorting;
  if ( sorting )
  {
    if ( sorting->IsOpenNode() )
    {
      morkArray* array = &sorting->mSorting_RowArray;
      pos = mCursor_Pos;
      if ( pos < 0 )
        pos = 0;
      else
        ++pos;
        
      if ( pos < (mork_pos)(array->mArray_Fill))
      {
        mCursor_Pos = pos; 
        morkRow* row = (morkRow*) array->At(pos);
        if ( row )
        {
          if ( row->IsRow() )
          {
            outRow = row;
            *outOid = row->mRow_Oid;
          }
          else
            row->NonRowTypeError(ev);
        }
        else
          ev->NilPointerError();
      }
      else
      {
        outOid->mOid_Scope = 0;
        outOid->mOid_Id = morkId_kMinusOne;
      }
    }
    else
      sorting->NonOpenNodeError(ev);
  }
  else
    ev->NilPointerError();

  *outPos = pos;
  return outRow;
}


