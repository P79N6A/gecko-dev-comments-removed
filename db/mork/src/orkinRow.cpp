




































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

#ifndef _MORKATOM_
#include "morkAtom.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _ORKINROW_
#include "orkinRow.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKROWOBJECT_
#include "morkRowObject.h"
#endif

#ifndef _MORKCELLOBJECT_
#include "morkCellObject.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _ORKINSTORE_
#include "orkinStore.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif

#ifndef _MORKROWCELLCURSOR_
#include "morkRowCellCursor.h"
#endif

#ifndef _ORKINROWCELLCURSOR_
#include "orkinRowCellCursor.h"
#endif

#ifndef _ORKINCELL_
#include "orkinCell.h"
#endif




orkinRow:: ~orkinRow() 
{
}    


orkinRow::orkinRow(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkRowObject* ioObject)  
: morkHandle(ev, ioFace, ioObject, morkMagic_kRow)
{
  
}


 orkinRow*
orkinRow::MakeRow(morkEnv* ev,  morkRowObject* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinRow));
    if ( face )
      return new(face) orkinRow(ev, face, ioObject);
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinRow*) 0;
}

morkEnv*
orkinRow::CanUseRow(nsIMdbEnv* mev, mork_bool inMutable,
  mdb_err* outErr, morkRow** outRow) const
{
  morkEnv* outEnv = 0;
  morkRow* innerRow = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRowObject* rowObj = (morkRowObject*)
      this->GetGoodHandleObject(ev, inMutable, morkMagic_kRow,
         morkBool_kFalse);
    if ( rowObj )
    {
      if ( rowObj->IsRowObject() )
      {
        morkRow* row = rowObj->mRowObject_Row;
        if ( row )
        {
          if ( row->IsRow() )
          {
            if ( row->mRow_Object == rowObj )
            {
              outEnv = ev;
              innerRow = row;
            }
            else
              rowObj->RowObjectRowNotSelfError(ev);
          }
          else
            row->NonRowTypeError(ev);
        }
        else
          rowObj->NilRowError(ev);
      }
      else
        rowObj->NonRowObjectTypeError(ev);
    }
    *outErr = ev->AsErr();
  }
  if ( outRow )
    *outRow = innerRow;
  MORK_ASSERT(outEnv);
  return outEnv;
}

morkStore*
orkinRow::CanUseRowStore(morkEnv* ev) const
{
  morkStore* outStore = 0;
  morkRowObject* rowObj = (morkRowObject*) mHandle_Object;
  if ( rowObj && rowObj->IsRowObject() )
  {
    morkStore* store = rowObj->mRowObject_Store;
    if ( store )
    {
      if ( store->IsStore() )
      {
        outStore = store;
      }
      else
        store->NonStoreTypeError(ev);
    }
    else
      rowObj->NilStoreError(ev);
  }
  return outStore;
}



NS_IMPL_QUERY_INTERFACE1(orkinRow, nsIMdbRow)

 nsrefcnt
orkinRow::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinRow::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}






 mdb_err
orkinRow::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinRow::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinRow::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinRow::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinRow::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinRow::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinRow::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinRow::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinRow::CloseMdbObject(nsIMdbEnv* mev)
{
  return this->Handle_CloseMdbObject(mev);
}

 mdb_err
orkinRow::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}








 mdb_err
orkinRow::GetSeed(nsIMdbEnv* mev,
  mdb_seed* outSeed)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    *outSeed = (mdb_seed) row->mRow_Seed;
    outErr = ev->AsErr();
  }
  return outErr;
}
 mdb_err
orkinRow::GetCount(nsIMdbEnv* mev,
  mdb_count* outCount)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    *outCount = (mdb_count) row->mRow_Length;
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinRow::GetPort(nsIMdbEnv* mev,
  nsIMdbPort** acqPort)
{
  mdb_err outErr = 0;
  nsIMdbPort* outPort = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    morkRowSpace* rowSpace = row->mRow_Space;
    if ( rowSpace && rowSpace->mSpace_Store )
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
orkinRow::GetCursor( 
  nsIMdbEnv* mev, 
  mdb_pos inMemberPos, 
  nsIMdbCursor** acqCursor)
{
  return this->GetRowCellCursor(mev, inMemberPos,
    (nsIMdbRowCellCursor**) acqCursor);
}



 mdb_err
orkinRow::GetOid(nsIMdbEnv* mev,
  mdbOid* outOid)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    *outOid = row->mRow_Oid;
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinRow::BecomeContent(nsIMdbEnv* mev,
  const mdbOid* inOid)
{
  MORK_USED_1(inOid);
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    
    
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  return outErr;
}



 mdb_err
orkinRow::DropActivity( 
  nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
    }







 mdb_err
orkinRow::GetRowCellCursor( 
  nsIMdbEnv* mev, 
  mdb_pos inPos, 
  nsIMdbRowCellCursor** acqCursor)
{
  mdb_err outErr = 0;
  nsIMdbRowCellCursor* outCursor = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    morkRowCellCursor* cursor = row->NewRowCellCursor(ev, inPos);
    if ( cursor )
    {
      if ( ev->Good() )
      {
        cursor->mCursor_Seed = (mork_seed) inPos;
        outCursor = cursor->AcquireRowCellCursorHandle(ev);
      }
      else
        cursor->CutStrongRef(mev);
    }
    outErr = ev->AsErr();
  }
  if ( acqCursor )
    *acqCursor = outCursor;
  return outErr;
}



 mdb_err
orkinRow::AddColumn( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  const mdbYarn* inYarn)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    morkStore* store = this->CanUseRowStore(ev);
    if ( store )
      row->AddColumn(ev, inColumn, inYarn, store);
      
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinRow::CutColumn( 
  nsIMdbEnv* mev, 
  mdb_column inColumn)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    row->CutColumn(ev, inColumn);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinRow::CutAllColumns( 
  nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    row->CutAllColumns(ev);
    outErr = ev->AsErr();
  }
  return outErr;
}



 mdb_err
orkinRow::NewCell( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbCell** acqCell)
{
  mdb_err outErr = 0;
  nsIMdbCell* outCell = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    mork_pos pos = 0;
    morkCell* cell = row->GetCell(ev, inColumn, &pos);
    if ( !cell )
    {
      morkStore* store = this->CanUseRowStore(ev);
      if ( store )
      {
        mdbYarn yarn; 
        yarn.mYarn_Buf = 0;
        yarn.mYarn_Fill = 0;
        yarn.mYarn_Size = 0;
        yarn.mYarn_More = 0;
        yarn.mYarn_Form = 0;
        yarn.mYarn_Grow = 0;
        row->AddColumn(ev, inColumn, &yarn, store);
        cell = row->GetCell(ev, inColumn, &pos);
      }
    }
    if ( cell )
      outCell = row->AcquireCellHandle(ev, cell, inColumn, pos);
      
    outErr = ev->AsErr();
  }
  if ( acqCell )
    *acqCell = outCell;
  return outErr;
}
  
 mdb_err
orkinRow::AddCell( 
  nsIMdbEnv* mev, 
  const nsIMdbCell* inCell)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    morkCell* cell = 0;
    morkCellObject* cellObj = (morkCellObject*) inCell;
    if ( cellObj->CanUseCell(mev, morkBool_kFalse, &outErr, &cell) )
    {

      morkRow* cellRow = cellObj->mCellObject_Row;
      if ( cellRow )
      {
        if ( row != cellRow )
        {
          morkStore* store = row->GetRowSpaceStore(ev);
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
              row->AddColumn(ev, col, &yarn, store);
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
  
 mdb_err
orkinRow::GetCell( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbCell** acqCell)
{
  mdb_err outErr = 0;
  nsIMdbCell* outCell = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    if ( inColumn )
    {
      mork_pos pos = 0;
      morkCell* cell = row->GetCell(ev, inColumn, &pos);
      if ( cell )
      {
        outCell = row->AcquireCellHandle(ev, cell, inColumn, pos);
      }
    }
    else
      row->ZeroColumnError(ev);
      
    outErr = ev->AsErr();
  }
  if ( acqCell )
    *acqCell = outCell;
  return outErr;
}
  
 mdb_err
orkinRow::EmptyAllCells( 
  nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    row->EmptyAllCells(ev);
    outErr = ev->AsErr();
  }
  return outErr;
}



 mdb_err
orkinRow::AddRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioSourceRow)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    morkRow* source = 0;
    orkinRow* unsafeSource = (orkinRow*) ioSourceRow; 
    if ( unsafeSource->CanUseRow(mev, morkBool_kFalse, &outErr, &source) )
    {
      row->AddRow(ev, source);
    }
    outErr = ev->AsErr();
  }
  return outErr;
}
  
 mdb_err
orkinRow::SetRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioSourceRow)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    morkRow* source = 0;
    orkinRow* unsafeSource = (orkinRow*) ioSourceRow; 
    if ( unsafeSource->CanUseRow(mev, morkBool_kFalse, &outErr, &source) )
    {
      row->SetRow(ev, source);
    }
    outErr = ev->AsErr();
  }
  return outErr;
}



 mdb_err
orkinRow::SetCellYarn( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  const mdbYarn* inYarn)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    morkStore* store = this->CanUseRowStore(ev);
    if ( store )
      row->AddColumn(ev, inColumn, inYarn, store);
      
    outErr = ev->AsErr();
  }
  return outErr;
}
 mdb_err
orkinRow::GetCellYarn(
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  mdbYarn* outYarn)  

{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    morkStore* store = this->CanUseRowStore(ev);
    if ( store )
    {
	    morkAtom* atom = row->GetColumnAtom(ev, inColumn);
	    atom->GetYarn(outYarn);
	    
    }
      
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinRow::AliasCellYarn(
  nsIMdbEnv* mev, 
    mdb_column inColumn, 
    mdbYarn* outYarn) 
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    morkStore* store = this->CanUseRowStore(ev);
    if ( store )
    {
	    morkAtom* atom = row->GetColumnAtom(ev, inColumn);
	    atom->AliasYarn(outYarn);
	    
    }
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinRow::NextCellYarn(nsIMdbEnv* mev, 
  mdb_column* ioColumn, 
  mdbYarn* outYarn)  












{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    morkStore* store = this->CanUseRowStore(ev);
    if ( store )
      row->NextColumn(ev, ioColumn, outYarn);
      
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinRow::SeekCellYarn( 
  nsIMdbEnv* mev, 
  mdb_pos inPos, 
  mdb_column* outColumn, 
  mdbYarn* outYarn) 





{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev = this->CanUseRow(mev,  morkBool_kFalse,
    &outErr, &row);
  if ( ev )
  {
    morkStore* store = this->CanUseRowStore(ev);
    if ( store )
      row->SeekColumn(ev, inPos, outColumn, outYarn);
      
    outErr = ev->AsErr();
  }
  return outErr;
}










