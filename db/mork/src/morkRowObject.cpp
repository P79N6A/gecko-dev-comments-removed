




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKROWOBJECT_
#include "morkRowObject.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKROWCELLCURSOR_
#include "morkRowCellCursor.h"
#endif

#ifndef _MORKCELLOBJECT_
#include "morkCellObject.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif






 void
morkRowObject::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseRowObject(ev);
    this->MarkShut();
  }
}


morkRowObject::~morkRowObject() 
{
  CloseMorkNode(mMorkEnv);
  MORK_ASSERT(this->IsShutNode());
}


morkRowObject::morkRowObject(morkEnv* ev,
  const morkUsage& inUsage, nsIMdbHeap* ioHeap,
     morkRow* ioRow, morkStore* ioStore)
: morkObject(ev, inUsage, ioHeap, morkColor_kNone, (morkHandle*) 0)
, mRowObject_Row( 0 )
, mRowObject_Store( 0 )
{
  if ( ev->Good() )
  {
    if ( ioRow && ioStore )
    {
      mRowObject_Row = ioRow;
      mRowObject_Store = ioStore; 
      
      if ( ev->Good() )
        mNode_Derived = morkDerived_kRowObject;
    }
    else
      ev->NilPointerError();
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(morkRowObject, morkObject, nsIMdbRow)



NS_IMETHODIMP
morkRowObject::GetSeed(nsIMdbEnv* mev,
  mdb_seed* outSeed)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    *outSeed = (mdb_seed) mRowObject_Row->mRow_Seed;
    outErr = ev->AsErr();
  }
  return outErr;
}
NS_IMETHODIMP
morkRowObject::GetCount(nsIMdbEnv* mev,
  mdb_count* outCount)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    *outCount = (mdb_count) mRowObject_Row->mRow_Length;
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkRowObject::GetPort(nsIMdbEnv* mev,
  nsIMdbPort** acqPort)
{
  mdb_err outErr = 0;
  nsIMdbPort* outPort = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRowSpace* rowSpace = mRowObject_Row->mRow_Space;
    if ( rowSpace && rowSpace->mSpace_Store )
    {
      morkStore* store = mRowObject_Row->GetRowSpaceStore(ev);
      if ( store )
        outPort = store->AcquireStoreHandle(ev);
    }
    else
      ev->NilPointerError();
      
    outErr = ev->AsErr();
  }
  if ( acqPort )
    *acqPort = outPort;
    
  return outErr;
}



NS_IMETHODIMP
morkRowObject::GetCursor( 
  nsIMdbEnv* mev, 
  mdb_pos inMemberPos, 
  nsIMdbCursor** acqCursor)
{
  return this->GetRowCellCursor(mev, inMemberPos,
    (nsIMdbRowCellCursor**) acqCursor);
}



NS_IMETHODIMP
morkRowObject::GetOid(nsIMdbEnv* mev,
  mdbOid* outOid)
{
  *outOid = mRowObject_Row->mRow_Oid;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  return (ev) ? ev->AsErr() : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
morkRowObject::BecomeContent(nsIMdbEnv* mev,
  const mdbOid* inOid)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
  
}



NS_IMETHODIMP
morkRowObject::DropActivity( 
  nsIMdbEnv* mev)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}







NS_IMETHODIMP
morkRowObject::GetRowCellCursor( 
  nsIMdbEnv* mev, 
  mdb_pos inPos, 
  nsIMdbRowCellCursor** acqCursor)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  nsIMdbRowCellCursor* outCursor = 0;
  if ( ev )
  {
    morkRowCellCursor* cursor = mRowObject_Row->NewRowCellCursor(ev, inPos);
    if ( cursor )
    {
      if ( ev->Good() )
      {
        cursor->mCursor_Seed = (mork_seed) inPos;
        outCursor = cursor;
        NS_ADDREF(cursor);
      }
    }
    outErr = ev->AsErr();
  }
  if ( acqCursor )
    *acqCursor = outCursor;
  return outErr;
}



NS_IMETHODIMP
morkRowObject::AddColumn( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  const mdbYarn* inYarn)
{
  mdb_err outErr = NS_ERROR_FAILURE;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( mRowObject_Store && mRowObject_Row)
      mRowObject_Row->AddColumn(ev, inColumn, inYarn, mRowObject_Store);
      
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkRowObject::CutColumn( 
  nsIMdbEnv* mev, 
  mdb_column inColumn)
{
  mdb_err outErr = NS_ERROR_FAILURE;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    mRowObject_Row->CutColumn(ev, inColumn);
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkRowObject::CutAllColumns( 
  nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    mRowObject_Row->CutAllColumns(ev);
    outErr = ev->AsErr();
  }
  return outErr;
}



NS_IMETHODIMP
morkRowObject::NewCell( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbCell** acqCell)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    GetCell(mev, inColumn, acqCell);
    if ( !*acqCell )
    {
      if ( mRowObject_Store )
      {
        mdbYarn yarn; 
        yarn.mYarn_Buf = 0;
        yarn.mYarn_Fill = 0;
        yarn.mYarn_Size = 0;
        yarn.mYarn_More = 0;
        yarn.mYarn_Form = 0;
        yarn.mYarn_Grow = 0;
        AddColumn(ev, inColumn, &yarn);
        GetCell(mev, inColumn, acqCell);
      }
    }
      
    outErr = ev->AsErr();
  }
  return outErr;
}
  
NS_IMETHODIMP
morkRowObject::AddCell( 
  nsIMdbEnv* mev, 
  const nsIMdbCell* inCell)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkCell* cell = 0;
    morkCellObject* cellObj = (morkCellObject*) inCell;
    if ( cellObj->CanUseCell(mev, morkBool_kFalse, &outErr, &cell) )
    {

      morkRow* cellRow = cellObj->mCellObject_Row;
      if ( cellRow )
      {
        if ( mRowObject_Row != cellRow )
        {
          morkStore* store = mRowObject_Row->GetRowSpaceStore(ev);
          morkStore* cellStore = cellRow->GetRowSpaceStore(ev);
          if ( store && cellStore )
          {
            mork_column col = cell->GetColumn();
            morkAtom* atom = cell->mCell_Atom;
            mdbYarn yarn;
            atom->AliasYarn(&yarn); 
            
            if ( store != cellStore )
              col = store->CopyToken(ev, col, cellStore);
            if ( ev->Good() )
              AddColumn(ev, col, &yarn);
          }
          else
            ev->NilPointerError();
        }
      }
      else
        ev->NilPointerError();
    }

    outErr = ev->AsErr();
  }
  return outErr;
}
  
NS_IMETHODIMP
morkRowObject::GetCell( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbCell** acqCell)
{
  mdb_err outErr = 0;
  nsIMdbCell* outCell = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);

  if ( ev )
  {
    if ( inColumn )
    {
      mork_pos pos = 0;
      morkCell* cell = mRowObject_Row->GetCell(ev, inColumn, &pos);
      if ( cell )
      {
        outCell = mRowObject_Row->AcquireCellHandle(ev, cell, inColumn, pos);
      }
    }
    else
      mRowObject_Row->ZeroColumnError(ev);
      
    outErr = ev->AsErr();
  }
  if ( acqCell )
    *acqCell = outCell;
  return outErr;
}
  
NS_IMETHODIMP
morkRowObject::EmptyAllCells( 
  nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    EmptyAllCells(ev);
    outErr = ev->AsErr();
  }
  return outErr;
}



NS_IMETHODIMP
morkRowObject::AddRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioSourceRow)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRow* unsafeSource = (morkRow*) ioSourceRow; 

    {
      mRowObject_Row->AddRow(ev, unsafeSource);
    }
    outErr = ev->AsErr();
  }
  return outErr;
}
  
NS_IMETHODIMP
morkRowObject::SetRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioSourceRow)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRowObject *sourceObject = (morkRowObject *) ioSourceRow; 
    morkRow* unsafeSource = sourceObject->mRowObject_Row;

    {
      mRowObject_Row->SetRow(ev, unsafeSource);
    }
    outErr = ev->AsErr();
  }
  return outErr;
}



NS_IMETHODIMP
morkRowObject::SetCellYarn( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  const mdbYarn* inYarn)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( mRowObject_Store )
      AddColumn(ev, inColumn, inYarn);
      
    outErr = ev->AsErr();
  }
  return outErr;
}
NS_IMETHODIMP
morkRowObject::GetCellYarn(
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  mdbYarn* outYarn)  

{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( mRowObject_Store && mRowObject_Row)
    {
      morkAtom* atom = mRowObject_Row->GetColumnAtom(ev, inColumn);
      atom->GetYarn(outYarn);
      
    }
      
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkRowObject::AliasCellYarn(
  nsIMdbEnv* mev, 
    mdb_column inColumn, 
    mdbYarn* outYarn) 
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( mRowObject_Store && mRowObject_Row)
    {
      morkAtom* atom = mRowObject_Row->GetColumnAtom(ev, inColumn);
      atom->AliasYarn(outYarn);
      
    }
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkRowObject::NextCellYarn(nsIMdbEnv* mev, 
  mdb_column* ioColumn, 
  mdbYarn* outYarn)  












{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( mRowObject_Store && mRowObject_Row)
      mRowObject_Row->NextColumn(ev, ioColumn, outYarn);
      
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkRowObject::SeekCellYarn( 
  nsIMdbEnv* mev, 
  mdb_pos inPos, 
  mdb_column* outColumn, 
  mdbYarn* outYarn) 





{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( mRowObject_Store && mRowObject_Row)
      mRowObject_Row->SeekColumn(ev, inPos, outColumn, outYarn);
      
    outErr = ev->AsErr();
  }
  return outErr;
}








 void
morkRowObject::CloseRowObject(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      morkRow* row = mRowObject_Row;
      mRowObject_Row = 0;
      this->CloseObject(ev);
      this->MarkShut();

      if ( row )
      {
        MORK_ASSERT(row->mRow_Object == this);
        if ( row->mRow_Object == this )
        {
          row->mRow_Object = 0; 
          
          mRowObject_Store = 0; 
            
          this->CutWeakRef(ev->AsMdbEnv()); 
        }
      }
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void
morkRowObject::NonRowObjectTypeError(morkEnv* ev)
{
  ev->NewError("non morkRowObject");
}

 void
morkRowObject::NilRowError(morkEnv* ev)
{
  ev->NewError("nil mRowObject_Row");
}

 void
morkRowObject::NilStoreError(morkEnv* ev)
{
  ev->NewError("nil mRowObject_Store");
}

 void
morkRowObject::RowObjectRowNotSelfError(morkEnv* ev)
{
  ev->NewError("mRowObject_Row->mRow_Object != self");
}


nsIMdbRow*
morkRowObject::AcquireRowHandle(morkEnv* ev) 
{
  AddRef();
  return this;
}



