




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKSPACE_
#include "morkSpace.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif






 void
morkSpace::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseSpace(ev);
    this->MarkShut();
  }
}


morkSpace::~morkSpace() 
{
  MORK_ASSERT(SpaceScope()==0);
  MORK_ASSERT(mSpace_Store==0);
  MORK_ASSERT(this->IsShutNode());
}    












morkSpace::morkSpace(morkEnv* ev,
  const morkUsage& inUsage, mork_scope inScope, morkStore* ioStore,
  nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
: morkBead(ev, inUsage, ioHeap, inScope)
, mSpace_Store( 0 )
, mSpace_DoAutoIDs( morkBool_kFalse )
, mSpace_HaveDoneAutoIDs( morkBool_kFalse )
, mSpace_CanDirty( morkBool_kFalse ) 
{
  if ( ev->Good() )
  {
    if ( ioStore && ioSlotHeap )
    {
      morkStore::SlotWeakStore(ioStore, ev, &mSpace_Store);

      mSpace_CanDirty = ioStore->mStore_CanDirty;
      if ( mSpace_CanDirty ) 
        this->MaybeDirtyStoreAndSpace();
        
      if ( ev->Good() )
        mNode_Derived = morkDerived_kSpace;
    }
    else
      ev->NilPointerError();
  }
}

 void
morkSpace::CloseSpace(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      morkStore::SlotWeakStore((morkStore*) 0, ev, &mSpace_Store);
      mBead_Color = 0; 
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void 
morkSpace::NonAsciiSpaceScopeName(morkEnv* ev)
{
  ev->NewError("SpaceScope() > 0x7F");
}

 void 
morkSpace::NilSpaceStoreError(morkEnv* ev)
{
  ev->NewError("nil mSpace_Store");
}

morkPool* morkSpace::GetSpaceStorePool() const
{
  return &mSpace_Store->mStore_Pool;
}

mork_bool morkSpace::MaybeDirtyStoreAndSpace()
{
  morkStore* store = mSpace_Store;
  if ( store && store->mStore_CanDirty )
  {
    store->SetStoreDirty();
    mSpace_CanDirty = morkBool_kTrue;
  }
  
  if ( mSpace_CanDirty )
  {
    this->SetSpaceDirty();
    return morkBool_kTrue;
  }
  
  return morkBool_kFalse;
}



