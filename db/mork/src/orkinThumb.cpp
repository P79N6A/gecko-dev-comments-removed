




































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

#ifndef _MORKTHUMB_
#include "morkThumb.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _ORKINTHUMB_
#include "orkinThumb.h"
#endif




orkinThumb:: ~orkinThumb() 
{
}


orkinThumb::orkinThumb(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkThumb* ioObject)  
: morkHandle(ev, ioFace, ioObject, morkMagic_kThumb)
{
  
}


 orkinThumb*
orkinThumb::MakeThumb(morkEnv* ev, morkThumb* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinThumb));
    if ( face )
      return new(face) orkinThumb(ev, face, ioObject);
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinThumb*) 0;
}

morkEnv*
orkinThumb::CanUseThumb(nsIMdbEnv* mev,
  mork_bool inMutable, mdb_err* outErr) const
{
  morkEnv* outEnv = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkThumb* self = (morkThumb*)
      this->GetGoodHandleObject(ev, inMutable, morkMagic_kThumb,
         morkBool_kFalse);
    if ( self )
    {
      if ( self->IsThumb() )
        outEnv = ev;
      else
        self->NonThumbTypeError(ev);
    }
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv);
  return outEnv;
}



NS_IMPL_QUERY_INTERFACE1(orkinThumb, nsIMdbThumb)

 nsrefcnt
orkinThumb::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinThumb::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}






 mdb_err
orkinThumb::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinThumb::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinThumb::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinThumb::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinThumb::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinThumb::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinThumb::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinThumb::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinThumb::CloseMdbObject(nsIMdbEnv* mev)
{
  return this->Handle_CloseMdbObject(mev);
}

 mdb_err
orkinThumb::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}






 mdb_err
orkinThumb::GetProgress(nsIMdbEnv* mev, mdb_count* outTotal,
  mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseThumb(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ((morkThumb*) mHandle_Object)->GetProgress(ev, outTotal,
      outCurrent, outDone, outBroken);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinThumb::DoMore(nsIMdbEnv* mev, mdb_count* outTotal,
  mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseThumb(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkThumb* thumb = (morkThumb*) mHandle_Object;
    thumb->DoMore(ev, outTotal, outCurrent, outDone, outBroken);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinThumb::CancelAndBreakThumb(nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseThumb(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkThumb* thumb = (morkThumb*) mHandle_Object;
    thumb->mThumb_Done = morkBool_kTrue;
    thumb->mThumb_Broken = morkBool_kTrue;
    thumb->CloseMorkNode(ev); 
    outErr = ev->AsErr();
  }
  return outErr;
}



