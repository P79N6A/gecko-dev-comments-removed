




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKFACTORY_
#include "morkFactory.h"
#endif

#ifndef _MORKPOOL_
#include "morkPool.h"
#endif

#ifndef _MORKHANDLE_
#include "morkHandle.h"
#endif






 void
morkHandle::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseHandle(ev);
    this->MarkShut();
  }
}


morkHandle::~morkHandle() 
{
  MORK_ASSERT(mHandle_Env==0);
  MORK_ASSERT(mHandle_Face==0);
  MORK_ASSERT(mHandle_Object==0);
  MORK_ASSERT(mHandle_Magic==0);
  MORK_ASSERT(mHandle_Tag==morkHandle_kTag); 
}


morkHandle::morkHandle(morkEnv* ev, 
    morkHandleFace* ioFace,  
    morkObject* ioObject,    
    mork_magic inMagic)      
: morkNode(ev, morkUsage::kPool, (nsIMdbHeap*) 0L)
, mHandle_Tag( 0 )
, mHandle_Env( ev )
, mHandle_Face( ioFace )
, mHandle_Object( 0 )
, mHandle_Magic( 0 )
{
  if ( ioFace && ioObject )
  {
    if ( ev->Good() )
    {
      mHandle_Tag = morkHandle_kTag;
      morkObject::SlotStrongObject(ioObject, ev, &mHandle_Object);
      morkHandle::SlotWeakHandle(this, ev, &ioObject->mObject_Handle);
      if ( ev->Good() )
      {
        mHandle_Magic = inMagic;
        mNode_Derived = morkDerived_kHandle;
      }
    }
    else
      ev->CantMakeWhenBadError();
  }
  else
    ev->NilPointerError();
}

 void
morkHandle::CloseHandle(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      morkObject* obj = mHandle_Object;
      mork_bool objDidRefSelf = ( obj && obj->mObject_Handle == this );
      if ( objDidRefSelf )
        obj->mObject_Handle = 0; 
      
      morkObject::SlotStrongObject((morkObject*) 0, ev, &mHandle_Object);
      mHandle_Magic = 0;
      
      this->MarkShut();

      if ( objDidRefSelf )
        this->CutWeakRef(ev); 
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




void morkHandle::NilFactoryError(morkEnv* ev) const
{
  ev->NewError("nil mHandle_Factory");
}
  
void morkHandle::NilHandleObjectError(morkEnv* ev) const
{
  ev->NewError("nil mHandle_Object");
}
  
void morkHandle::NonNodeObjectError(morkEnv* ev) const
{
  ev->NewError("non-node mHandle_Object");
}
  
void morkHandle::NonOpenObjectError(morkEnv* ev) const
{
  ev->NewError("non-open mHandle_Object");
}
  
void morkHandle::NewBadMagicHandleError(morkEnv* ev, mork_magic inMagic) const
{
  MORK_USED_1(inMagic);
  ev->NewError("wrong mHandle_Magic");
}

void morkHandle::NewDownHandleError(morkEnv* ev) const
{
  if ( this->IsHandle() )
  {
    if ( this->GoodHandleTag() )
    {
      if ( this->IsOpenNode() )
        ev->NewError("unknown down morkHandle error");
      else
        this->NonOpenNodeError(ev);
    }
    else
      ev->NewError("wrong morkHandle tag");
  }
  else
    ev->NewError("non morkHandle");
}

morkObject* morkHandle::GetGoodHandleObject(morkEnv* ev,
  mork_bool inMutable, mork_magic inMagicType, mork_bool inClosedOkay) const
{
  morkObject* outObject = 0;
  if ( this->IsHandle() && this->GoodHandleTag() &&
    ( inClosedOkay || this->IsOpenNode() ) )
  {
    if ( !inMagicType || mHandle_Magic == inMagicType )
    {
      morkObject* obj = this->mHandle_Object;
      if ( obj )
      {
        if ( obj->IsNode() )
        {
          if ( inClosedOkay || obj->IsOpenNode() )
          {
            if ( this->IsMutable() || !inMutable )
              outObject = obj;
            else
              this->NonMutableNodeError(ev);
          }
          else
            this->NonOpenObjectError(ev);
        }
        else
          this->NonNodeObjectError(ev);
      }
      else if ( !inClosedOkay )
        this->NilHandleObjectError(ev);
    }
    else
      this->NewBadMagicHandleError(ev, inMagicType);
  }
  else
    this->NewDownHandleError(ev);
  
  MORK_ASSERT(outObject || inClosedOkay);
  return outObject;
}


morkEnv*
morkHandle::CanUseHandle(nsIMdbEnv* mev, mork_bool inMutable,
  mork_bool inClosedOkay, mdb_err* outErr) const
{
  morkEnv* outEnv = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkObject* obj = this->GetGoodHandleObject(ev, inMutable,
       0, inClosedOkay);
    if ( obj )
    {
      outEnv = ev;
    }
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv || inClosedOkay);
  return outEnv;
}




 mdb_err
morkHandle::Handle_IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  mdb_err outErr = 0;
  mdb_bool readOnly = mdbBool_kTrue;
  
  morkEnv* ev = CanUseHandle(mev,  morkBool_kFalse,
     morkBool_kTrue, &outErr);
  if ( ev )
  {
    readOnly = mHandle_Object->IsFrozen();
    
    outErr = ev->AsErr();
  }
  MORK_ASSERT(outIsReadonly);
  if ( outIsReadonly )
    *outIsReadonly = readOnly;

  return outErr;
}




 mdb_err
morkHandle::Handle_GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  mdb_err outErr = 0;
  nsIMdbFactory* handle = 0;
  
  morkEnv* ev = CanUseHandle(mev,  morkBool_kFalse,
     morkBool_kTrue, &outErr);
  if ( ev )
  {
    morkFactory* factory = ev->mEnv_Factory;
    if ( factory )
    {
      handle = factory;
      NS_ADDREF(handle);
    }
    else
      this->NilFactoryError(ev);
      
    outErr = ev->AsErr();
  }

  MORK_ASSERT(acqFactory);
  if ( acqFactory )
    *acqFactory = handle;

  return outErr;
} 



 mdb_err
morkHandle::Handle_GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  mdb_err outErr = 0;
  mdb_count count = 0;
  
  morkEnv* ev = CanUseHandle(mev,  morkBool_kFalse,
     morkBool_kTrue, &outErr);
  if ( ev )
  {
    count = this->WeakRefsOnly();
    
    outErr = ev->AsErr();
  }
  MORK_ASSERT(outCount);
  if ( outCount )
    *outCount = count;
    
  return outErr;
}  
 mdb_err
morkHandle::Handle_GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  mdb_err outErr = 0;
  mdb_count count = 0;
  
  morkEnv* ev = CanUseHandle(mev,  morkBool_kFalse,
     morkBool_kTrue, &outErr);
  if ( ev )
  {
    count = this->StrongRefsOnly();
    
    outErr = ev->AsErr();
  }
  MORK_ASSERT(outCount);
  if ( outCount )
    *outCount = count;
    
  return outErr;
}

 mdb_err
morkHandle::Handle_AddWeakRef(nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  
  morkEnv* ev = CanUseHandle(mev,  morkBool_kFalse,
     morkBool_kTrue, &outErr);
  if ( ev )
  {
    this->AddWeakRef(ev);
    outErr = ev->AsErr();
  }
    
  return outErr;
}
 mdb_err
morkHandle::Handle_AddStrongRef(nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  
  morkEnv* ev = CanUseHandle(mev,  morkBool_kFalse,
     morkBool_kFalse, &outErr);
  if ( ev )
  {
    this->AddStrongRef(ev);
    outErr = ev->AsErr();
  }
    
  return outErr;
}

 mdb_err
morkHandle::Handle_CutWeakRef(nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  
  morkEnv* ev = CanUseHandle(mev,  morkBool_kFalse,
     morkBool_kTrue, &outErr);
  if ( ev )
  {
    this->CutWeakRef(ev);
    outErr = ev->AsErr();
  }
    
  return outErr;
}
 mdb_err
morkHandle::Handle_CutStrongRef(nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  morkEnv* ev = CanUseHandle(mev,  morkBool_kFalse,
     morkBool_kTrue, &outErr);
  if ( ev )
  {
    this->CutStrongRef(ev);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
morkHandle::Handle_CloseMdbObject(nsIMdbEnv* mev)

{
  
  if (mNode_Uses == 1)
    return Handle_CutStrongRef(mev);

  mdb_err outErr = 0;
  
  if ( this->IsNode() && this->IsOpenNode() )
  {
    morkEnv* ev = CanUseHandle(mev,  morkBool_kFalse,
     morkBool_kTrue, &outErr);
    if ( ev )
    {
      morkObject* object = mHandle_Object;
      if ( object && object->IsNode() && object->IsOpenNode() )
        object->CloseMorkNode(ev);
        
      this->CloseMorkNode(ev);
      outErr = ev->AsErr();
    }
  }
  return outErr;
}

 mdb_err
morkHandle::Handle_IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  MORK_USED_1(mev);
  mdb_err outErr = 0;
  
  MORK_ASSERT(outOpen);
  if ( outOpen )
    *outOpen = this->IsOpenNode();
      
  return outErr;
}






