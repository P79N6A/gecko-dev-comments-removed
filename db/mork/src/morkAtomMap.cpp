




































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

#ifndef _MORKATOMMAP_
#include "morkAtomMap.h"
#endif

#ifndef _MORKATOM_
#include "morkAtom.h"
#endif

#ifndef _MORKINTMAP_
#include "morkIntMap.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif






 void
morkAtomAidMap::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseAtomAidMap(ev);
    this->MarkShut();
  }
}


morkAtomAidMap::~morkAtomAidMap() 
{
  MORK_ASSERT(this->IsShutNode());
}



morkAtomAidMap::morkAtomAidMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
#ifdef MORK_ENABLE_PROBE_MAPS
: morkProbeMap(ev, inUsage,  ioHeap,
   sizeof(morkBookAtom*),  0,
  ioSlotHeap, morkAtomAidMap_kStartSlotCount, 
   morkBool_kTrue)
#else 
: morkMap(ev, inUsage,  ioHeap,
   sizeof(morkBookAtom*),  0,
  morkAtomAidMap_kStartSlotCount, ioSlotHeap,
   morkBool_kFalse)
#endif 
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kAtomAidMap;
}

 void
morkAtomAidMap::CloseAtomAidMap(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
#ifdef MORK_ENABLE_PROBE_MAPS
      this->CloseProbeMap(ev);
#else 
      this->CloseMap(ev);
#endif 
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




#ifdef MORK_ENABLE_PROBE_MAPS

   mork_test 
  morkAtomAidMap::MapTest(morkEnv* ev, const void* inMapKey,
    const void* inAppKey) const
  {
    MORK_USED_1(ev);
    const morkBookAtom* key = *(const morkBookAtom**) inMapKey;
    if ( key )
    {
      mork_bool hit = key->EqualAid(*(const morkBookAtom**) inAppKey);
      return ( hit ) ? morkTest_kHit : morkTest_kMiss;
    }
    else
      return morkTest_kVoid;
  }

   mork_u4 
  morkAtomAidMap::MapHash(morkEnv* ev, const void* inAppKey) const
  {
    const morkBookAtom* key = *(const morkBookAtom**) inAppKey;
    if ( key )
      return key->HashAid();
    else
    {
      ev->NilPointerWarning();
      return 0;
    }
  }

   mork_u4 
  morkAtomAidMap::ProbeMapHashMapKey(morkEnv* ev,
    const void* inMapKey) const
  {
    const morkBookAtom* key = *(const morkBookAtom**) inMapKey;
    if ( key )
      return key->HashAid();
    else
    {
      ev->NilPointerWarning();
      return 0;
    }
  }
#else 
  
   mork_bool 
  morkAtomAidMap::Equal(morkEnv* ev, const void* inKeyA,
    const void* inKeyB) const
  {
    MORK_USED_1(ev);
    return (*(const morkBookAtom**) inKeyA)->EqualAid(
      *(const morkBookAtom**) inKeyB);
  }

   mork_u4 
  morkAtomAidMap::Hash(morkEnv* ev, const void* inKey) const
  {
    MORK_USED_1(ev);
    return (*(const morkBookAtom**) inKey)->HashAid();
  }
  
#endif 


mork_bool
morkAtomAidMap::AddAtom(morkEnv* ev, morkBookAtom* ioAtom)
{
  if ( ev->Good() )
  {
#ifdef MORK_ENABLE_PROBE_MAPS
    this->MapAtPut(ev, &ioAtom,  (void*) 0, 
       (void*) 0,  (void*) 0);
#else 
    this->Put(ev, &ioAtom,  (void*) 0, 
       (void*) 0,  (void*) 0, (mork_change**) 0);
#endif 
  }
  return ev->Good();
}

morkBookAtom*
morkAtomAidMap::CutAtom(morkEnv* ev, const morkBookAtom* inAtom)
{
  morkBookAtom* oldKey = 0;
  
#ifdef MORK_ENABLE_PROBE_MAPS
  MORK_USED_1(inAtom);
  morkProbeMap::ProbeMapCutError(ev);
#else 
  this->Cut(ev, &inAtom, &oldKey,  (void*) 0,
    (mork_change**) 0);
#endif 
    
  return oldKey;
}

morkBookAtom*
morkAtomAidMap::GetAtom(morkEnv* ev, const morkBookAtom* inAtom)
{
  morkBookAtom* key = 0; 

#ifdef MORK_ENABLE_PROBE_MAPS
  this->MapAt(ev, &inAtom, &key,  (void*) 0);
#else 
  this->Get(ev, &inAtom, &key,  (void*) 0, (mork_change**) 0);
#endif 
  
  return key;
}

morkBookAtom*
morkAtomAidMap::GetAid(morkEnv* ev, mork_aid inAid)
{
  morkWeeBookAtom weeAtom(inAid);
  morkBookAtom* key = &weeAtom; 
  morkBookAtom* oldKey = 0; 

#ifdef MORK_ENABLE_PROBE_MAPS
  this->MapAt(ev, &key, &oldKey,  (void*) 0);
#else 
  this->Get(ev, &key, &oldKey,  (void*) 0, (mork_change**) 0);
#endif 
  
  return oldKey;
}









 void
morkAtomBodyMap::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseAtomBodyMap(ev);
    this->MarkShut();
  }
}


morkAtomBodyMap::~morkAtomBodyMap() 
{
  MORK_ASSERT(this->IsShutNode());
}



morkAtomBodyMap::morkAtomBodyMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
#ifdef MORK_ENABLE_PROBE_MAPS
: morkProbeMap(ev, inUsage,  ioHeap,
   sizeof(morkBookAtom*),  0,
  ioSlotHeap, morkAtomBodyMap_kStartSlotCount, 
   morkBool_kTrue)
#else 
: morkMap(ev, inUsage,  ioHeap,
   sizeof(morkBookAtom*),  0,
  morkAtomBodyMap_kStartSlotCount, ioSlotHeap,
   morkBool_kFalse)
#endif 
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kAtomBodyMap;
}

 void
morkAtomBodyMap::CloseAtomBodyMap(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
#ifdef MORK_ENABLE_PROBE_MAPS
      this->CloseProbeMap(ev);
#else 
      this->CloseMap(ev);
#endif 
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}



#ifdef MORK_ENABLE_PROBE_MAPS

   mork_test 
  morkAtomBodyMap::MapTest(morkEnv* ev, const void* inMapKey,
    const void* inAppKey) const
  {
    const morkBookAtom* key = *(const morkBookAtom**) inMapKey;
    if ( key )
    {
      return ( key->EqualFormAndBody(ev, *(const morkBookAtom**) inAppKey) ) ?
        morkTest_kHit : morkTest_kMiss;
    }
    else
      return morkTest_kVoid;
  }

   mork_u4 
  morkAtomBodyMap::MapHash(morkEnv* ev, const void* inAppKey) const
  {
    const morkBookAtom* key = *(const morkBookAtom**) inAppKey;
    if ( key )
      return key->HashFormAndBody(ev);
    else
      return 0;
  }

   mork_u4 
  morkAtomBodyMap::ProbeMapHashMapKey(morkEnv* ev, const void* inMapKey) const
  {
    const morkBookAtom* key = *(const morkBookAtom**) inMapKey;
    if ( key )
      return key->HashFormAndBody(ev);
    else
      return 0;
  }
#else 
  
   mork_bool 
  morkAtomBodyMap::Equal(morkEnv* ev, const void* inKeyA,
    const void* inKeyB) const
  {
    return (*(const morkBookAtom**) inKeyA)->EqualFormAndBody(ev,
      *(const morkBookAtom**) inKeyB);
  }

   mork_u4 
  morkAtomBodyMap::Hash(morkEnv* ev, const void* inKey) const
  {
    return (*(const morkBookAtom**) inKey)->HashFormAndBody(ev);
  }
  
#endif 


mork_bool
morkAtomBodyMap::AddAtom(morkEnv* ev, morkBookAtom* ioAtom)
{
  if ( ev->Good() )
  {
#ifdef MORK_ENABLE_PROBE_MAPS
    this->MapAtPut(ev, &ioAtom,  (void*) 0, 
       (void*) 0,  (void*) 0);
#else 
    this->Put(ev, &ioAtom,  (void*) 0, 
       (void*) 0,  (void*) 0, (mork_change**) 0);
#endif 
  }
  return ev->Good();
}

morkBookAtom*
morkAtomBodyMap::CutAtom(morkEnv* ev, const morkBookAtom* inAtom)
{
  morkBookAtom* oldKey = 0;

#ifdef MORK_ENABLE_PROBE_MAPS
  MORK_USED_1(inAtom);
  morkProbeMap::ProbeMapCutError(ev);
#else 
  this->Cut(ev, &inAtom, &oldKey,  (void*) 0,
    (mork_change**) 0);
#endif 
    
  return oldKey;
}

morkBookAtom*
morkAtomBodyMap::GetAtom(morkEnv* ev, const morkBookAtom* inAtom)
{
  morkBookAtom* key = 0; 
#ifdef MORK_ENABLE_PROBE_MAPS
  this->MapAt(ev, &inAtom, &key,  (void*) 0);
#else 
  this->Get(ev, &inAtom, &key,  (void*) 0, (mork_change**) 0);
#endif 
  
  return key;
}



morkAtomRowMap::~morkAtomRowMap()
{
}





morkAtomRowMap::morkAtomRowMap(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap, mork_column inIndexColumn)
  : morkIntMap(ev, inUsage, sizeof(mork_ip), ioHeap, ioSlotHeap,
     morkBool_kFalse)
, mAtomRowMap_IndexColumn( inIndexColumn )
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kAtomRowMap;
}

void morkAtomRowMap::AddRow(morkEnv* ev, morkRow* ioRow)

{
  mork_aid aid = ioRow->GetCellAtomAid(ev, mAtomRowMap_IndexColumn);
  if ( aid )
    this->AddAid(ev, aid, ioRow);
}

void morkAtomRowMap::CutRow(morkEnv* ev, morkRow* ioRow)

{
  mork_aid aid = ioRow->GetCellAtomAid(ev, mAtomRowMap_IndexColumn);
  if ( aid )
    this->CutAid(ev, aid);
}


