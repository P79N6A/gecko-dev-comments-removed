




































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

#ifndef _MORKROWMAP_
#include "morkRowMap.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif






 void
morkRowMap::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseRowMap(ev);
    this->MarkShut();
  }
}


morkRowMap::~morkRowMap() 
{
  MORK_ASSERT(this->IsShutNode());
}


morkRowMap::morkRowMap(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap, mork_size inSlots)
: morkMap(ev, inUsage,  ioHeap,
   sizeof(morkRow*),  0,
  inSlots, ioSlotHeap,  morkBool_kFalse)
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kRowMap;
}

 void
morkRowMap::CloseRowMap(morkEnv* ev) 
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
morkRowMap::Equal(morkEnv* ev, const void* inKeyA,
  const void* inKeyB) const
{
  MORK_USED_1(ev);
  return (*(const morkRow**) inKeyA)->EqualRow(*(const morkRow**) inKeyB);
}

 mork_u4 
morkRowMap::Hash(morkEnv* ev, const void* inKey) const
{
  MORK_USED_1(ev);
  return (*(const morkRow**) inKey)->HashRow();
}



mork_bool
morkRowMap::AddRow(morkEnv* ev, morkRow* ioRow)
{
  if ( ev->Good() )
  {
    this->Put(ev, &ioRow,  (void*) 0, 
       (void*) 0,  (void*) 0, (mork_change**) 0);
  }
  return ev->Good();
}

morkRow*
morkRowMap::CutOid(morkEnv* ev, const mdbOid* inOid)
{
  morkRow row(inOid);
  morkRow* key = &row;
  morkRow* oldKey = 0;
  this->Cut(ev, &key, &oldKey,  (void*) 0,
    (mork_change**) 0);
    
  return oldKey;
}

morkRow*
morkRowMap::CutRow(morkEnv* ev, const morkRow* ioRow)
{
  morkRow* oldKey = 0;
  this->Cut(ev, &ioRow, &oldKey,  (void*) 0,
    (mork_change**) 0);
    
  return oldKey;
}

morkRow*
morkRowMap::GetOid(morkEnv* ev, const mdbOid* inOid)
{
  morkRow row(inOid);
  morkRow* key = &row;
  morkRow* oldKey = 0;
  this->Get(ev, &key, &oldKey,  (void*) 0, (mork_change**) 0);
  
  return oldKey;
}

morkRow*
morkRowMap::GetRow(morkEnv* ev, const morkRow* ioRow)
{
  morkRow* oldKey = 0;
  this->Get(ev, &ioRow, &oldKey,  (void*) 0, (mork_change**) 0);
  
  return oldKey;
}








 void
morkRowProbeMap::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseRowProbeMap(ev);
    this->MarkShut();
  }
}


morkRowProbeMap::~morkRowProbeMap() 
{
  MORK_ASSERT(this->IsShutNode());
}


morkRowProbeMap::morkRowProbeMap(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap, mork_size inSlots)
: morkProbeMap(ev, inUsage,  ioHeap,
   sizeof(morkRow*),  0,
  ioSlotHeap, inSlots, 
   morkBool_kTrue)
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kRowProbeMap;
}

 void
morkRowProbeMap::CloseRowProbeMap(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      this->CloseProbeMap(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 mork_test 
morkRowProbeMap::MapTest(morkEnv* ev, const void* inMapKey,
  const void* inAppKey) const
{
  MORK_USED_1(ev);
  const morkRow* key = *(const morkRow**) inMapKey;
  if ( key )
  {
    mork_bool hit = key->EqualRow(*(const morkRow**) inAppKey);
    return ( hit ) ? morkTest_kHit : morkTest_kMiss;
  }
  else
    return morkTest_kVoid;
}

 mork_u4 
morkRowProbeMap::MapHash(morkEnv* ev, const void* inAppKey) const
{
  const morkRow* key = *(const morkRow**) inAppKey;
  if ( key )
    return key->HashRow();
  else
  {
    ev->NilPointerWarning();
    return 0;
  }
}

 mork_u4 
morkRowProbeMap::ProbeMapHashMapKey(morkEnv* ev,
  const void* inMapKey) const
{
  const morkRow* key = *(const morkRow**) inMapKey;
  if ( key )
    return key->HashRow();
  else
  {
    ev->NilPointerWarning();
    return 0;
  }
}

mork_bool
morkRowProbeMap::AddRow(morkEnv* ev, morkRow* ioRow)
{
  if ( ev->Good() )
  {
    this->MapAtPut(ev, &ioRow,  (void*) 0, 
       (void*) 0,  (void*) 0);
  }
  return ev->Good();
}

morkRow*
morkRowProbeMap::CutOid(morkEnv* ev, const mdbOid* inOid)
{
  MORK_USED_1(inOid);
  morkProbeMap::ProbeMapCutError(ev);
    
  return 0;
}

morkRow*
morkRowProbeMap::CutRow(morkEnv* ev, const morkRow* ioRow)
{
  MORK_USED_1(ioRow);
  morkProbeMap::ProbeMapCutError(ev);
    
  return 0;
}

morkRow*
morkRowProbeMap::GetOid(morkEnv* ev, const mdbOid* inOid)
{
  morkRow row(inOid);
  morkRow* key = &row;
  morkRow* oldKey = 0;
  this->MapAt(ev, &key, &oldKey,  (void*) 0);
  
  return oldKey;
}

morkRow*
morkRowProbeMap::GetRow(morkEnv* ev, const morkRow* ioRow)
{
  morkRow* oldKey = 0;
  this->MapAt(ev, &ioRow, &oldKey,  (void*) 0);
  
  return oldKey;
}



