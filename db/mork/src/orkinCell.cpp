




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKHANDLE_
#include "morkHandle.h"
#endif

#ifndef _MORKCELL_
#include "morkCell.h"
#endif

#ifndef _MORKCELLOBJECT_
#include "morkCellObject.h"
#endif

#ifndef _MORKROWOBJECT_
#include "morkRowObject.h"
#endif

#ifndef _ORKINCELL_
#include "orkinCell.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKPOOL_
#include "morkPool.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _MORKATOM_
#include "morkAtom.h"
#endif

#ifndef _MORKSPACE_
#include "morkSpace.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif




orkinCell:: ~orkinCell() 
{
}


orkinCell::orkinCell(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkCellObject* ioObject)  
: morkHandle(ev, ioFace, ioObject, morkMagic_kCell)
{
  
}


 orkinCell*
orkinCell::MakeCell(morkEnv* ev, morkCellObject* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinCell));
    if ( face )
      return new(face) orkinCell(ev, face, ioObject);
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinCell*) 0;
}























morkEnv*
orkinCell::CanUseCell(nsIMdbEnv* mev, mork_bool inMutable,
  mdb_err* outErr, morkCell** outCell) const
{
  morkEnv* outEnv = 0;
  morkCell* cell = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkCellObject* cellObj = (morkCellObject*)
      this->GetGoodHandleObject(ev, inMutable, morkMagic_kCell,
         morkBool_kFalse);
    if ( cellObj )
    {
      if ( cellObj->IsCellObject() )
      {
        if ( cellObj->IsMutable() || !inMutable )
        {
          morkRowObject* rowObj = cellObj->mCellObject_RowObject;
          if ( rowObj )
          {
            morkRow* row = cellObj->mCellObject_Row;
            if ( row )
            {
              if ( rowObj->mRowObject_Row == row )
              {
                mork_u2 oldSeed = cellObj->mCellObject_RowSeed;
                if ( row->mRow_Seed == oldSeed || cellObj->ResyncWithRow(ev) )
                {
                  cell = cellObj->mCellObject_Cell;
                  if ( cell )
                  {
                    outEnv = ev;
                  }
                  else
                    cellObj->NilCellError(ev);
                }
              }
              else
                cellObj->WrongRowObjectRowError(ev);
            }
            else
              cellObj->NilRowError(ev);
          }
          else
            cellObj->NilRowObjectError(ev);
        }
        else
          cellObj->NonMutableNodeError(ev);
      }
      else
        cellObj->NonCellObjectTypeError(ev);
    }
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv);
  *outCell = cell;
  
  return outEnv;
}


NS_IMPL_QUERY_INTERFACE0(orkinCell)

 nsrefcnt
orkinCell::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinCell::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}





 mdb_err
orkinCell::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinCell::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinCell::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinCell::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinCell::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinCell::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinCell::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinCell::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinCell::CloseMdbObject(nsIMdbEnv* mev)
{
  return this->Handle_CloseMdbObject(mev);
}

 mdb_err
orkinCell::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}







 mdb_err
orkinCell::SetBlob(nsIMdbEnv* mev,
  nsIMdbBlob* ioBlob)
{
  MORK_USED_1(ioBlob);
  mdb_err outErr = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    

    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
    
  return outErr;
} 


 mdb_err
orkinCell::ClearBlob( 
  nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    

    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
    
  return outErr;
}


 mdb_err
orkinCell::GetBlobFill(nsIMdbEnv* mev,
  mdb_fill* outFill)


{
  mdb_err outErr = 0;
  mdb_fill fill = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outFill )
    *outFill = fill;
    
  return outErr;
}  

 mdb_err
orkinCell::SetYarn(nsIMdbEnv* mev, 
  const mdbYarn* inYarn)
{
  mdb_err outErr = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkCellObject* cellObj = (morkCellObject*) mHandle_Object;
    morkRow* row = cellObj->mCellObject_Row;
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


 mdb_err
orkinCell::GetYarn(nsIMdbEnv* mev, 
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


 mdb_err
orkinCell::AliasYarn(nsIMdbEnv* mev, 
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








 mdb_err
orkinCell::SetColumn(nsIMdbEnv* mev, mdb_column inColumn)
{
  MORK_USED_1(inColumn);
  mdb_err outErr = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    

    morkCellObject* cellObj = (morkCellObject*) mHandle_Object;
    MORK_USED_1(cellObj);
    
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
    
  return outErr;
} 

 mdb_err
orkinCell::GetColumn(nsIMdbEnv* mev, mdb_column* outColumn)
{
  mdb_err outErr = 0;
  mdb_column col = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkCellObject* cellObj = (morkCellObject*) mHandle_Object;
    col = cellObj->mCellObject_Col;
    outErr = ev->AsErr();
  }
  if ( outColumn )
    *outColumn = col;
  return outErr;
}

 mdb_err
orkinCell::GetCellInfo(  
  nsIMdbEnv* mev, 
  mdb_column* outColumn,           
  mdb_fill*   outBlobFill,         
  mdbOid*     outChildOid,         
  mdb_bool*   outIsRowChild)  


{
  mdb_err outErr = 0;
  mdb_bool isRowChild = morkBool_kFalse;
  mdbOid childOid;
  childOid.mOid_Scope = 0;
  childOid.mOid_Id = 0;
  mork_fill blobFill = 0;
  mdb_column column = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkCellObject* cellObj;
    cellObj = (morkCellObject*) mHandle_Object;
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outIsRowChild )
    *outIsRowChild = isRowChild;
  if ( outChildOid )
    *outChildOid = childOid;
   if ( outBlobFill )
     *outBlobFill = blobFill;
  if ( outColumn )
    *outColumn = column;
    
  return outErr;
}


 mdb_err
orkinCell::GetRow(nsIMdbEnv* mev, 
  nsIMdbRow** acqRow)
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkCellObject* cellObj = (morkCellObject*) mHandle_Object;
    morkRowObject* rowObj = cellObj->mCellObject_RowObject;
    outRow = rowObj->AcquireRowHandle(ev);
    
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}

 mdb_err
orkinCell::GetPort(nsIMdbEnv* mev, 
  nsIMdbPort** acqPort)
{
  mdb_err outErr = 0;
  nsIMdbPort* outPort = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkCellObject* cellObj = (morkCellObject*) mHandle_Object;
    morkRow* row = cellObj->mCellObject_Row;
    if ( row )
    {
      morkStore* store = row->GetRowSpaceStore(ev);
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



 mdb_err
orkinCell::HasAnyChild( 
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
    morkCellObject* cellObj = (morkCellObject*) mHandle_Object;
    morkAtom* atom = cellObj->GetCellAtom(ev);
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

 mdb_err
orkinCell::GetAnyChild( 
  nsIMdbEnv* mev, 
  nsIMdbRow** acqRow, 
  nsIMdbTable** acqTable) 
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  nsIMdbTable* outTable = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkCellObject* cellObj;
    cellObj = (morkCellObject*) mHandle_Object;
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  MORK_ASSERT(acqTable);
  if ( acqTable )
    *acqTable = outTable;
  MORK_ASSERT(acqRow);
  if ( acqRow )
    *acqRow = outRow;
    
  return outErr;
}


 mdb_err
orkinCell::SetChildRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow)
{
  MORK_USED_1(ioRow);
  mdb_err outErr = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    

    morkCellObject* cellObj = (morkCellObject*) mHandle_Object;
    MORK_USED_1(cellObj);

    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
    
  return outErr;
} 

 mdb_err
orkinCell::GetChildRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow** acqRow) 
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkCellObject* cellObj;
    cellObj = (morkCellObject*) mHandle_Object;
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
    
  return outErr;
}


 mdb_err
orkinCell::SetChildTable( 
  nsIMdbEnv* mev, 
  nsIMdbTable* inTable) 
{
  MORK_USED_1(inTable);
  mdb_err outErr = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    

    morkCellObject* cellObj = (morkCellObject*) mHandle_Object;
    MORK_USED_1(cellObj);

    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
    
  return outErr;
}

 mdb_err
orkinCell::GetChildTable( 
  nsIMdbEnv* mev, 
  nsIMdbTable** acqTable) 
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkCell* cell = 0;
  morkEnv* ev = this->CanUseCell(mev,  morkBool_kTrue,
    &outErr, &cell);
  if ( ev )
  {
    morkCellObject* cellObj;
    cellObj = (morkCellObject*) mHandle_Object;
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
    
  return outErr;
}






