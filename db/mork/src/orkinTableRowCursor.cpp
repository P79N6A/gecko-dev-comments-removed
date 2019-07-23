




































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

#ifndef _MORKFACTORY_
#include "morkFactory.h"
#endif

#ifndef _ORKINFACTORY_
#include "orkinFactory.h"
#endif

#ifndef _MORKTABLEROWCURSOR_
#include "morkTableRowCursor.h"
#endif

#ifndef _ORKINTABLEROWCURSOR_
#include "orkinTableRowCursor.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _MORKARRAY_
#include "morkArray.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _ORKINTABLE_
#include "orkinTable.h"
#endif




orkinTableRowCursor:: ~orkinTableRowCursor()

{
}


orkinTableRowCursor::orkinTableRowCursor(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkTableRowCursor* ioObject)  
: morkHandle(ev, ioFace, ioObject, morkMagic_kTableRowCursor)
{
  
}


 orkinTableRowCursor*
orkinTableRowCursor::MakeTableRowCursor(morkEnv* ev,
   morkTableRowCursor* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinTableRowCursor));
    if ( face )
      return new(face) orkinTableRowCursor(ev, face, ioObject);
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinTableRowCursor*) 0;
}

morkEnv*
orkinTableRowCursor::CanUseTableRowCursor(nsIMdbEnv* mev,
  mork_bool inMutable, mdb_err* outErr) const
{
  morkEnv* outEnv = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkTableRowCursor* self = (morkTableRowCursor*)
      this->GetGoodHandleObject(ev, inMutable, morkMagic_kTableRowCursor,
         morkBool_kFalse);
    if ( self )
    {
      if ( self->IsTableRowCursor() )
      {
        morkTable* table = self->mTableRowCursor_Table;
        if ( table && table->IsOpenNode() )
        {
          outEnv = ev;
        }
      }
      else
        self->NonTableRowCursorTypeError(ev);
    }
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv);
  return outEnv;
}


NS_IMPL_QUERY_INTERFACE1(orkinTableRowCursor, nsIMdbTableRowCursor)

 nsrefcnt
orkinTableRowCursor::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinTableRowCursor::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}






 mdb_err
orkinTableRowCursor::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinTableRowCursor::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinTableRowCursor::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinTableRowCursor::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinTableRowCursor::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinTableRowCursor::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinTableRowCursor::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinTableRowCursor::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinTableRowCursor::CloseMdbObject(nsIMdbEnv* mev)
{
  return this->Handle_CloseMdbObject(mev);
}

 mdb_err
orkinTableRowCursor::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}







 mdb_err
orkinTableRowCursor::GetCount(nsIMdbEnv* mev, mdb_count* outCount)
{
  mdb_err outErr = 0;
  mdb_count count = 0;
  morkEnv* ev =
    this->CanUseTableRowCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTableRowCursor* cursor = (morkTableRowCursor*) mHandle_Object;
    count = cursor->GetMemberCount(ev);
    outErr = ev->AsErr();
  }
  if ( outCount )
    *outCount = count;
  return outErr;
}

 mdb_err
orkinTableRowCursor::GetSeed(nsIMdbEnv* mev, mdb_seed* outSeed)
{
  mdb_err outErr = 0;
  mdb_seed seed = 0;
  morkEnv* ev =
    this->CanUseTableRowCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outSeed )
    *outSeed = seed;
  return outErr;
}

 mdb_err
orkinTableRowCursor::SetPos(nsIMdbEnv* mev, mdb_pos inPos)
{
  mdb_err outErr = 0;
  morkEnv* ev =
    this->CanUseTableRowCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTableRowCursor* cursor = (morkTableRowCursor*) mHandle_Object;
    cursor->mCursor_Pos = inPos;
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTableRowCursor::GetPos(nsIMdbEnv* mev, mdb_pos* outPos)
{
  mdb_err outErr = 0;
  mdb_pos pos = 0;
  morkEnv* ev =
    this->CanUseTableRowCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTableRowCursor* cursor = (morkTableRowCursor*) mHandle_Object;
    pos = cursor->mCursor_Pos;
    outErr = ev->AsErr();
  }
  if ( outPos )
    *outPos = pos;
  return outErr;
}

 mdb_err
orkinTableRowCursor::SetDoFailOnSeedOutOfSync(nsIMdbEnv* mev, mdb_bool inFail)
{
  MORK_USED_1(inFail);
  mdb_err outErr = 0;
  morkEnv* ev =
    this->CanUseTableRowCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTableRowCursor* cursor = (morkTableRowCursor*) mHandle_Object;
    cursor->mCursor_DoFailOnSeedOutOfSync = inFail;
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTableRowCursor::GetDoFailOnSeedOutOfSync(nsIMdbEnv* mev, mdb_bool* outFail)
{
  mdb_bool fail = morkBool_kFalse;
  mdb_err outErr = 0;
  morkEnv* ev =
    this->CanUseTableRowCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTableRowCursor* cursor = (morkTableRowCursor*) mHandle_Object;
    fail = cursor->mCursor_DoFailOnSeedOutOfSync;
    outErr = ev->AsErr();
  }
  if ( outFail )
    *outFail = fail;
  return outErr;
}









 mdb_err
orkinTableRowCursor::GetTable(nsIMdbEnv* mev, nsIMdbTable** acqTable)
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev =
    this->CanUseTableRowCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTableRowCursor* cursor = (morkTableRowCursor*) mHandle_Object;
    morkTable* table = cursor->mTableRowCursor_Table;
    if ( table )
      outTable = table->AcquireTableHandle(ev);
    
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}



 mdb_err
orkinTableRowCursor::NextRowOid( 
  nsIMdbEnv* mev, 
  mdbOid* outOid, 
  mdb_pos* outRowPos)
{
  mdb_err outErr = 0;
  mork_pos pos = -1;
  morkEnv* ev =
    this->CanUseTableRowCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( outOid )
    {
      pos = ((morkTableRowCursor*) mHandle_Object)->NextRowOid(ev, outOid);
    }
    else
      ev->NilPointerError();
    outErr = ev->AsErr();
  }
  if ( outRowPos )
    *outRowPos = pos;
  return outErr;
}



 mdb_err
orkinTableRowCursor::NextRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow** acqRow, 
  mdb_pos* outRowPos)
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev =
    this->CanUseTableRowCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTableRowCursor* cursor = (morkTableRowCursor*) mHandle_Object;
      
    mdbOid oid; 
    morkRow* row = cursor->NextRow(ev, &oid, outRowPos);
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




 mdb_err
orkinTableRowCursor::CanHaveDupRowMembers(nsIMdbEnv* mev, 
  mdb_bool* outCanHaveDups)
{
  mdb_err outErr = 0;
  mdb_bool canHaveDups = mdbBool_kFalse;
  
  morkEnv* ev =
    this->CanUseTableRowCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTableRowCursor* cursor = (morkTableRowCursor*) mHandle_Object;
    canHaveDups = cursor->CanHaveDupRowMembers(ev);
    outErr = ev->AsErr();
  }
  if ( outCanHaveDups )
    *outCanHaveDups = canHaveDups;
  return outErr;
}
  
 mdb_err
orkinTableRowCursor::MakeUniqueCursor( 
  nsIMdbEnv* mev, 
  nsIMdbTableRowCursor** acqCursor)    
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  nsIMdbTableRowCursor* outCursor = 0;
  
  morkEnv* ev =
    this->CanUseTableRowCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( this->Handle_AddStrongRef(mev) == 0 )
      outCursor = this;
      
    outErr = ev->AsErr();
  }
  if ( acqCursor )
    *acqCursor = outCursor;
  return outErr;
}






