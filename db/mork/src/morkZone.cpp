




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKZONE_
#include "morkZone.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif





void morkZone::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseZone(ev);
    this->MarkShut();
  }
}

morkZone::~morkZone() 
{
  MORK_ASSERT(this->IsShutNode());
}


morkZone::morkZone(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioNodeHeap, nsIMdbHeap* ioZoneHeap)
: morkNode(ev, inUsage, ioNodeHeap)
, mZone_Heap( 0 )
, mZone_HeapVolume( 0 )
, mZone_BlockVolume( 0 )
, mZone_RunVolume( 0 )
, mZone_ChipVolume( 0 )
  
, mZone_FreeOldRunVolume( 0 )
  
, mZone_HunkCount( 0 )
, mZone_FreeOldRunCount( 0 )

, mZone_HunkList( 0 )
, mZone_FreeOldRunList( 0 )
  
, mZone_At( 0 )
, mZone_AtSize( 0 )
    
  
{

  morkRun** runs = mZone_FreeRuns;
  morkRun** end = runs + (morkZone_kBuckets + 1); 
  --runs; 
  while ( ++runs < end ) 
    *runs = 0; 
  
  if ( ev->Good() )
  {
    if ( ioZoneHeap )
    {
      nsIMdbHeap_SlotStrongHeap(ioZoneHeap, ev, &mZone_Heap);
      if ( ev->Good() )
        mNode_Derived = morkDerived_kZone;
    }
    else
      ev->NilPointerError();
  }
}

void morkZone::CloseZone(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      nsIMdbHeap* heap = mZone_Heap;
      if ( heap )
      {
        morkHunk* hunk = 0;
        nsIMdbEnv* mev = ev->AsMdbEnv();
        
        morkHunk* next = mZone_HunkList;
        while ( ( hunk = next ) != 0 )
        {
#ifdef morkHunk_USE_TAG_SLOT
          if ( !hunk->HunkGoodTag()  )
            hunk->BadHunkTagWarning(ev);
#endif 

          next = hunk->HunkNext();
          heap->Free(mev, hunk);
        }
      }
      nsIMdbHeap_SlotStrongHeap((nsIMdbHeap*) 0, ev, &mZone_Heap);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}



 void
morkZone::NonZoneTypeError(morkEnv* ev)
{
  ev->NewError("non morkZone");
}

 void
morkZone::NilZoneHeapError(morkEnv* ev)
{
  ev->NewError("nil mZone_Heap");
}

 void
morkHunk::BadHunkTagWarning(morkEnv* ev)
{
  ev->NewWarning("bad mHunk_Tag");
}

 void
morkRun::BadRunTagError(morkEnv* ev)
{
  ev->NewError("bad mRun_Tag");
}

 void
morkRun::RunSizeAlignError(morkEnv* ev)
{
  ev->NewError("bad RunSize() alignment");
}




mork_size morkZone::zone_grow_at(morkEnv* ev, mork_size inNeededSize)
{
  mZone_At = 0;       
  mZone_AtSize = 0;   
  
  mork_size runSize = 0; 
  
  
  morkRun* run = mZone_FreeOldRunList; 
  morkRun* prev = 0; 
 
  while ( run ) 
  {
    morkOldRun* oldRun = (morkOldRun*) run;
    mork_size oldSize = oldRun->OldSize();
    if ( oldSize >= inNeededSize ) 
    {
      runSize = oldSize;
      break; 
    }
    prev = run; 
    run = run->RunNext(); 
  }
  if ( runSize && run ) 
  {
    morkRun* next = run->RunNext();
    if ( prev ) 
      prev->RunSetNext(next); 
    else
      mZone_FreeOldRunList = next; 
      
    morkOldRun *oldRun = (morkOldRun *) run;
    oldRun->OldSetSize(runSize);
    mZone_At = (mork_u1*) run;
    mZone_AtSize = runSize;

#ifdef morkZone_CONFIG_DEBUG
#ifdef morkZone_CONFIG_ALIGN_8
    mork_ip lowThree = ((mork_ip) mZone_At) & 7;
    if ( lowThree ) 
#else 
    mork_ip lowTwo = ((mork_ip) mZone_At) & 3;
    if ( lowTwo ) 
#endif 
      ev->NewWarning("mZone_At not aligned");
#endif 
  }
  else 
  {
    inNeededSize += 7; 
    mork_size newSize = ( inNeededSize > morkZone_kNewHunkSize )?
      inNeededSize : morkZone_kNewHunkSize;
      
    morkHunk* hunk = this->zone_new_hunk(ev, newSize);
    if ( hunk )
    {
      morkRun* hunkRun = hunk->HunkRun();
      mork_u1* at = (mork_u1*) hunkRun->RunAsBlock();
      mork_ip lowBits = ((mork_ip) at) & 7;
      if ( lowBits ) 
      {
        mork_ip skip = (8 - lowBits); 
        at += skip;
        newSize -= skip;
      }
      mZone_At = at;
      mZone_AtSize = newSize;
    }
  }
  
  return mZone_AtSize;
}

morkHunk* morkZone::zone_new_hunk(morkEnv* ev, mdb_size inSize) 
{
  mdb_size hunkSize = inSize + sizeof(morkHunk);
  void* outBlock = 0; 
  mZone_Heap->Alloc(ev->AsMdbEnv(), hunkSize, &outBlock);
  if ( outBlock )
  {
#ifdef morkZone_CONFIG_VOL_STATS
    mZone_HeapVolume += hunkSize; 
#endif 
  
    morkHunk* hunk = (morkHunk*) outBlock;
#ifdef morkHunk_USE_TAG_SLOT
    hunk->HunkInitTag();
#endif 
  
    hunk->HunkSetNext(mZone_HunkList);
    mZone_HunkList = hunk;
    ++mZone_HunkCount;
    
    morkRun* run = hunk->HunkRun();
    run->RunSetSize(inSize);
#ifdef morkRun_USE_TAG_SLOT
    run->RunInitTag();
#endif 
    
    return hunk;
  }
  if ( ev->Good() ) 
    ev->OutOfMemoryError();
  return (morkHunk*) 0;
}

void* morkZone::zone_new_chip(morkEnv* ev, mdb_size inSize) 
{
#ifdef morkZone_CONFIG_VOL_STATS
  mZone_BlockVolume += inSize; 
#endif 
  
  mork_u1* at = mZone_At;
  mork_size atSize = mZone_AtSize; 
  if ( atSize >= inSize ) 
  {
    mZone_At = at + inSize;
    mZone_AtSize = atSize - inSize;
    return at;
  }
  else if ( atSize > morkZone_kMaxHunkWaste ) 
  {
    morkHunk* hunk = this->zone_new_hunk(ev, inSize);
    if ( hunk )
      return hunk->HunkRun();
      
    return (void*) 0; 
  }
  else 
  {
    atSize = this->zone_grow_at(ev, inSize); 
  }

  if ( atSize >= inSize ) 
  {
    at = mZone_At;
    mZone_At = at + inSize;
    mZone_AtSize = atSize - inSize;
    return at;
  }
  
  if ( ev->Good() ) 
    ev->OutOfMemoryError();
    
  return (void*) 0; 
}

void* morkZone::ZoneNewChip(morkEnv* ev, mdb_size inSize) 
{
#ifdef morkZone_CONFIG_ARENA

#ifdef morkZone_CONFIG_DEBUG
  if ( !this->IsZone() )
    this->NonZoneTypeError(ev);
  else if ( !mZone_Heap )
    this->NilZoneHeapError(ev);
#endif 

#ifdef morkZone_CONFIG_ALIGN_8
  inSize += 7;
  inSize &= ~((mork_ip) 7); 
#else 
  inSize += 3;
  inSize &= ~((mork_ip) 3); 
#endif 

#ifdef morkZone_CONFIG_VOL_STATS
  mZone_ChipVolume += inSize; 
#endif 

  return this->zone_new_chip(ev, inSize);

#else 
  void* outBlock = 0;
  mZone_Heap->Alloc(ev->AsMdbEnv(), inSize, &outBlock);
  return outBlock;
#endif 
  
}
  

void* morkZone::ZoneNewRun(morkEnv* ev, mdb_size inSize) 
{
#ifdef morkZone_CONFIG_ARENA

#ifdef morkZone_CONFIG_DEBUG
  if ( !this->IsZone() )
    this->NonZoneTypeError(ev);
  else if ( !mZone_Heap )
    this->NilZoneHeapError(ev);
#endif 

  inSize += morkZone_kRoundAdd;
  inSize &= morkZone_kRoundMask;
  if ( inSize <= morkZone_kMaxCachedRun )
  {
    morkRun** bucket = mZone_FreeRuns + (inSize >> morkZone_kRoundBits);
    morkRun* hit = *bucket;
    if ( hit ) 
    {
      *bucket = hit->RunNext();
      hit->RunSetSize(inSize);
      return hit->RunAsBlock();
    }
  }
  mdb_size blockSize = inSize + sizeof(morkRun); 
#ifdef morkZone_CONFIG_VOL_STATS
  mZone_RunVolume += blockSize; 
#endif 
  morkRun* run = (morkRun*) this->zone_new_chip(ev, blockSize);
  if ( run )
  {
    run->RunSetSize(inSize);
#ifdef morkRun_USE_TAG_SLOT
    run->RunInitTag();
#endif 
    return run->RunAsBlock();
  }
  
  if ( ev->Good() ) 
    ev->OutOfMemoryError();
  
  return (void*) 0; 

#else 
  void* outBlock = 0;
  mZone_Heap->Alloc(ev->AsMdbEnv(), inSize, &outBlock);
  return outBlock;
#endif 
}

void morkZone::ZoneZapRun(morkEnv* ev, void* ioRunBlock) 
{
#ifdef morkZone_CONFIG_ARENA

  morkRun* run = morkRun::BlockAsRun(ioRunBlock);
  mdb_size runSize = run->RunSize();
#ifdef morkZone_CONFIG_VOL_STATS
  mZone_BlockVolume -= runSize; 
#endif 

#ifdef morkZone_CONFIG_DEBUG
  if ( !this->IsZone() )
    this->NonZoneTypeError(ev);
  else if ( !mZone_Heap )
    this->NilZoneHeapError(ev);
  else if ( !ioRunBlock )
    ev->NilPointerError();
  else if ( runSize & morkZone_kRoundAdd )
    run->RunSizeAlignError(ev);
#ifdef morkRun_USE_TAG_SLOT
  else if ( !run->RunGoodTag() )
    run->BadRunTagError(ev);
#endif 
#endif 

  if ( runSize <= morkZone_kMaxCachedRun ) 
  {
    morkRun** bucket = mZone_FreeRuns + (runSize >> morkZone_kRoundBits);
    run->RunSetNext(*bucket); 
    *bucket = run;
  }
  else 
  {
    run->RunSetNext(mZone_FreeOldRunList); 
    mZone_FreeOldRunList = run;
    ++mZone_FreeOldRunCount;
#ifdef morkZone_CONFIG_VOL_STATS
    mZone_FreeOldRunVolume += runSize;
#endif 

    morkOldRun* oldRun = (morkOldRun*) run; 
    oldRun->OldSetSize(runSize); 
  }

#else 
  mZone_Heap->Free(ev->AsMdbEnv(), ioRunBlock);
#endif 
}

void* morkZone::ZoneGrowRun(morkEnv* ev, void* ioRunBlock, mdb_size inSize)
{
#ifdef morkZone_CONFIG_ARENA

  morkRun* run = morkRun::BlockAsRun(ioRunBlock);
  mdb_size runSize = run->RunSize();

#ifdef morkZone_CONFIG_DEBUG
  if ( !this->IsZone() )
    this->NonZoneTypeError(ev);
  else if ( !mZone_Heap )
    this->NilZoneHeapError(ev);
#endif 

#ifdef morkZone_CONFIG_ALIGN_8
  inSize += 7;
  inSize &= ~((mork_ip) 7); 
#else 
  inSize += 3;
  inSize &= ~((mork_ip) 3); 
#endif 

  if ( inSize > runSize )
  {
    void* newBuf = this->ZoneNewRun(ev, inSize);
    if ( newBuf )
    {
      MORK_MEMCPY(newBuf, ioRunBlock, runSize);
      this->ZoneZapRun(ev, ioRunBlock);
      
      return newBuf;
    }
  }
  else
    return ioRunBlock; 
  
  if ( ev->Good() ) 
    ev->OutOfMemoryError();
  
  return (void*) 0; 

#else 
  void* outBlock = 0;
  mZone_Heap->Free(ev->AsMdbEnv(), ioRunBlock);
  return outBlock;
#endif 
}




 mdb_err
morkZone::Alloc(nsIMdbEnv* mev, 
  mdb_size inSize,   
  void** outBlock)  
{
  mdb_err outErr = 0;
  void* block = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    block = this->ZoneNewRun(ev, inSize);
    outErr = ev->AsErr();
  }
  else
    outErr = 1;
    
  if ( outBlock )
    *outBlock = block;
    
  return outErr;
}
  
 mdb_err
morkZone::Free(nsIMdbEnv* mev, 
  void* inBlock)
{
  mdb_err outErr = 0;
  if ( inBlock )
  {
    morkEnv* ev = morkEnv::FromMdbEnv(mev);
    if ( ev )
    {
      this->ZoneZapRun(ev, inBlock);
      outErr = ev->AsErr();
    }
    else
      outErr = 1;
  }
    
  return outErr;
}

 mdb_err
morkZone::HeapAddStrongRef(nsIMdbEnv* mev) 
{
  MORK_USED_1(mev);
  return 0;
}

 mdb_err
morkZone::HeapCutStrongRef(nsIMdbEnv* mev) 
{
  MORK_USED_1(mev);
  return 0;
}



