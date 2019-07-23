




































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

#ifndef _MORKROWCELLCURSOR_
#include "morkRowCellCursor.h"
#endif

#ifndef _ORKINROWCELLCURSOR_
#include "orkinRowCellCursor.h"
#endif

#ifndef _MORKROWOBJECT_
#include "morkRowObject.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _ORKINROW_
#include "orkinRow.h"
#endif

#ifndef _MORKCELL_
#include "morkCell.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif




orkinRowCellCursor:: ~orkinRowCellCursor() 
{
}


orkinRowCellCursor::orkinRowCellCursor(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkRowCellCursor* ioObject)  
: morkHandle(ev, ioFace, ioObject, morkMagic_kRowCellCursor)
{
  
}


 orkinRowCellCursor*
orkinRowCellCursor::MakeRowCellCursor(morkEnv* ev, morkRowCellCursor* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinRowCellCursor));
    if ( face )
      return new(face) orkinRowCellCursor(ev, face, ioObject);
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinRowCellCursor*) 0;
}

morkEnv*
orkinRowCellCursor::CanUseRowCellCursor(nsIMdbEnv* mev, mork_bool inMutable,
  mdb_err* outErr, morkRow** outRow) const
{
  morkEnv* outEnv = 0;
  morkRow* row = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRowCellCursor* self = (morkRowCellCursor*)
      this->GetGoodHandleObject(ev, inMutable, morkMagic_kRowCellCursor,
         morkBool_kFalse);
    if ( self )
    {
      if ( self->IsRowCellCursor() )
      {
        if ( self->IsMutable() || !inMutable )
        {
          morkRowObject* rowObj = self->mRowCellCursor_RowObject;
          if ( rowObj )
          {
            morkRow* theRow = rowObj->mRowObject_Row;
            if ( theRow )
            {
              if ( theRow->IsRow() )
              {
                outEnv = ev;
                row = theRow;
              }
              else
                theRow->NonRowTypeError(ev);
            }
            else
              rowObj->NilRowError(ev);
          }
          else
            self->NilRowObjectError(ev);
        }
        else
          self->NonMutableNodeError(ev);
      }
      else
        self->NonRowCellCursorTypeError(ev);
    }
    *outErr = ev->AsErr();
  }
  *outRow = row;
  MORK_ASSERT(outEnv);
  return outEnv;
}


NS_IMPL_QUERY_INTERFACE0(orkinRowCellCursor)

 nsrefcnt
orkinRowCellCursor::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinRowCellCursor::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}





 mdb_err
orkinRowCellCursor::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinRowCellCursor::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinRowCellCursor::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinRowCellCursor::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinRowCellCursor::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinRowCellCursor::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinRowCellCursor::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinRowCellCursor::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinRowCellCursor::CloseMdbObject(nsIMdbEnv* mev)
{
  return this->Handle_CloseMdbObject(mev);
}

 mdb_err
orkinRowCellCursor::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}







 mdb_err
orkinRowCellCursor::GetCount(nsIMdbEnv* mev, mdb_count* outCount)
{
  mdb_err outErr = 0;
  mdb_count count = 0;
  morkRow* row = 0;
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    count = row->mRow_Length;
    outErr = ev->AsErr();
  }
  if ( outCount )
    *outCount = count;
  return outErr;
}

 mdb_err
orkinRowCellCursor::GetSeed(nsIMdbEnv* mev, mdb_seed* outSeed)
{
  mdb_err outErr = 0;
  mdb_seed seed = 0;
  morkRow* row = 0;
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    seed = row->mRow_Seed;
    outErr = ev->AsErr();
  }
  if ( outSeed )
    *outSeed = seed;
  return outErr;
}

 mdb_err
orkinRowCellCursor::SetPos(nsIMdbEnv* mev, mdb_pos inPos)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    morkRowCellCursor* cursor = (morkRowCellCursor*) mHandle_Object;
    cursor->mCursor_Pos = inPos;
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinRowCellCursor::GetPos(nsIMdbEnv* mev, mdb_pos* outPos)
{
  mdb_err outErr = 0;
  mdb_pos pos = 0;
  morkRow* row = 0;
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    morkRowCellCursor* cursor = (morkRowCellCursor*) mHandle_Object;
    pos = cursor->mCursor_Pos;
    outErr = ev->AsErr();
  }
  if ( outPos )
    *outPos = pos;
  return outErr;
}

 mdb_err
orkinRowCellCursor::SetDoFailOnSeedOutOfSync(nsIMdbEnv* mev, mdb_bool inFail)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    morkRowCellCursor* cursor = (morkRowCellCursor*) mHandle_Object;
    cursor->mCursor_DoFailOnSeedOutOfSync = inFail;
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinRowCellCursor::GetDoFailOnSeedOutOfSync(nsIMdbEnv* mev, mdb_bool* outFail)
{
  mdb_err outErr = 0;
  mdb_bool doFail = morkBool_kFalse;
  morkRow* row = 0;
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    morkRowCellCursor* cursor = (morkRowCellCursor*) mHandle_Object;
    doFail = cursor->mCursor_DoFailOnSeedOutOfSync;
    outErr = ev->AsErr();
  }
  if ( outFail )
    *outFail = doFail;
  return outErr;
}







 mdb_err
orkinRowCellCursor::SetRow(nsIMdbEnv* mev, nsIMdbRow* ioRow)
{
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    morkRowCellCursor* cursor = (morkRowCellCursor*) mHandle_Object;
    row = (morkRow *) ioRow;
    morkStore* store = row->GetRowSpaceStore(ev);
    if ( store )
    {
      morkRowObject* rowObj = row->AcquireRowObject(ev, store);
      if ( rowObj )
      {
        morkRowObject::SlotStrongRowObject((morkRowObject*) 0, ev,
          &cursor->mRowCellCursor_RowObject);
          
        cursor->mRowCellCursor_RowObject = rowObj; 
        cursor->mCursor_Seed = row->mRow_Seed;
        
        row->GetCell(ev, cursor->mRowCellCursor_Col, &cursor->mCursor_Pos);
      }
    }
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinRowCellCursor::GetRow(nsIMdbEnv* mev, nsIMdbRow** acqRow)
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkRow* row = 0;
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    morkRowCellCursor* cursor = (morkRowCellCursor*) mHandle_Object;
    morkRowObject* rowObj = cursor->mRowCellCursor_RowObject;
    if ( rowObj )
      outRow = rowObj->AcquireRowHandle(ev);

    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}



 mdb_err
orkinRowCellCursor::MakeCell( 
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
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    morkRowCellCursor* cursor = (morkRowCellCursor*) mHandle_Object;
    pos = cursor->mCursor_Pos;
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



 mdb_err
orkinRowCellCursor::SeekCell( 
  nsIMdbEnv* mev, 
  mdb_pos inPos, 
  mdb_column* outColumn, 
  nsIMdbCell** acqCell)
{
  MORK_USED_1(inPos);
  mdb_err outErr = 0;
  mdb_column column = 0;
  nsIMdbCell* outCell = 0;
  morkRow* row = 0;
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    morkRowCellCursor* cursor;
    cursor = (morkRowCellCursor*) mHandle_Object;
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( acqCell )
    *acqCell = outCell;
  if ( outColumn )
    *outColumn = column;
  return outErr;
}



 mdb_err
orkinRowCellCursor::NextCell( 
  nsIMdbEnv* mev, 
  nsIMdbCell* ioCell, 
  mdb_column* outColumn, 
  mdb_pos* outPos)
{
  MORK_USED_1(ioCell);
  mdb_err outErr = 0;
  mdb_pos pos = -1;
  mdb_column column = 0;
  morkRow* row = 0;
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    morkRowCellCursor* cursor;
    cursor = (morkRowCellCursor*) mHandle_Object;
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outColumn )
    *outColumn = column;
  if ( outPos )
    *outPos = pos;
  return outErr;
}
  
 mdb_err
orkinRowCellCursor::PickNextCell( 
  nsIMdbEnv* mev, 
  nsIMdbCell* ioCell, 
  const mdbColumnSet* inFilterSet, 
  mdb_column* outColumn, 
  mdb_pos* outPos)



{
  MORK_USED_2(ioCell,inFilterSet);
  mdb_pos pos = -1;
  mdb_column column = 0;
  mdb_err outErr = 0;
  morkRow* row = 0;
  morkEnv* ev =
    this->CanUseRowCellCursor(mev,  morkBool_kFalse, &outErr, &row);
  if ( ev )
  {
    morkRowCellCursor* cursor;
    cursor = (morkRowCellCursor*) mHandle_Object;
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outColumn )
    *outColumn = column;
  if ( outPos )
    *outPos = pos;
  return outErr;
}







