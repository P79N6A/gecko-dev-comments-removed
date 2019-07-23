




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif

#ifndef _MORKHANDLE_
#include "morkHandle.h"
#endif

#include "nsCOMPtr.h"




NS_IMPL_ISUPPORTS1(morkObject, nsIMdbObject)




 void
morkObject::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseObject(ev);
    this->MarkShut();
  }
}


morkObject::~morkObject() 
{
  if (!IsShutNode())
    CloseMorkNode(this->mMorkEnv);
  MORK_ASSERT(mObject_Handle==0);
}


morkObject::morkObject(const morkUsage& inUsage, nsIMdbHeap* ioHeap,
  mork_color inBeadColor)
: morkBead(inUsage, ioHeap, inBeadColor)
, mObject_Handle( 0 )
{
  mMorkEnv = nsnull;
}


morkObject::morkObject(morkEnv* ev,
  const morkUsage& inUsage, nsIMdbHeap* ioHeap, 
  mork_color inBeadColor, morkHandle* ioHandle)
: morkBead(ev, inUsage, ioHeap, inBeadColor)
, mObject_Handle( 0 )
{
  mMorkEnv = ev;
  if ( ev->Good() )
  {
    if ( ioHandle )
      morkHandle::SlotWeakHandle(ioHandle, ev, &mObject_Handle);
      
    if ( ev->Good() )
      mNode_Derived = morkDerived_kObject;
  }
}

 void
morkObject::CloseObject(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      if ( !this->IsShutNode() )
      {
        if ( mObject_Handle )
          morkHandle::SlotWeakHandle((morkHandle*) 0L, ev, &mObject_Handle);
          
        mBead_Color = 0; 
        this->MarkShut();
      }
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}





NS_IMETHODIMP
morkObject::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  nsresult rv;
  nsCOMPtr <nsIMdbObject> obj = do_QueryInterface(mev);
  if (obj)
    rv = obj->GetMdbFactory(mev, acqFactory);
  else
    return NS_ERROR_NO_INTERFACE;

  return rv;
} 



NS_IMETHODIMP
morkObject::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  *outCount = WeakRefsOnly();
  return NS_OK;
}  
NS_IMETHODIMP
morkObject::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  *outCount = StrongRefsOnly();
  return NS_OK;
}

NS_IMETHODIMP
morkObject::AddWeakRef(nsIMdbEnv* mev)
{
  return morkNode::AddWeakRef((morkEnv *) mev);
}
NS_IMETHODIMP
morkObject::AddStrongRef(nsIMdbEnv* mev)
{
  return morkNode::AddStrongRef((morkEnv *) mev);
}

NS_IMETHODIMP
morkObject::CutWeakRef(nsIMdbEnv* mev)
{
  return morkNode::CutWeakRef((morkEnv *) mev);
}
NS_IMETHODIMP
morkObject::CutStrongRef(nsIMdbEnv* mev)
{
  return morkNode::CutStrongRef((morkEnv *) mev);
}

  
NS_IMETHODIMP
morkObject::CloseMdbObject(nsIMdbEnv* mev)
{
  return morkNode::CloseMdbObject((morkEnv *) mev);
}

NS_IMETHODIMP
morkObject::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  *outOpen = IsOpenNode();
  return NS_OK;
}
NS_IMETHODIMP
morkObject::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  *outIsReadonly = IsFrozen();
  return NS_OK;
}








