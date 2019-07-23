




































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

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKINTMAP_
#include "morkIntMap.h"
#endif






 void
morkIntMap::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseIntMap(ev);
    this->MarkShut();
  }
}


morkIntMap::~morkIntMap() 
{
  MORK_ASSERT(this->IsShutNode());
}


morkIntMap::morkIntMap(morkEnv* ev,
  const morkUsage& inUsage, mork_size inValSize,
  nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap, mork_bool inHoldChanges)
: morkMap(ev, inUsage, ioHeap, sizeof(mork_u4), inValSize,
  morkIntMap_kStartSlotCount, ioSlotHeap, inHoldChanges)
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kIntMap;
}

 void
morkIntMap::CloseIntMap(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      this->CloseMap(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}





 mork_bool 
morkIntMap::Equal(morkEnv* ev, const void* inKeyA, const void* inKeyB) const
{
  MORK_USED_1(ev);
  return *((const mork_u4*) inKeyA) == *((const mork_u4*) inKeyB);
}

 mork_u4 
morkIntMap::Hash(morkEnv* ev, const void* inKey) const
{
  MORK_USED_1(ev);
  return *((const mork_u4*) inKey);
}


mork_bool
morkIntMap::AddInt(morkEnv* ev, mork_u4 inKey, void* ioAddress)
  
{
  if ( ev->Good() )
  {
    this->Put(ev, &inKey, &ioAddress, 
       (void*) 0,  (void*) 0, (mork_change**) 0);
  }
    
  return ev->Good();
}

mork_bool
morkIntMap::CutInt(morkEnv* ev, mork_u4 inKey)
{
  return this->Cut(ev, &inKey,  (void*) 0,  (void*) 0,
    (mork_change**) 0);
}

void*
morkIntMap::GetInt(morkEnv* ev, mork_u4 inKey)
  
{
  void* val = 0; 
  this->Get(ev, &inKey,  (void*) 0, &val, (mork_change**) 0);
  
  return val;
}

mork_bool
morkIntMap::HasInt(morkEnv* ev, mork_u4 inKey)
  
{
  return this->Get(ev, &inKey,  (void*) 0,  (void*) 0, 
    (mork_change**) 0);
}



#ifdef MORK_POINTER_MAP_IMPL




 void
morkPointerMap::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->ClosePointerMap(ev);
    this->MarkShut();
  }
}


morkPointerMap::~morkPointerMap() 
{
  MORK_ASSERT(this->IsShutNode());
}


morkPointerMap::morkPointerMap(morkEnv* ev,
  const morkUsage& inUsage, nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
: morkMap(ev, inUsage, ioHeap, sizeof(void*), sizeof(void*),
  morkPointerMap_kStartSlotCount, ioSlotHeap,
   morkBool_kFalse)
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kPointerMap;
}

 void
morkPointerMap::ClosePointerMap(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      this->CloseMap(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}





 mork_bool 
morkPointerMap::Equal(morkEnv* ev, const void* inKeyA, const void* inKeyB) const
{
  MORK_USED_1(ev);
  return *((const void**) inKeyA) == *((const void**) inKeyB);
}

 mork_u4 
morkPointerMap::Hash(morkEnv* ev, const void* inKey) const
{
  MORK_USED_1(ev);
  return *((const mork_u4*) inKey);
}


mork_bool
morkPointerMap::AddPointer(morkEnv* ev, void* inKey, void* ioAddress)
  
{
  if ( ev->Good() )
  {
    this->Put(ev, &inKey, &ioAddress, 
       (void*) 0,  (void*) 0, (mork_change**) 0);
  }
    
  return ev->Good();
}

mork_bool
morkPointerMap::CutPointer(morkEnv* ev, void* inKey)
{
  return this->Cut(ev, &inKey,  (void*) 0,  (void*) 0,
    (mork_change**) 0);
}

void*
morkPointerMap::GetPointer(morkEnv* ev, void* inKey)
  
{
  void* val = 0; 
  this->Get(ev, &inKey,  (void*) 0, &val, (mork_change**) 0);
  
  return val;
}

mork_bool
morkPointerMap::HasPointer(morkEnv* ev, void* inKey)
  
{
  return this->Get(ev, &inKey,  (void*) 0,  (void*) 0, 
    (mork_change**) 0);
}
#endif 



