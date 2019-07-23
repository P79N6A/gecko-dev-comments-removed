




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _ORKINHEAP_
#include "orkinHeap.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif




orkinHeap::orkinHeap() 
#ifdef MORK_DEBUG_HEAP_STATS
  : sHeap_AllocCount( 0 )
  , sHeap_FreeCount( 0 )
  , sHeap_BlockCount( 0 )
  
  , sHeap_BlockVolume( 0 )
  , sHeap_HighWaterVolume( 0 )
  , sHeap_HighWaterTenKilo( 0 )
  , sHeap_HighWaterHundredKilo( 0 )
#endif 
{
}


orkinHeap::~orkinHeap() 
{
}


 mdb_err
orkinHeap::Alloc(nsIMdbEnv* mev, 
  mdb_size inSize,   
  void** outBlock)  
{
#ifdef MORK_DEBUG_HEAP_STATS
  mdb_size realSize = inSize;
  inSize += 12; 
  ++sHeap_AllocCount;
#endif 

  MORK_USED_1(mev);
  mdb_err outErr = 0;
  void* block = ::operator new(inSize);
  if ( !block )
    outErr = morkEnv_kOutOfMemoryError;
#ifdef MORK_DEBUG_HEAP_STATS
  else
  {
    printf("%lx allocating %d\n", this, realSize);
    mork_u4* array = (mork_u4*) block;
    *array++ = (mork_u4) this;
    *array++ = realSize;
    *array++ = orkinHeap_kTag;
    block = array;
    ++sHeap_BlockCount;
    mork_num blockVol = sHeap_BlockVolume + realSize;
    sHeap_BlockVolume = blockVol;
    if ( blockVol > sHeap_HighWaterVolume )
    {
      sHeap_HighWaterVolume = blockVol;
      
      mork_num tenKiloVol = blockVol / (10 * 1024);
      if ( tenKiloVol > sHeap_HighWaterTenKilo )
      {
        sHeap_HighWaterTenKilo = tenKiloVol;
      
        mork_num hundredKiloVol = blockVol / (100 * 1024);
        if ( hundredKiloVol > sHeap_HighWaterHundredKilo )
          sHeap_HighWaterHundredKilo = hundredKiloVol;
      }
    }
  }
#endif 
    
  MORK_ASSERT(outBlock);
  if ( outBlock )
    *outBlock = block;
  return outErr;
}
  
 mdb_err
orkinHeap::Free(nsIMdbEnv* mev, 
  void* inBlock)
{
#ifdef MORK_DEBUG_HEAP_STATS
  ++sHeap_FreeCount;
#endif 

  MORK_USED_1(mev);
  MORK_ASSERT(inBlock);
  if ( inBlock )
  {
#ifdef MORK_DEBUG_HEAP_STATS
    morkEnv* ev = 0; 
    mork_u4* array = (mork_u4*) inBlock;
    if ( *--array != orkinHeap_kTag )
    {
      if ( ev )
        ev->NewWarning("heap block tag not hEaP");
    }
    mork_u4 realSize = *--array;
    inBlock = --array; 
    
    printf("%lx freeing %d\n", this, realSize);
    if ( sHeap_BlockCount )
      --sHeap_BlockCount;
    else if ( ev ) 
      ev->NewWarning("sHeap_BlockCount underflow");
    
    if ( sHeap_BlockVolume >= realSize )
      sHeap_BlockVolume -= realSize;
    else if ( ev )
    {
      sHeap_BlockVolume = 0;
      ev->NewWarning("sHeap_BlockVolume underflow");
    }
#endif 
    
    ::operator delete(inBlock);
  }
  return 0;
}

 mdb_err
orkinHeap::HeapAddStrongRef(nsIMdbEnv* ev) 
{
  MORK_USED_1(ev);
  return 0;
}

 mdb_err
orkinHeap::HeapCutStrongRef(nsIMdbEnv* ev) 
{
  MORK_USED_1(ev);
  return 0;
}




