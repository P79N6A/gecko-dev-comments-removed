




































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

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _ORKINSTORE_
#include "orkinStore.h"
#endif

#ifndef _MORKPORTTABLECURSOR_
#include "morkPortTableCursor.h"
#endif

#ifndef _ORKINPORTTABLECURSOR_
#include "orkinPortTableCursor.h"
#endif




orkinPortTableCursor:: ~orkinPortTableCursor() 
{
}


orkinPortTableCursor::orkinPortTableCursor(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkPortTableCursor* ioObject)  
: morkHandle(ev, ioFace, ioObject,
  morkMagic_kPortTableCursor)
{
  
}


 orkinPortTableCursor*
orkinPortTableCursor::MakePortTableCursor(morkEnv* ev,
   morkPortTableCursor* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinPortTableCursor));
    if ( face )
      return new(face) orkinPortTableCursor(ev, face, ioObject);
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinPortTableCursor*) 0;
}

morkEnv*
orkinPortTableCursor::CanUsePortTableCursor(nsIMdbEnv* mev,
  mork_bool inMutable, mdb_err* outErr) const
{
  morkEnv* outEnv = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkPortTableCursor* self = (morkPortTableCursor*)
      this->GetGoodHandleObject(ev, inMutable, morkMagic_kPortTableCursor,
         morkBool_kFalse);
    if ( self )
    {
      if ( self->IsPortTableCursor() )
        outEnv = ev;
      else
        self->NonPortTableCursorTypeError(ev);
    }
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv);
  return outEnv;
}


NS_IMPL_QUERY_INTERFACE0(orkinPortTableCursor)

 nsrefcnt
orkinPortTableCursor::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinPortTableCursor::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}






 mdb_err
orkinPortTableCursor::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinPortTableCursor::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinPortTableCursor::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinPortTableCursor::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinPortTableCursor::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinPortTableCursor::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinPortTableCursor::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinPortTableCursor::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinPortTableCursor::CloseMdbObject(nsIMdbEnv* mev)
{
  return this->Handle_CloseMdbObject(mev);
}

 mdb_err
orkinPortTableCursor::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}







 mdb_err
orkinPortTableCursor::GetCount(nsIMdbEnv* mev, mdb_count* outCount)
{
  mdb_err outErr = 0;
  mdb_count count = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outCount )
    *outCount = count;
  return outErr;
}

 mdb_err
orkinPortTableCursor::GetSeed(nsIMdbEnv* mev, mdb_seed* outSeed)
{
  mdb_err outErr = 0;
  mdb_seed seed = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
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
orkinPortTableCursor::SetPos(nsIMdbEnv* mev, mdb_pos inPos)
{
  MORK_USED_1(inPos);
  mdb_err outErr = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinPortTableCursor::GetPos(nsIMdbEnv* mev, mdb_pos* outPos)
{
  mdb_err outErr = 0;
  mdb_pos pos = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outPos )
    *outPos = pos;
    
  return outErr;
}

 mdb_err
orkinPortTableCursor::SetDoFailOnSeedOutOfSync(nsIMdbEnv* mev, mdb_bool inFail)
{
  MORK_USED_1(inFail);
  mdb_err outErr = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinPortTableCursor::GetDoFailOnSeedOutOfSync(nsIMdbEnv* mev, mdb_bool* outFail)
{
  mdb_err outErr = 0;
  mdb_bool fail = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outFail )
    *outFail = fail;
  return outErr;
}







 mdb_err
orkinPortTableCursor::SetPort(nsIMdbEnv* mev, nsIMdbPort* ioPort)
{
  MORK_USED_1(ioPort);
  mdb_err outErr = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinPortTableCursor::GetPort(nsIMdbEnv* mev, nsIMdbPort** acqPort)
{
  mdb_err outErr = 0;
  nsIMdbPort* outPort = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkPortTableCursor* cursor = (morkPortTableCursor*) mHandle_Object;
    morkStore* store = cursor->mPortTableCursor_Store;
    if ( store )
      outPort = store->AcquireStoreHandle(ev);
    outErr = ev->AsErr();
  }
  if ( acqPort )
    *acqPort = outPort;
  return outErr;
}

 mdb_err
orkinPortTableCursor::SetRowScope(nsIMdbEnv* mev, 
  mdb_scope inRowScope)
{
  mdb_err outErr = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkPortTableCursor* cursor = (morkPortTableCursor*) mHandle_Object;
    cursor->mCursor_Pos = -1;
    
    cursor->SetRowScope(ev, inRowScope);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinPortTableCursor::GetRowScope(nsIMdbEnv* mev, mdb_scope* outRowScope)
{
  mdb_err outErr = 0;
  mdb_scope rowScope = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkPortTableCursor* cursor = (morkPortTableCursor*) mHandle_Object;
    rowScope = cursor->mPortTableCursor_RowScope;
    outErr = ev->AsErr();
  }
  *outRowScope = rowScope;
  return outErr;
}

  
 mdb_err
orkinPortTableCursor::SetTableKind(nsIMdbEnv* mev, 
  mdb_kind inTableKind)
{
  mdb_err outErr = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkPortTableCursor* cursor = (morkPortTableCursor*) mHandle_Object;
    cursor->mCursor_Pos = -1;
    
    cursor->SetTableKind(ev, inTableKind);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinPortTableCursor::GetTableKind(nsIMdbEnv* mev, mdb_kind* outTableKind)

{
  mdb_err outErr = 0;
  mdb_kind tableKind = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkPortTableCursor* cursor = (morkPortTableCursor*) mHandle_Object;
    tableKind = cursor->mPortTableCursor_TableKind;
    outErr = ev->AsErr();
  }
  *outTableKind = tableKind;
  return outErr;
}



 mdb_err
orkinPortTableCursor::NextTable( 
  nsIMdbEnv* mev, 
  nsIMdbTable** acqTable)
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkPortTableCursor* cursor = (morkPortTableCursor*) mHandle_Object;
    morkTable* table = cursor->NextTable(ev);
    if ( table && ev->Good() )
      outTable = table->AcquireTableHandle(ev);
        
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}






