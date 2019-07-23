




































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

#ifndef _ORKINENV_
#include "orkinEnv.h"
#endif

#ifndef _ORKINHEAP_
#include "orkinHeap.h"
#endif




orkinEnv:: ~orkinEnv() 
{
}

void orkinEnv::CloseMorkNode(morkEnv* ev) 
{
  morkEnv* mev = (morkEnv*) this->mHandle_Object;

  if ( mev->IsOpenNode() )
  {
    mev->MarkClosing();
    mev->CloseEnv(ev);
    mev->MarkShut();
  }
  morkHandle::CloseMorkNode(ev);
}


orkinEnv::orkinEnv(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkEnv* ioObject)  
: morkHandle(ev, ioFace, ioObject, morkMagic_kEnv)
{
  
}


 orkinEnv*
orkinEnv::MakeEnv(morkEnv* ev, morkEnv* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinEnv));
    if ( face )
      return new(face) orkinEnv(ev, face, ioObject);
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinEnv*) 0;
}

morkEnv*
orkinEnv::CanUseEnv(mork_bool inMutable, mdb_err* outErr) const
{
  MORK_USED_1(inMutable);
  morkEnv* outEnv = 0;
  mdb_err err = morkEnv_kBadEnvError;
  if ( this->IsHandle() )
  {
    if ( this->IsOpenNode() )
    {
      morkEnv* ev = (morkEnv*) this->mHandle_Object;
      if ( ev && ev->IsEnv() )
      {
        outEnv = ev;
        err = 0;
      }
      else
      {
        err = morkEnv_kNonEnvTypeError;
        MORK_ASSERT(outEnv);
      }
    }
    else
    {
      err = morkEnv_kNonOpenNodeError;
      MORK_ASSERT(outEnv);
    }
  }
  else
  {
    err = morkEnv_kNonHandleTypeError;
    MORK_ASSERT(outEnv);
  }
  *outErr = err;
  return outEnv;
}



NS_IMPL_QUERY_INTERFACE0(orkinEnv)

 nsrefcnt
orkinEnv::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinEnv::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}





 mdb_err
orkinEnv::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinEnv::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinEnv::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinEnv::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinEnv::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinEnv::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinEnv::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinEnv::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinEnv::CloseMdbObject(nsIMdbEnv* mev)
{
  morkEnv* ev = (morkEnv*) this->mHandle_Object;
  mdb_err ret = this->Handle_CloseMdbObject(mev);
  if (ev && ev->mEnv_Heap)
  {
    mork_bool ownsHeap = ev->mEnv_OwnsHeap;
    nsIMdbHeap*saveHeap = ev->mEnv_Heap;

    ev->mEnv_Heap->Free(this, ev);
    if (ownsHeap)
    {
#ifdef MORK_DEBUG_HEAP_STATS
      printf("%d blocks remaining \n", ((orkinHeap *) saveHeap)->HeapBlockCount());
#endif 
      delete saveHeap;
    }

  }
  return ret;
}

 mdb_err
orkinEnv::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}







 mdb_err
orkinEnv::GetErrorCount(mdb_count* outCount,
  mdb_bool* outShouldAbort)
{
  mdb_err outErr = 0;
  mdb_count count = 1;
  mork_bool shouldAbort = morkBool_kFalse;
  morkEnv* ev = this->CanUseEnv( morkBool_kFalse, &outErr);
  if ( ev )
  {
    count = (mdb_count) ev->mEnv_ErrorCount;
    shouldAbort = ev->mEnv_ShouldAbort;
  }
  if ( outCount )
    *outCount = count;
  if ( outShouldAbort )
    *outShouldAbort = shouldAbort;
  return outErr;
}

 mdb_err
orkinEnv::GetWarningCount(mdb_count* outCount,
  mdb_bool* outShouldAbort)
{
  mdb_err outErr = 0;
  mdb_count count = 1;
  mork_bool shouldAbort = morkBool_kFalse;
  morkEnv* ev = this->CanUseEnv( morkBool_kFalse, &outErr);
  if ( ev )
  {
    count = (mdb_count) ev->mEnv_WarningCount;
    shouldAbort = ev->mEnv_ShouldAbort;
  }
  if ( outCount )
    *outCount = count;
  if ( outShouldAbort )
    *outShouldAbort = shouldAbort;
  return outErr;
}

 mdb_err
orkinEnv::GetEnvBeVerbose(mdb_bool* outBeVerbose)
{
  mdb_err outErr = 0;
  mork_bool beVerbose = morkBool_kFalse;
  morkEnv* ev = this->CanUseEnv( morkBool_kFalse, &outErr);
  if ( ev )
  {
    beVerbose = ev->mEnv_BeVerbose;
  }
  if ( outBeVerbose )
    *outBeVerbose = beVerbose;
  return outErr;
}

 mdb_err
orkinEnv::SetEnvBeVerbose(mdb_bool inBeVerbose)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseEnv( morkBool_kTrue, &outErr);
  if ( ev )
  {
    ev->mEnv_BeVerbose = inBeVerbose;
  }
  return outErr;
}

 mdb_err
orkinEnv::GetDoTrace(mdb_bool* outDoTrace)
{
  mdb_err outErr = 0;
  mork_bool doTrace = morkBool_kFalse;
  morkEnv* ev = this->CanUseEnv( morkBool_kFalse, &outErr);
  if ( ev )
  {
    doTrace = ev->mEnv_DoTrace;
  }
  if ( outDoTrace )
    *outDoTrace = doTrace;
  return outErr;
}

 mdb_err
orkinEnv::SetDoTrace(mdb_bool inDoTrace)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseEnv( morkBool_kTrue, &outErr);
  if ( ev )
  {
    ev->mEnv_DoTrace = inDoTrace;
  }
  return outErr;
}

 mdb_err
orkinEnv::GetAutoClear(mdb_bool* outAutoClear)
{
  mdb_err outErr = 0;
  mork_bool autoClear = morkBool_kFalse;
  morkEnv* ev = this->CanUseEnv( morkBool_kFalse, &outErr);
  if ( ev )
  {
    autoClear = ev->DoAutoClear();
  }
  if ( outAutoClear )
    *outAutoClear = autoClear;
  return outErr;
}

 mdb_err
orkinEnv::SetAutoClear(mdb_bool inAutoClear)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseEnv( morkBool_kTrue, &outErr);
  if ( ev )
  {
    if ( inAutoClear )
      ev->EnableAutoClear();
    else
      ev->DisableAutoClear();
  }
  return outErr;
}

 mdb_err
orkinEnv::GetErrorHook(nsIMdbErrorHook** acqErrorHook)
{
  mdb_err outErr = 0;
  nsIMdbErrorHook* outErrorHook = 0;
  morkEnv* ev = this->CanUseEnv( morkBool_kFalse, &outErr);
  if ( ev )
  {
    outErrorHook = ev->mEnv_ErrorHook;
  }
  if ( acqErrorHook )
    *acqErrorHook = outErrorHook;
  return outErr;
}

 mdb_err
orkinEnv::SetErrorHook(
  nsIMdbErrorHook* ioErrorHook) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseEnv( morkBool_kTrue, &outErr);
  if ( ev )
  {
    ev->mEnv_ErrorHook = ioErrorHook;
  }
  return outErr;
}

 mdb_err
orkinEnv::GetHeap(nsIMdbHeap** acqHeap)
{
  mdb_err outErr = 0;
  nsIMdbHeap* outHeap = 0;
  morkEnv* ev = this->CanUseEnv( morkBool_kFalse, &outErr);
  if ( ev )
  {
    nsIMdbHeap* heap = ev->mEnv_Heap;
    if ( heap && heap->HeapAddStrongRef(this) == 0 )
      outHeap = heap;
  }
  if ( acqHeap )
    *acqHeap = outHeap;
  return outErr;
}

 mdb_err
orkinEnv::SetHeap(
  nsIMdbHeap* ioHeap) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseEnv( morkBool_kTrue, &outErr);
  if ( ev )
  {
    nsIMdbHeap_SlotStrongHeap(ioHeap, ev, &ev->mEnv_Heap);
  }
  return outErr;
}


 mdb_err
orkinEnv::ClearErrors() 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseEnv( morkBool_kTrue, &outErr);
  if ( ev )
  {
    ev->mEnv_ErrorCount = 0;
    ev->mEnv_ErrorCode = 0;
    ev->mEnv_ShouldAbort = morkBool_kFalse;
  }
  return outErr;
}

 mdb_err
orkinEnv::ClearWarnings() 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseEnv( morkBool_kTrue, &outErr);
  if ( ev )
  {
    ev->mEnv_WarningCount = 0;
  }
  return outErr;
}

 mdb_err
orkinEnv::ClearErrorsAndWarnings() 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseEnv( morkBool_kTrue, &outErr);
  if ( ev )
  {
    ev->ClearMorkErrorsAndWarnings();
  }
  return outErr;
}




