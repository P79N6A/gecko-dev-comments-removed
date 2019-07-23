





































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

#ifndef _MORKTABLEROWCURSOR_
#include "morkTableRowCursor.h"
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

#ifndef _MORKROW_
#include "morkRow.h"
#endif






 void
morkTableRowCursor::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseTableRowCursor(ev);
    this->MarkShut();
  }
}


morkTableRowCursor::~morkTableRowCursor() 
{
  CloseMorkNode(mMorkEnv);
  MORK_ASSERT(this->IsShutNode());
}


morkTableRowCursor::morkTableRowCursor(morkEnv* ev,
  const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, morkTable* ioTable, mork_pos inRowPos)
: morkCursor(ev, inUsage, ioHeap)
, mTableRowCursor_Table( 0 )
{
  if ( ev->Good() )
  {
    if ( ioTable )
    {
      mCursor_Pos = inRowPos;
      mCursor_Seed = ioTable->TableSeed();
      morkTable::SlotWeakTable(ioTable, ev, &mTableRowCursor_Table);
      if ( ev->Good() )
        mNode_Derived = morkDerived_kTableRowCursor;
    }
    else
      ev->NilPointerError();
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(morkTableRowCursor, morkCursor, nsIMdbTableRowCursor)
 void
morkTableRowCursor::CloseTableRowCursor(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      mCursor_Pos = -1;
      mCursor_Seed = 0;
      morkTable::SlotWeakTable((morkTable*) 0, ev, &mTableRowCursor_Table);
      this->CloseCursor(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 mdb_err
morkTableRowCursor::GetCount(nsIMdbEnv* mev, mdb_count* outCount)
{
  mdb_err outErr = 0;
  mdb_count count = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    count = GetMemberCount(ev);
    outErr = ev->AsErr();
  }
  if ( outCount )
    *outCount = count;
  return outErr;
}

 mdb_err
morkTableRowCursor::GetSeed(nsIMdbEnv* mev, mdb_seed* outSeed)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

 mdb_err
morkTableRowCursor::SetPos(nsIMdbEnv* mev, mdb_pos inPos)
{
  mCursor_Pos = inPos;
  return NS_OK;
}

 mdb_err
morkTableRowCursor::GetPos(nsIMdbEnv* mev, mdb_pos* outPos)
{
  *outPos = mCursor_Pos;
  return NS_OK;
}

 mdb_err
morkTableRowCursor::SetDoFailOnSeedOutOfSync(nsIMdbEnv* mev, mdb_bool inFail)
{
  mCursor_DoFailOnSeedOutOfSync = inFail;
  return NS_OK;
}

 mdb_err
morkTableRowCursor::GetDoFailOnSeedOutOfSync(nsIMdbEnv* mev, mdb_bool* outFail)
{
  NS_ENSURE_ARG_POINTER(outFail);
  *outFail = mCursor_DoFailOnSeedOutOfSync;
  return NS_OK;
}







NS_IMETHODIMP
morkTableRowCursor::GetTable(nsIMdbEnv* mev, nsIMdbTable** acqTable)
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( mTableRowCursor_Table )
      outTable = mTableRowCursor_Table->AcquireTableHandle(ev);
    
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}



NS_IMETHODIMP
morkTableRowCursor::NextRowOid( 
  nsIMdbEnv* mev, 
  mdbOid* outOid, 
  mdb_pos* outRowPos)
{
  mdb_err outErr = 0;
  mork_pos pos = -1;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( outOid )
    {
      pos = NextRowOid(ev, outOid);
    }
    else
      ev->NilPointerError();
    outErr = ev->AsErr();
  }
  if ( outRowPos )
    *outRowPos = pos;
  return outErr;
}

NS_IMETHODIMP
morkTableRowCursor::PrevRowOid( 
  nsIMdbEnv* mev, 
  mdbOid* outOid, 
  mdb_pos* outRowPos)
{
  mdb_err outErr = 0;
  mork_pos pos = -1;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( outOid )
    {
      pos = PrevRowOid(ev, outOid);
    }
    else
      ev->NilPointerError();
    outErr = ev->AsErr();
  }
  if ( outRowPos )
    *outRowPos = pos;
  return outErr;
}



NS_IMETHODIMP
morkTableRowCursor::NextRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow** acqRow, 
  mdb_pos* outRowPos)
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
      
    mdbOid oid; 
    morkRow* row = NextRow(ev, &oid, outRowPos);
    if ( row )
    {
      morkStore* store = row->GetRowSpaceStore(ev);
      if ( store )
        outRow = row->AcquireRowHandle(ev, store);
    }
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}

NS_IMETHODIMP
morkTableRowCursor::PrevRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow** acqRow, 
  mdb_pos* outRowPos)
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
      
    mdbOid oid; 
    morkRow* row = PrevRow(ev, &oid, outRowPos);
    if ( row )
    {
      morkStore* store = row->GetRowSpaceStore(ev);
      if ( store )
        outRow = row->AcquireRowHandle(ev, store);
    }
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}





NS_IMETHODIMP
morkTableRowCursor::CanHaveDupRowMembers(nsIMdbEnv* mev, 
  mdb_bool* outCanHaveDups)
{
  mdb_err outErr = 0;
  mdb_bool canHaveDups = mdbBool_kFalse;
  
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    canHaveDups = CanHaveDupRowMembers(ev);
    outErr = ev->AsErr();
  }
  if ( outCanHaveDups )
    *outCanHaveDups = canHaveDups;
  return outErr;
}
  
NS_IMETHODIMP
morkTableRowCursor::MakeUniqueCursor( 
  nsIMdbEnv* mev, 
  nsIMdbTableRowCursor** acqCursor)    
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  nsIMdbTableRowCursor* outCursor = 0;
  
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    AddRef();
    outCursor = this;
      
    outErr = ev->AsErr();
  }
  if ( acqCursor )
    *acqCursor = outCursor;
  return outErr;
}





 void
morkTableRowCursor::NonTableRowCursorTypeError(morkEnv* ev)
{
  ev->NewError("non morkTableRowCursor");
}


mdb_pos
morkTableRowCursor::NextRowOid(morkEnv* ev, mdbOid* outOid)
{
  mdb_pos outPos = -1;
  (void) this->NextRow(ev, outOid, &outPos);
  return outPos;
}

mdb_pos
morkTableRowCursor::PrevRowOid(morkEnv* ev, mdbOid* outOid)
{
  mdb_pos outPos = -1;
  (void) this->PrevRow(ev, outOid, &outPos);
  return outPos;
}

mork_bool
morkTableRowCursor::CanHaveDupRowMembers(morkEnv* ev)
{
  return morkBool_kFalse; 
}

mork_count
morkTableRowCursor::GetMemberCount(morkEnv* ev)
{
  morkTable* table = mTableRowCursor_Table;
  if ( table )
    return table->mTable_RowArray.mArray_Fill;
  else
    return 0;
}

morkRow*
morkTableRowCursor::PrevRow(morkEnv* ev, mdbOid* outOid, mdb_pos* outPos)
{
  morkRow* outRow = 0;
  mork_pos pos = -1;
  
  morkTable* table = mTableRowCursor_Table;
  if ( table )
  {
    if ( table->IsOpenNode() )
    {
      morkArray* array = &table->mTable_RowArray;
      pos = mCursor_Pos - 1;
        
      if ( pos >= 0 && pos < (mork_pos)(array->mArray_Fill) )
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
      table->NonOpenNodeError(ev);
  }
  else
    ev->NilPointerError();

  *outPos = pos;
  return outRow;
}

morkRow*
morkTableRowCursor::NextRow(morkEnv* ev, mdbOid* outOid, mdb_pos* outPos)
{
  morkRow* outRow = 0;
  mork_pos pos = -1;
  
  morkTable* table = mTableRowCursor_Table;
  if ( table )
  {
    if ( table->IsOpenNode() )
    {
      morkArray* array = &table->mTable_RowArray;
      pos = mCursor_Pos;
      if ( pos < 0 )
        pos = 0;
      else
        ++pos;
        
      if ( pos < (mork_pos)(array->mArray_Fill) )
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
      table->NonOpenNodeError(ev);
  }
  else
    ev->NilPointerError();

  *outPos = pos;
  return outRow;
}


