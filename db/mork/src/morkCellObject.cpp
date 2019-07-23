




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKCELLOBJECT_
#include "morkCellObject.h"
#endif

#ifndef _MORKROWOBJECT_
#include "morkRowObject.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _MORKCELL_
#include "morkCell.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif






 void
morkCellObject::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseCellObject(ev);
    this->MarkShut();
  }
}


morkCellObject::~morkCellObject() 
{
  CloseMorkNode(mMorkEnv);
  MORK_ASSERT(mCellObject_Row==0);
}


morkCellObject::morkCellObject(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, morkRow* ioRow, morkCell* ioCell,
  mork_column inCol, mork_pos inPos)
: morkObject(ev, inUsage, ioHeap, morkColor_kNone, (morkHandle*) 0)
, mCellObject_RowObject( 0 )
, mCellObject_Row( 0 )
, mCellObject_Cell( 0 )
, mCellObject_Col( inCol )
, mCellObject_RowSeed( 0 )
, mCellObject_Pos( (mork_u2) inPos )
{
  if ( ev->Good() )
  {
    if ( ioRow && ioCell )
    {
      if ( ioRow->IsRow() )
      {
        morkStore* store = ioRow->GetRowSpaceStore(ev);
        if ( store )
        {
          morkRowObject* rowObj = ioRow->AcquireRowObject(ev, store);
          if ( rowObj )
          {
            mCellObject_Row = ioRow;
            mCellObject_Cell = ioCell;
            mCellObject_RowSeed = ioRow->mRow_Seed;
            
            
            
              
            mCellObject_RowObject = rowObj; 
          }
          if ( ev->Good() )
            mNode_Derived = morkDerived_kCellObject;
        }
      }
      else
        ioRow->NonRowTypeError(ev);
    }
    else
      ev->NilPointerError();
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(morkCellObject, morkObject, nsIMdbCell)

 void
morkCellObject::CloseCellObject(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      NS_RELEASE(mCellObject_RowObject);
      mCellObject_Row = 0;
      mCellObject_Cell = 0;
      mCellObject_RowSeed = 0;
      this->CloseObject(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




mork_bool
morkCellObject::ResyncWithRow(morkEnv* ev)
{
  morkRow* row = mCellObject_Row;
  mork_pos pos = 0;
  morkCell* cell = row->GetCell(ev, mCellObject_Col, &pos);
  if ( cell )
  {
    mCellObject_Pos = (mork_u2) pos;
    mCellObject_Cell = cell;
    mCellObject_RowSeed = row->mRow_Seed;
  }
  else
  {
    mCellObject_Cell = 0;
    this->MissingRowColumnError(ev);
  }
  return ev->Good();
}

morkAtom*
morkCellObject::GetCellAtom(morkEnv* ev) const
{
  morkCell* cell = mCellObject_Cell;
  if ( cell )
    return cell->GetAtom();
  else
    this->NilCellError(ev);
    
  return (morkAtom*) 0;
}

 void
morkCellObject::WrongRowObjectRowError(morkEnv* ev)
{
  ev->NewError("mCellObject_Row != mCellObject_RowObject->mRowObject_Row");
}

 void
morkCellObject::NilRowError(morkEnv* ev)
{
  ev->NewError("nil mCellObject_Row");
}

 void
morkCellObject::NilRowObjectError(morkEnv* ev)
{
  ev->NewError("nil mCellObject_RowObject");
}

 void
morkCellObject::NilCellError(morkEnv* ev)
{
  ev->NewError("nil mCellObject_Cell");
}

 void
morkCellObject::NonCellObjectTypeError(morkEnv* ev)
{
  ev->NewError("non morkCellObject");
}

 void
morkCellObject::MissingRowColumnError(morkEnv* ev)
{
  ev->NewError("mCellObject_Col not in mCellObject_Row");
}

nsIMdbCell*
morkCellObject::AcquireCellHandle(morkEnv* ev)
{
  nsIMdbCell* outCell = this;
  NS_ADDREF(outCell);
  return outCell;
}


morkEnv*
morkCellObject::CanUseCell(nsIMdbEnv* mev, mork_bool inMutable,
  mdb_err* outErr, morkCell** outCell) 
{
  morkEnv* outEnv = 0;
  morkCell* cell = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( IsCellObject() )
    {
      if ( IsMutable() || !inMutable )
      {
        morkRowObject* rowObj = mCellObject_RowObject;
        if ( rowObj )
        {
          morkRow* row = mCellObject_Row;
          if ( row )
          {
            if ( rowObj->mRowObject_Row == row )
            {
              mork_u2 oldSeed = mCellObject_RowSeed;
              if ( row->mRow_Seed == oldSeed || ResyncWithRow(ev) )
              {
                cell = mCellObject_Cell;
                if ( cell )
                {
                  outEnv = ev;
                }
                else
                  NilCellError(ev);
              }
            }
            else
              WrongRowObjectRowError(ev);
          }
          else
            NilRowError(ev);
        }
        else
          NilRowObjectError(ev);
      }
      else
        NonMutableNodeError(ev);
    }
    else
      NonCellObjectTypeError(ev);
  }
  *outErr = ev->AsErr();
  MORK_ASSERT(outEnv);
  *outCell = cell;
  
  return outEnv;
}


NS_IMETHODIMP morkCellObject::SetBlob(nsIMdbEnv* ,
  nsIMdbBlob* )
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
} 



NS_IMETHODIMP morkCellObject::ClearBlob( 
  nsIMdbEnv*  )
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
  
}


NS_IMETHODIMP morkCellObject::GetBlobFill(nsIMdbEnv* mev,
  mdb_fill* outFill)


{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}  

NS_IMETHODIMP morkCellObject::SetYarn(nsIMdbEnv* mev, 
  const mdbYarn* inYarn)
{
  mdb_err outErr = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkRow* row = mCellObject_Row;
    if ( row )
    {
      morkStore* store = row->GetRowSpaceStore(ev);
      if ( store )
      {
        cell->SetYarn(ev, inYarn, store);
        if ( row->IsRowClean() && store->mStore_CanDirty )
          row->MaybeDirtySpaceStoreAndRow();
      }
    }
    else
      ev->NilPointerError();

    outErr = ev->AsErr();
  }
    
  return outErr;
}   


NS_IMETHODIMP morkCellObject::GetYarn(nsIMdbEnv* mev, 
  mdbYarn* outYarn)
{
  mdb_err outErr = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkAtom* atom = cell->GetAtom();
    atom->GetYarn(outYarn);
    outErr = ev->AsErr();
  }
    
  return outErr;
}  


NS_IMETHODIMP morkCellObject::AliasYarn(nsIMdbEnv* mev, 
  mdbYarn* outYarn)
{
  mdb_err outErr = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkAtom* atom = cell->GetAtom();
    atom->AliasYarn(outYarn);
    outErr = ev->AsErr();
  }
    
  return outErr;
} 








NS_IMETHODIMP morkCellObject::SetColumn(nsIMdbEnv* mev, mdb_column inColumn)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
  
} 

NS_IMETHODIMP morkCellObject::GetColumn(nsIMdbEnv* mev, mdb_column* outColumn)
{
  mdb_err outErr = 0;
  mdb_column col = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    col = mCellObject_Col;
    outErr = ev->AsErr();
  }
  if ( outColumn )
    *outColumn = col;
  return outErr;
}

NS_IMETHODIMP morkCellObject::GetCellInfo(  
  nsIMdbEnv* mev, 
  mdb_column* outColumn,           
  mdb_fill*   outBlobFill,         
  mdbOid*     outChildOid,         
  mdb_bool*   outIsRowChild)  


{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP morkCellObject::GetRow(nsIMdbEnv* mev, 
  nsIMdbRow** acqRow)
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    outRow = mCellObject_RowObject->AcquireRowHandle(ev);
    
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}

NS_IMETHODIMP morkCellObject::GetPort(nsIMdbEnv* mev, 
  nsIMdbPort** acqPort)
{
  mdb_err outErr = 0;
  nsIMdbPort* outPort = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    if ( mCellObject_Row )
    {
      morkStore* store = mCellObject_Row->GetRowSpaceStore(ev);
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



NS_IMETHODIMP morkCellObject::HasAnyChild( 
  nsIMdbEnv* mev,
  mdbOid* outOid,  
  mdb_bool* outIsRow) 
{
  mdb_err outErr = 0;
  mdb_bool isRow = morkBool_kFalse;
  outOid->mOid_Scope = 0;
  outOid->mOid_Id = morkId_kMinusOne;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkAtom* atom = GetCellAtom(ev);
    if ( atom )
    {
      isRow = atom->IsRowOid();
      if ( isRow || atom->IsTableOid() )
        *outOid = ((morkOidAtom*) atom)->mOidAtom_Oid;
    }
      
    outErr = ev->AsErr();
  }
  if ( outIsRow )
    *outIsRow = isRow;
    
  return outErr;
}

NS_IMETHODIMP morkCellObject::GetAnyChild( 
  nsIMdbEnv* mev, 
  nsIMdbRow** acqRow, 
  nsIMdbTable** acqTable) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP morkCellObject::SetChildRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
} 

NS_IMETHODIMP morkCellObject::GetChildRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow** acqRow) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP morkCellObject::SetChildTable( 
  nsIMdbEnv* mev, 
  nsIMdbTable* inTable) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
  
}

NS_IMETHODIMP morkCellObject::GetChildTable( 
  nsIMdbEnv* mev, 
  nsIMdbTable** acqTable) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}






