




































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

#ifndef _MORKSEARCHROWCURSOR_
#include "morkSearchRowCursor.h"
#endif

#ifndef _MORKUNIQROWCURSOR_
#include "morkUniqRowCursor.h"
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
morkSearchRowCursor::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseSearchRowCursor(ev);
    this->MarkShut();
  }
}


morkSearchRowCursor::~morkSearchRowCursor() 
{
  MORK_ASSERT(this->IsShutNode());
}


morkSearchRowCursor::morkSearchRowCursor(morkEnv* ev,
  const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, morkTable* ioTable, mork_pos inRowPos)
: morkTableRowCursor(ev, inUsage, ioHeap, ioTable, inRowPos)

{
  if ( ev->Good() )
  {
    if ( ioTable )
    {
      
      if ( ev->Good() )
      {
        
        
      }
    }
    else
      ev->NilPointerError();
  }
}

 void
morkSearchRowCursor::CloseSearchRowCursor(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      
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
morkSearchRowCursor::NonSearchRowCursorTypeError(morkEnv* ev)
{
  ev->NewError("non morkSearchRowCursor");
}

morkUniqRowCursor*
morkSearchRowCursor::MakeUniqCursor(morkEnv* ev)
{
  morkUniqRowCursor* outCursor = 0;
  
  return outCursor;
}

#if 0
orkinTableRowCursor*
morkSearchRowCursor::AcquireUniqueRowCursorHandle(morkEnv* ev)
{
  orkinTableRowCursor* outCursor = 0;
  
  morkUniqRowCursor* uniqCursor = this->MakeUniqCursor(ev);
  if ( uniqCursor )
  {
    outCursor = uniqCursor->AcquireTableRowCursorHandle(ev);
    uniqCursor->CutStrongRef(ev);
  }
  return outCursor;
}
#endif
mork_bool
morkSearchRowCursor::CanHaveDupRowMembers(morkEnv* ev)
{
  return morkBool_kTrue; 
}

mork_count
morkSearchRowCursor::GetMemberCount(morkEnv* ev)
{
  morkTable* table = mTableRowCursor_Table;
  if ( table )
    return table->mTable_RowArray.mArray_Fill;
  else
    return 0;
}

morkRow*
morkSearchRowCursor::NextRow(morkEnv* ev, mdbOid* outOid, mdb_pos* outPos)
{
  morkRow* outRow = 0;
  mork_pos pos = -1;
  
  morkTable* table = mTableRowCursor_Table;
  if ( table )
  {
  }
  else
    ev->NilPointerError();

  *outPos = pos;
  return outRow;
}


