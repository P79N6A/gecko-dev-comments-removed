




































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

#ifndef _MORKROWCELLCURSOR_
#include "morkRowCellCursor.h"
#endif

#ifndef _ORKINROWCELLCURSOR_
#include "orkinRowCellCursor.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKROWOBJECT_
#include "morkRowObject.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif






 void
morkRowCellCursor::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseRowCellCursor(ev);
    this->MarkShut();
  }
}


morkRowCellCursor::~morkRowCellCursor() 
{
  CloseMorkNode(mMorkEnv);
  MORK_ASSERT(this->IsShutNode());
}


morkRowCellCursor::morkRowCellCursor(morkEnv* ev,
  const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, morkRowObject* ioRowObject)
: morkCursor(ev, inUsage, ioHeap)
, mRowCellCursor_RowObject( 0 )
, mRowCellCursor_Col( 0 )
{
  if ( ev->Good() )
  {
    if ( ioRowObject )
    {
      morkRow* row = ioRowObject->mRowObject_Row;
      if ( row )
      {
        if ( row->IsRow() )
        {
          mCursor_Pos = -1;
          mCursor_Seed = row->mRow_Seed;
          
          morkRowObject::SlotStrongRowObject(ioRowObject, ev,
            &mRowCellCursor_RowObject);
          if ( ev->Good() )
            mNode_Derived = morkDerived_kRowCellCursor;
        }
        else
          row->NonRowTypeError(ev);
      }
      else
        ioRowObject->NilRowError(ev);
    }
    else
      ev->NilPointerError();
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(morkRowCellCursor, morkCursor, nsIMdbRowCellCursor)

 void
morkRowCellCursor::CloseRowCellCursor(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      mCursor_Pos = -1;
      mCursor_Seed = 0;
      morkRowObject::SlotStrongRowObject((morkRowObject*) 0, ev,
        &mRowCellCursor_RowObject);
      this->CloseCursor(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void
morkRowCellCursor::NilRowObjectError(morkEnv* ev)
{
  ev->NewError("nil mRowCellCursor_RowObject");
}

 void
morkRowCellCursor::NonRowCellCursorTypeError(morkEnv* ev)
{
  ev->NewError("non morkRowCellCursor");
}




NS_IMETHODIMP
morkRowCellCursor::SetRow(nsIMdbEnv* mev, nsIMdbRow* ioRow)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    row = (morkRow *) ioRow;
    morkStore* store = row->GetRowSpaceStore(ev);
    if ( store )
    {
      morkRowObject* rowObj = row->AcquireRowObject(ev, store);
      if ( rowObj )
      {
        morkRowObject::SlotStrongRowObject((morkRowObject*) 0, ev,
          &mRowCellCursor_RowObject);
          
        mRowCellCursor_RowObject = rowObj; 
        mCursor_Seed = row->mRow_Seed;
        
        row->GetCell(ev, mRowCellCursor_Col, &mCursor_Pos);
      }
    }
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkRowCellCursor::GetRow(nsIMdbEnv* mev, nsIMdbRow** acqRow)
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRowObject* rowObj = mRowCellCursor_RowObject;
    if ( rowObj )
      outRow = rowObj->AcquireRowHandle(ev);

    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}



NS_IMETHODIMP
morkRowCellCursor::MakeCell( 
  nsIMdbEnv* mev, 
  mdb_column* outColumn, 
  mdb_pos* outPos, 
  nsIMdbCell** acqCell)
{
  mdb_err outErr = 0;
  nsIMdbCell* outCell = 0;
  mdb_pos pos = 0;
  mdb_column col = 0;
  morkRow* row = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    pos = mCursor_Pos;
    morkCell* cell = row->CellAt(ev, pos);
    if ( cell )
    {
      col = cell->GetColumn();
      outCell = row->AcquireCellHandle(ev, cell, col, pos);
    }
    outErr = ev->AsErr();
  }
  if ( acqCell )
    *acqCell = outCell;
   if ( outPos )
     *outPos = pos;
   if ( outColumn )
     *outColumn = col;
     
  return outErr;
}



NS_IMETHODIMP
morkRowCellCursor::SeekCell( 
  nsIMdbEnv* mev, 
  mdb_pos inPos, 
  mdb_column* outColumn, 
  nsIMdbCell** acqCell)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
morkRowCellCursor::NextCell( 
  nsIMdbEnv* mev, 
  nsIMdbCell** acqCell, 
  mdb_column* outColumn, 
  mdb_pos* outPos)
{
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  mdb_column col = 0;
  mdb_pos pos = mRowCellCursor_Col;
  if ( pos < 0 )
    pos = 0;
  else
    ++pos;

  morkCell* cell = mRowCellCursor_RowObject->mRowObject_Row->CellAt(ev, pos);
  if ( cell )
  {
    col = cell->GetColumn();
    *acqCell = mRowCellCursor_RowObject->mRowObject_Row->AcquireCellHandle(ev, cell, col, pos);
  }
  else
  {
    *acqCell = nsnull;
    pos = -1;
  }
 if ( outPos )
   *outPos = pos;
 if ( outColumn )
   *outColumn = col;
     
  mRowCellCursor_Col = pos;
  *outPos = pos;
  return NS_OK;
}
  
NS_IMETHODIMP
morkRowCellCursor::PickNextCell( 
  nsIMdbEnv* mev, 
  nsIMdbCell* ioCell, 
  const mdbColumnSet* inFilterSet, 
  mdb_column* outColumn, 
  mdb_pos* outPos)



{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}





