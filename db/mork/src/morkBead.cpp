




































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

#ifndef _MORKBEAD_
#include "morkBead.h"
#endif






 void
morkBead::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseBead(ev);
    this->MarkShut();
  }
}


morkBead::~morkBead() 
{
  MORK_ASSERT(mBead_Color==0 || mNode_Usage == morkUsage_kStack );
}


morkBead::morkBead(mork_color inBeadColor)
: morkNode( morkUsage_kStack )
, mBead_Color( inBeadColor )
{
}


morkBead::morkBead(const morkUsage& inUsage, nsIMdbHeap* ioHeap, 
  mork_color inBeadColor)
: morkNode( inUsage, ioHeap )
, mBead_Color( inBeadColor )
{
}


morkBead::morkBead(morkEnv* ev,
  const morkUsage& inUsage, nsIMdbHeap* ioHeap, mork_color inBeadColor)
: morkNode(ev, inUsage, ioHeap)
, mBead_Color( inBeadColor )
{
  if ( ev->Good() )
  {
    if ( ev->Good() )
      mNode_Derived = morkDerived_kBead;
  }
}

 void
morkBead::CloseBead(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      if ( !this->IsShutNode() )
      {
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









 void
morkBeadMap::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseBeadMap(ev);
    this->MarkShut();
  }
}


morkBeadMap::~morkBeadMap() 
{
  MORK_ASSERT(this->IsShutNode());
}


morkBeadMap::morkBeadMap(morkEnv* ev,
  const morkUsage& inUsage, nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
: morkMap(ev, inUsage, ioHeap, sizeof(morkBead*),  0,
   11, ioSlotHeap,  morkBool_kFalse)
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kBeadMap;
}

 void
morkBeadMap::CloseBeadMap(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      this->CutAllBeads(ev);
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
morkBeadMap::AddBead(morkEnv* ev, morkBead* ioBead)
  
{
  if ( ioBead && ev->Good() )
  {
    morkBead* oldBead = 0; 

    mork_bool put = this->Put(ev, &ioBead,  (void*) 0,
       &oldBead,  (void*) 0, (mork_change**) 0);
      
    if ( put ) 
    {
      if ( oldBead != ioBead ) 
        ioBead->AddStrongRef(ev); 
        
      if ( oldBead && oldBead != ioBead ) 
        oldBead->CutStrongRef(ev);
    }
    else
      ioBead->AddStrongRef(ev); 
  }
  else if ( !ioBead )
    ev->NilPointerError();
    
  return ev->Good();
}

mork_bool
morkBeadMap::CutBead(morkEnv* ev, mork_color inColor)
{
  morkBead* oldBead = 0; 
  morkBead bead(inColor);
  morkBead* key = &bead;
  
  mork_bool outCutNode = this->Cut(ev, &key, 
     &oldBead,  (void*) 0, (mork_change**) 0);
    
  if ( oldBead )
    oldBead->CutStrongRef(ev);
  
  bead.CloseBead(ev);
  return outCutNode;
}

morkBead*
morkBeadMap::GetBead(morkEnv* ev, mork_color inColor)
  
{
  morkBead* oldBead = 0; 
  morkBead bead(inColor);
  morkBead* key = &bead;

  this->Get(ev, &key,  &oldBead,  (void*) 0, (mork_change**) 0);
  
  bead.CloseBead(ev);
  return oldBead;
}

mork_num
morkBeadMap::CutAllBeads(morkEnv* ev)
  
{
  mork_num outSlots = mMap_Slots;
  
  morkBeadMapIter i(ev, this);
  morkBead* b = i.FirstBead(ev);

  while ( b )
  {
    b->CutStrongRef(ev);
    i.CutHereBead(ev);
    b = i.NextBead(ev);
  }
  
  return outSlots;
}



 mork_bool
morkBeadMap::Equal(morkEnv* ev, const void* inKeyA, const void* inKeyB) const
{
  MORK_USED_1(ev);
  return (*(const morkBead**) inKeyA)->BeadEqual(
    *(const morkBead**) inKeyB);
}

 mork_u4
morkBeadMap::Hash(morkEnv* ev, const void* inKey) const
{
  MORK_USED_1(ev);
    return (*(const morkBead**) inKey)->BeadHash();
}





 
morkBead* morkBeadMapIter::FirstBead(morkEnv* ev)
{
  morkBead* bead = 0;
  this->First(ev, &bead,  (void*) 0);
  return bead;
}

morkBead* morkBeadMapIter::NextBead(morkEnv* ev)
{
  morkBead* bead = 0;
  this->Next(ev, &bead,  (void*) 0);
  return bead;
}

morkBead* morkBeadMapIter::HereBead(morkEnv* ev)
{
  morkBead* bead = 0;
  this->Here(ev, &bead,  (void*) 0);
  return bead;
}

void morkBeadMapIter::CutHereBead(morkEnv* ev)
{
  this->CutHere(ev,  (void*) 0,  (void*) 0);
}








 void
morkBeadProbeMap::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseBeadProbeMap(ev);
    this->MarkShut();
  }
}


morkBeadProbeMap::~morkBeadProbeMap() 
{
  MORK_ASSERT(this->IsShutNode());
}



morkBeadProbeMap::morkBeadProbeMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
: morkProbeMap(ev, inUsage, ioHeap,
   sizeof(morkBead*),  0,
  ioSlotHeap,  11, 
   morkBool_kTrue)
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kBeadProbeMap;
}

 void
morkBeadProbeMap::CloseBeadProbeMap(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      this->CutAllBeads(ev);
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
morkBeadProbeMap::MapTest(morkEnv* ev, const void* inMapKey,
  const void* inAppKey) const
{
  MORK_USED_1(ev);
  const morkBead* key = *(const morkBead**) inMapKey;
  if ( key )
  {
    mork_bool hit = key->BeadEqual(*(const morkBead**) inAppKey);
    return ( hit ) ? morkTest_kHit : morkTest_kMiss;
  }
  else
    return morkTest_kVoid;
}

 mork_u4 
morkBeadProbeMap::MapHash(morkEnv* ev, const void* inAppKey) const
{
  const morkBead* key = *(const morkBead**) inAppKey;
  if ( key )
    return key->BeadHash();
  else
  {
    ev->NilPointerWarning();
    return 0;
  }
}

 mork_u4 
morkBeadProbeMap::ProbeMapHashMapKey(morkEnv* ev,
  const void* inMapKey) const
{
  const morkBead* key = *(const morkBead**) inMapKey;
  if ( key )
    return key->BeadHash();
  else
  {
    ev->NilPointerWarning();
    return 0;
  }
}

mork_bool
morkBeadProbeMap::AddBead(morkEnv* ev, morkBead* ioBead)
{
  if ( ioBead && ev->Good() )
  {
    morkBead* bead = 0; 
    
    mork_bool put = this->MapAtPut(ev, &ioBead,  (void*) 0, 
       &bead,  (void*) 0);
          
    if ( put ) 
    {
      if ( bead != ioBead ) 
        ioBead->AddStrongRef(ev); 
        
      if ( bead && bead != ioBead ) 
        bead->CutStrongRef(ev);
    }
    else
      ioBead->AddStrongRef(ev); 
  }
  else if ( !ioBead )
    ev->NilPointerError();
    
  return ev->Good();
}

morkBead*
morkBeadProbeMap::GetBead(morkEnv* ev, mork_color inColor)
{
  morkBead* oldBead = 0; 
  morkBead bead(inColor);
  morkBead* key = &bead;

  this->MapAt(ev, &key, &oldBead,  (void*) 0);
  
  bead.CloseBead(ev);
  return oldBead;
}

mork_num
morkBeadProbeMap::CutAllBeads(morkEnv* ev)
  
{
  mork_num outSlots = sMap_Slots;
  
  morkBeadProbeMapIter i(ev, this);
  morkBead* b = i.FirstBead(ev);

  while ( b )
  {
    b->CutStrongRef(ev);
    b = i.NextBead(ev);
  }
  this->MapCutAll(ev);
  
  return outSlots;
}





