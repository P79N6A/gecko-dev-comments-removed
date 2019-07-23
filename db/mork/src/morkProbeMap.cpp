









































#include "nscore.h"

#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKPROBEMAP_
#include "morkProbeMap.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif




void morkMapScratch::halt_map_scratch(morkEnv* ev)
{
  nsIMdbHeap* heap = sMapScratch_Heap;
  
  if ( heap )
  {
    if ( sMapScratch_Keys )
      heap->Free(ev->AsMdbEnv(), sMapScratch_Keys);
    if ( sMapScratch_Vals )
      heap->Free(ev->AsMdbEnv(), sMapScratch_Vals);
  }
}







void morkProbeMap::ProbeMapBadTagError(morkEnv* ev) const
{
  ev->NewError("bad sProbeMap_Tag");
  if ( !this )
    ev->NewError("nil morkProbeMap");
}

void morkProbeMap::WrapWithNoVoidSlotError(morkEnv* ev) const
{
  ev->NewError("wrap without void morkProbeMap slot");
}

void morkProbeMap::GrowFailsMaxFillError(morkEnv* ev) const
{
  ev->NewError("grow fails morkEnv > sMap_Fill");
}

void morkProbeMap::MapKeyIsNotIPError(morkEnv* ev) const
{
  ev->NewError("not sMap_KeyIsIP");
}

void morkProbeMap::MapValIsNotIPError(morkEnv* ev) const
{
  ev->NewError("not sMap_ValIsIP");
}

void morkProbeMap::rehash_old_map(morkEnv* ev, morkMapScratch* ioScratch)
{
  mork_size keySize = sMap_KeySize; 
  mork_size valSize = sMap_ValSize; 
  
  mork_count slots = sMap_Slots; 
  mork_u1* keys = sMap_Keys; 
  mork_u1* vals = sMap_Vals; 
  
  mork_bool keyIsIP = ( keys && keySize == sizeof(mork_ip) && sMap_KeyIsIP );
  mork_bool valIsIP = ( vals && valSize == sizeof(mork_ip) && sMap_ValIsIP );

  mork_count oldSlots = ioScratch->sMapScratch_Slots; 
  mork_u1* oldKeys = ioScratch->sMapScratch_Keys; 
  mork_u1* oldVals = ioScratch->sMapScratch_Vals; 
  mork_u1* end = oldKeys + (keySize * oldSlots); 
  
  mork_fill fill = 0; 
  
  while ( oldKeys < end ) 
  {
    if ( !this->ProbeMapIsKeyNil(ev, oldKeys) ) 
    {
      ++fill; 
      mork_u4 hash = this->ProbeMapHashMapKey(ev, oldKeys);

      mork_pos i = hash % slots;   
      mork_pos startPos = i;       
      
      mork_u1* k = keys + (i * keySize);
      while ( !this->ProbeMapIsKeyNil(ev, k) )
      {
        if ( ++i >= (mork_pos)slots ) 
          i = 0; 
          
        if ( i == startPos ) 
        {
          this->WrapWithNoVoidSlotError(ev); 
          return; 
        }
        k = keys + (i * keySize);
      }
      if ( keyIsIP ) 
        *((mork_ip*) k) = *((const mork_ip*) oldKeys); 
      else
        MORK_MEMCPY(k, oldKeys, keySize); 

      if ( oldVals ) 
      {
        mork_size valOffset = (i * valSize);
        mork_u1* v = vals + valOffset;
        mork_u1* ov = oldVals + valOffset;
        if ( valIsIP ) 
          *((mork_ip*) v) = *((const mork_ip*) ov); 
        else
          MORK_MEMCPY(v, ov, valSize); 
      }
    }
    oldKeys += keySize; 
  }
  if ( fill != sMap_Fill ) 
  {
    ev->NewWarning("fill != sMap_Fill");
    sMap_Fill = fill;
  }
}

mork_bool morkProbeMap::grow_probe_map(morkEnv* ev)
{
  if ( sMap_Heap ) 
  {
    mork_num newSlots = ((sMap_Slots * 4) / 3) + 1; 
    morkMapScratch old; 
    if ( this->new_slots(ev, &old, newSlots) ) 
    {      
      ++sMap_Seed; 
      this->rehash_old_map(ev, &old);
      
      if ( ev->Good() ) 
      {
        mork_count slots = sMap_Slots;
        mork_num emptyReserve = (slots / 7) + 1; 
        mork_fill maxFill = slots - emptyReserve; 
        if ( maxFill > sMap_Fill ) 
          sProbeMap_MaxFill = maxFill; 
        else
          this->GrowFailsMaxFillError(ev); 
      }
      
      if ( ev->Bad() ) 
        this->revert_map(ev, &old); 

      old.halt_map_scratch(ev); 
    }
  }
  else ev->OutOfMemoryError();
  
  return ev->Good();
}

void morkProbeMap::revert_map(morkEnv* ev, morkMapScratch* ioScratch)
{
  mork_count tempSlots = ioScratch->sMapScratch_Slots; 
  mork_u1* tempKeys = ioScratch->sMapScratch_Keys;     
  mork_u1* tempVals = ioScratch->sMapScratch_Vals;     
  
  ioScratch->sMapScratch_Slots = sMap_Slots;
  ioScratch->sMapScratch_Keys = sMap_Keys;
  ioScratch->sMapScratch_Vals = sMap_Vals;
  
  sMap_Slots = tempSlots;
  sMap_Keys = tempKeys;
  sMap_Vals = tempVals;
}

void morkProbeMap::put_probe_kv(morkEnv* ev,
  const void* inAppKey, const void* inAppVal, mork_pos inPos)
{
  mork_u1* mapVal = 0;
  mork_u1* mapKey = 0;

  mork_num valSize = sMap_ValSize;
  if ( valSize && inAppVal ) 
  {
    mork_u1* val = sMap_Vals + (valSize * inPos);
    if ( valSize == sizeof(mork_ip) && sMap_ValIsIP ) 
      *((mork_ip*) val) = *((const mork_ip*) inAppVal);
    else
      mapVal = val; 
  }
  if ( inAppKey ) 
  {
    mork_num keySize = sMap_KeySize;
    mork_u1* key = sMap_Keys + (keySize * inPos);
    if ( keySize == sizeof(mork_ip) && sMap_KeyIsIP ) 
      *((mork_ip*) key) = *((const mork_ip*) inAppKey);
    else
      mapKey = key; 
  }
  else
    ev->NilPointerError();

  if ( (  inAppVal && mapVal ) || ( inAppKey && mapKey ) )
    this->ProbeMapPushIn(ev, inAppKey, inAppVal, mapKey, mapVal);

  if ( sMap_Fill > sProbeMap_MaxFill )
    this->grow_probe_map(ev);
}

void morkProbeMap::get_probe_kv(morkEnv* ev,
  void* outAppKey, void* outAppVal, mork_pos inPos) const
{
  const mork_u1* mapVal = 0;
  const mork_u1* mapKey = 0;

  mork_num valSize = sMap_ValSize;
  if ( valSize && outAppVal ) 
  {
    const mork_u1* val = sMap_Vals + (valSize * inPos);
    if ( valSize == sizeof(mork_ip) && sMap_ValIsIP ) 
      *((mork_ip*) outAppVal) = *((const mork_ip*) val);
    else
      mapVal = val; 
  }
  if ( outAppKey ) 
  {
    mork_num keySize = sMap_KeySize;
    const mork_u1* key = sMap_Keys + (keySize * inPos);
    if ( keySize == sizeof(mork_ip) && sMap_KeyIsIP ) 
      *((mork_ip*) outAppKey) = *((const mork_ip*) key);
    else
      mapKey = key; 
  }
  if ( ( outAppVal && mapVal ) || ( outAppKey && mapKey ) )
    this->ProbeMapPullOut(ev, mapKey, mapVal, outAppKey, outAppVal);
}

mork_test
morkProbeMap::find_key_pos(morkEnv* ev, const void* inAppKey,
  mork_u4 inHash, mork_pos* outPos) const
{
  mork_u1* k = sMap_Keys;        
  mork_num size = sMap_KeySize;  
  mork_count slots = sMap_Slots; 
  mork_pos i = inHash % slots;   
  mork_pos startPos = i;         
  
  mork_test outTest = this->MapTest(ev, k + (i * size), inAppKey);
  while ( outTest == morkTest_kMiss )
  {
    if ( ++i >= (mork_pos)slots ) 
      i = 0; 
      
    if ( i == startPos ) 
    {
      this->WrapWithNoVoidSlotError(ev); 
      break; 
    }
    outTest = this->MapTest(ev, k + (i * size), inAppKey);
  }
  *outPos = i;
  
  return outTest;
}
 
void morkProbeMap::probe_map_lazy_init(morkEnv* ev)
{
  if ( this->need_lazy_init() && sMap_Fill == 0 ) 
  {
    
    
    
    mork_u1* keys = sMap_Keys;
    if ( keys ) 
    {
      if ( sProbeMap_ZeroIsClearKey ) 
      {
        mork_num keyVolume = sMap_Slots * sMap_KeySize;
        if ( keyVolume )
          MORK_MEMSET(keys, 0, keyVolume);
      }
      else
        this->ProbeMapClearKey(ev, keys, sMap_Slots);
    }
    else
      this->MapNilKeysError(ev);
  }
  sProbeMap_LazyClearOnAdd = 0; 
}

mork_bool
morkProbeMap::MapAtPut(morkEnv* ev,
  const void* inAppKey, const void* inAppVal,
  void* outAppKey, void* outAppVal)
{
  mork_bool outPut = morkBool_kFalse;
  
  if ( this->GoodProbeMap() ) 
  {
    if ( this->need_lazy_init() && sMap_Fill == 0 ) 
      this->probe_map_lazy_init(ev);
          
    if ( ev->Good() )
    {
      mork_pos slotPos = 0;
      mork_u4 hash = this->MapHash(ev, inAppKey);
      mork_test test = this->find_key_pos(ev, inAppKey, hash, &slotPos);
      outPut = ( test == morkTest_kHit );

      if ( outPut ) 
      {
        if ( outAppKey || outAppVal ) 
          this->get_probe_kv(ev, outAppKey, outAppVal, slotPos);
      }
      else 
      {
        ++sMap_Fill; 
      }
      
      if ( test != morkTest_kMiss ) 
      {
        ++sMap_Seed; 
        this->put_probe_kv(ev, inAppKey, inAppVal, slotPos);
      }
    }
  }
  else this->ProbeMapBadTagError(ev);
  
  return outPut;
}
    
mork_bool
morkProbeMap::MapAt(morkEnv* ev, const void* inAppKey,
    void* outAppKey, void* outAppVal)
{
  if ( this->GoodProbeMap() ) 
  {
    if ( this->need_lazy_init() && sMap_Fill == 0 ) 
      this->probe_map_lazy_init(ev);
          
    mork_pos slotPos = 0;
    mork_u4 hash = this->MapHash(ev, inAppKey);
    mork_test test = this->find_key_pos(ev, inAppKey, hash, &slotPos);
    if ( test == morkTest_kHit ) 
    {
      this->get_probe_kv(ev, outAppKey, outAppVal, slotPos);
      return morkBool_kTrue;
    }
  }
  else this->ProbeMapBadTagError(ev);
  
  return morkBool_kFalse;
}
    
mork_num
morkProbeMap::MapCutAll(morkEnv* ev)
{
  mork_num outCutAll = 0;
  
  if ( this->GoodProbeMap() ) 
  {
    outCutAll = sMap_Fill; 
    
    if ( sMap_Keys && !sProbeMap_ZeroIsClearKey )
      this->ProbeMapClearKey(ev, sMap_Keys, sMap_Slots);

    sMap_Fill = 0; 
  }
  else this->ProbeMapBadTagError(ev);
  
  return outCutAll;
}
    



morkProbeMap::~morkProbeMap() 
{
  MORK_ASSERT(sMap_Keys==0);
  MORK_ASSERT(sProbeMap_Tag==0);
}

 void
morkProbeMap::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseProbeMap(ev);
    this->MarkShut();
  }
}

void morkProbeMap::CloseProbeMap(morkEnv* ev)
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      nsIMdbHeap* heap = sMap_Heap;
      if ( heap ) 
      {
        void* block = sMap_Keys;
        if ( block )
        {
          heap->Free(ev->AsMdbEnv(), block);
          sMap_Keys = 0;
        }
          
        block = sMap_Vals;
        if ( block )
        {
          heap->Free(ev->AsMdbEnv(), block);
          sMap_Vals = 0;
        }
      }
      sMap_Keys = 0;
      sMap_Vals = 0;
      
      this->CloseNode(ev);
      sProbeMap_Tag = 0;
      sProbeMap_MaxFill = 0;
      
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}

void*
morkProbeMap::clear_alloc(morkEnv* ev, mork_size inSize)
{
  void* p = 0;
  nsIMdbHeap* heap = sMap_Heap;
  if ( heap )
  {
    if ( heap->Alloc(ev->AsMdbEnv(), inSize, (void**) &p) == 0 && p )
    {
      MORK_MEMSET(p, 0, inSize);
      return p;
    }
  }
  else
    ev->NilPointerError();
    
  return (void*) 0;
}




mork_u1*
morkProbeMap::map_new_keys(morkEnv* ev, mork_num inSlots)
{
  mork_num size = inSlots * sMap_KeySize;
  return (mork_u1*) this->clear_alloc(ev, size);
}






mork_u1*
morkProbeMap::map_new_vals(morkEnv* ev, mork_num inSlots)
{
  mork_u1* values = 0;
  mork_num size = inSlots * sMap_ValSize;
  if ( size )
    values = (mork_u1*) this->clear_alloc(ev, size);
  return values;
}


void morkProbeMap::MapSeedOutOfSyncError(morkEnv* ev)
{
  ev->NewError("sMap_Seed out of sync");
}

void morkProbeMap::MapFillUnderflowWarning(morkEnv* ev)
{
  ev->NewWarning("sMap_Fill underflow");
}

void morkProbeMap::MapNilKeysError(morkEnv* ev)
{
  ev->NewError("nil sMap_Keys");
}

void morkProbeMap::MapZeroKeySizeError(morkEnv* ev)
{
  ev->NewError("zero sMap_KeySize");
}


void morkProbeMap::ProbeMapCutError(morkEnv* ev)
{
  ev->NewError("morkProbeMap cannot cut");
}


void morkProbeMap::init_probe_map(morkEnv* ev, mork_size inSlots)
{
  
  
  
  

  if ( ev->Good() )
  {
    morkMapScratch old;

    if ( inSlots < 7 ) 
      inSlots = 7; 
    else if ( inSlots > (128 * 1024) ) 
      inSlots = (128 * 1024); 
      
    if ( this->new_slots(ev, &old, inSlots) )
      sProbeMap_Tag = morkProbeMap_kTag;
      
    mork_count slots = sMap_Slots;
    mork_num emptyReserve = (slots / 7) + 1; 
    sProbeMap_MaxFill = slots - emptyReserve;

    MORK_MEMSET(&old, 0, sizeof(morkMapScratch)); 
  }
}

mork_bool
morkProbeMap::new_slots(morkEnv* ev, morkMapScratch* old, mork_num inSlots)
{
  mork_bool outNew = morkBool_kFalse;
  
  
  
  
  
    
  
  mork_u1* newKeys = this->map_new_keys(ev, inSlots);
  mork_u1* newVals = this->map_new_vals(ev, inSlots);
  
  
  mork_bool okayValues = ( newVals || !sMap_ValSize );
  
  if ( newKeys && okayValues )
  {
    outNew = morkBool_kTrue; 

    
    old->sMapScratch_Heap = sMap_Heap;
    
    old->sMapScratch_Slots = sMap_Slots;
    old->sMapScratch_Keys = sMap_Keys;
    old->sMapScratch_Vals = sMap_Vals;
    
    
    ++sMap_Seed; 
    sMap_Keys = newKeys;
    sMap_Vals = newVals;
    sMap_Slots = inSlots;
  }
  else 
  {
    nsIMdbHeap* heap = sMap_Heap;
    if ( newKeys )
      heap->Free(ev->AsMdbEnv(), newKeys);
    if ( newVals )
      heap->Free(ev->AsMdbEnv(), newVals);
    
    MORK_MEMSET(old, 0, sizeof(morkMapScratch)); 
  }
  
  return outNew;
}

void
morkProbeMap::clear_probe_map(morkEnv* ev, nsIMdbHeap* ioMapHeap)
{
  sProbeMap_Tag = 0;
  sMap_Seed = 0;
  sMap_Slots = 0;
  sMap_Fill = 0;
  sMap_Keys = 0;
  sMap_Vals = 0;
  sProbeMap_MaxFill = 0;
  
  sMap_Heap = ioMapHeap;
  if ( !ioMapHeap )
    ev->NilPointerError();
}

morkProbeMap::morkProbeMap(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioNodeHeap,
  mork_size inKeySize, mork_size inValSize,
  nsIMdbHeap* ioMapHeap, mork_size inSlots,
  mork_bool inZeroIsClearKey)
  
: morkNode(ev, inUsage, ioNodeHeap)
, sMap_Heap( ioMapHeap )
    
, sMap_Keys( 0 )
, sMap_Vals( 0 )
  
, sMap_Seed( 0 )   
    
, sMap_Slots( 0 )  
, sMap_Fill( 0 )   

, sMap_KeySize( 0 ) 
, sMap_ValSize( 0 ) 
  
, sMap_KeyIsIP( morkBool_kFalse ) 
, sMap_ValIsIP( morkBool_kFalse ) 

, sProbeMap_MaxFill( 0 )
, sProbeMap_LazyClearOnAdd( 0 )
, sProbeMap_ZeroIsClearKey( inZeroIsClearKey )
, sProbeMap_Tag( 0 )
{
  
  
  
  

  if ( ev->Good() )
  {
    this->clear_probe_map(ev, ioMapHeap);
    if ( ev->Good() )
    {      
      sMap_KeySize = inKeySize;
      sMap_ValSize = inValSize;
      sMap_KeyIsIP = ( inKeySize == sizeof(mork_ip) );
      sMap_ValIsIP = ( inValSize == sizeof(mork_ip) );
      
      this->init_probe_map(ev, inSlots);
      if ( ev->Good() )
      {
        if ( !inZeroIsClearKey ) 
          sProbeMap_LazyClearOnAdd = morkProbeMap_kLazyClearOnAdd;
          
        mNode_Derived = morkDerived_kProbeMap;
      }
    }
  }
}



 mork_test 
morkProbeMap::MapTest(morkEnv* ev,
  const void* inMapKey, const void* inAppKey) const
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
  mork_size keySize = sMap_KeySize;
  if ( keySize == sizeof(mork_ip) && sMap_KeyIsIP )
  {
    mork_ip mapKey = *((const mork_ip*) inMapKey);
    if ( mapKey == *((const mork_ip*) inAppKey) )
      return morkTest_kHit;
    else
    {
      return ( mapKey )? morkTest_kMiss : morkTest_kVoid;
    }
  }
  else
  {
    mork_bool allSame = morkBool_kTrue;
    mork_bool allZero = morkBool_kTrue;
    const mork_u1* ak = (const mork_u1*) inAppKey;
    const mork_u1* mk = (const mork_u1*) inMapKey;
    const mork_u1* end = mk + keySize;
    --mk; 
    while ( ++mk < end )
    {
      mork_u1 byte = *mk;
      if ( byte ) 
        allZero = morkBool_kFalse;
      if ( byte != *ak++ ) 
        allSame = morkBool_kFalse;
    }
    if ( allSame )
      return morkTest_kHit;
    else
      return ( allZero )? morkTest_kVoid : morkTest_kMiss;
  }
}

 mork_u4 
morkProbeMap::MapHash(morkEnv* ev, const void* inAppKey) const
{
  mork_size keySize = sMap_KeySize;
  if ( keySize == sizeof(mork_ip) && sMap_KeyIsIP )
  {
    return *((const mork_ip*) inAppKey);
  }
  else
  {
    const mork_u1* key = (const mork_u1*) inAppKey;
    const mork_u1* end = key + keySize;
    --key; 
    while ( ++key < end )
    {
      if ( *key ) 
        return morkBool_kFalse;
    }
    return morkBool_kTrue;
  }
  return (mork_u4) NS_PTR_TO_INT32(inAppKey);
}




 mork_u4
morkProbeMap::ProbeMapHashMapKey(morkEnv* ev, const void* inMapKey) const
  
  
  
  
  
  
  
  
  
  
  
  
{
  return this->MapHash(ev, inMapKey);
}

 mork_bool
morkProbeMap::ProbeMapIsKeyNil(morkEnv* ev, void* ioMapKey)
  
  
  
  
  
  
{
  if ( sMap_KeySize == sizeof(mork_ip) && sMap_KeyIsIP )
  {
    return !*((const mork_ip*) ioMapKey);
  }
  else
  {
    const mork_u1* key = (const mork_u1*) ioMapKey;
    const mork_u1* end = key + sMap_KeySize;
    --key; 
    while ( ++key < end )
    {
      if ( *key ) 
        return morkBool_kFalse;
    }
    return morkBool_kTrue;
  }
}

 void
morkProbeMap::ProbeMapClearKey(morkEnv* ev, 
  void* ioMapKey, mork_count inKeyCount) 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
  if ( ioMapKey && inKeyCount )
  {
    MORK_MEMSET(ioMapKey, 0, (inKeyCount * sMap_KeySize));
  }
  else
    ev->NilPointerWarning();
}

 void
morkProbeMap::ProbeMapPushIn(morkEnv* ev, 
  const void* inAppKey, const void* inAppVal, 
  void* outMapKey, void* outMapVal)      
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
}

 void
morkProbeMap::ProbeMapPullOut(morkEnv* ev, 
  const void* inMapKey, const void* inMapVal, 
  void* outAppKey, void* outAppVal) const    
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
}





morkProbeMapIter::morkProbeMapIter(morkEnv* ev, morkProbeMap* ioMap)
: sProbeMapIter_Map( 0 )
, sProbeMapIter_Seed( 0 )
, sProbeMapIter_HereIx( morkProbeMapIter_kBeforeIx )
{
  if ( ioMap )
  {
    if ( ioMap->GoodProbeMap() )
    {
      if ( ioMap->need_lazy_init() ) 
        ioMap->probe_map_lazy_init(ev);
        
      sProbeMapIter_Map = ioMap;
      sProbeMapIter_Seed = ioMap->sMap_Seed;
    }
    else ioMap->ProbeMapBadTagError(ev);
  }
  else ev->NilPointerError();
}

void morkProbeMapIter::CloseMapIter(morkEnv* ev)
{
  MORK_USED_1(ev);
  sProbeMapIter_Map = 0;
  sProbeMapIter_Seed = 0;

  sProbeMapIter_HereIx = morkProbeMapIter_kAfterIx;
}

morkProbeMapIter::morkProbeMapIter( )

{
  sProbeMapIter_Map = 0;
  sProbeMapIter_Seed = 0;

  sProbeMapIter_HereIx = morkProbeMapIter_kBeforeIx;
}

void morkProbeMapIter::InitProbeMapIter(morkEnv* ev, morkProbeMap* ioMap)
{
  sProbeMapIter_Map = 0;
  sProbeMapIter_Seed = 0;

  sProbeMapIter_HereIx = morkProbeMapIter_kBeforeIx;

  if ( ioMap )
  {
    if ( ioMap->GoodProbeMap() )
    {
      if ( ioMap->need_lazy_init() ) 
        ioMap->probe_map_lazy_init(ev);
        
      sProbeMapIter_Map = ioMap;
      sProbeMapIter_Seed = ioMap->sMap_Seed;
    }
    else ioMap->ProbeMapBadTagError(ev);
  }
  else ev->NilPointerError();
}
 
mork_bool morkProbeMapIter::IterFirst(morkEnv* ev,
  void* outAppKey, void* outAppVal)
{
  sProbeMapIter_HereIx = morkProbeMapIter_kAfterIx; 
  morkProbeMap* map = sProbeMapIter_Map;
  
  if ( map && map->GoodProbeMap() ) 
  {
    sProbeMapIter_Seed = map->sMap_Seed; 
    
    mork_u1* k = map->sMap_Keys;  
    mork_num size = map->sMap_KeySize;  
    mork_count slots = map->sMap_Slots; 
    mork_pos here = 0;  
    
    while ( here < (mork_pos)slots )
    {
      if ( !map->ProbeMapIsKeyNil(ev, k + (here * size)) )
      {
        map->get_probe_kv(ev, outAppKey, outAppVal, here);
        
        sProbeMapIter_HereIx = (mork_i4) here;
        return morkBool_kTrue;
      }
      ++here; 
    } 
  }
  else map->ProbeMapBadTagError(ev);

  return morkBool_kFalse;
}

mork_bool morkProbeMapIter::IterNext(morkEnv* ev,
  void* outAppKey, void* outAppVal)
{
  morkProbeMap* map = sProbeMapIter_Map;
  
  if ( map && map->GoodProbeMap() ) 
  {    
    if ( sProbeMapIter_Seed == map->sMap_Seed ) 
    {
      if ( sProbeMapIter_HereIx != morkProbeMapIter_kAfterIx )
      {
        mork_pos here = (mork_pos) sProbeMapIter_HereIx;
        if ( sProbeMapIter_HereIx < 0 )
          here = 0;
        else
          ++here;
          
        sProbeMapIter_HereIx = morkProbeMapIter_kAfterIx; 

        mork_u1* k = map->sMap_Keys;  
        mork_num size = map->sMap_KeySize;  
        mork_count slots = map->sMap_Slots; 
        
        while ( here < (mork_pos)slots )
        {
          if ( !map->ProbeMapIsKeyNil(ev, k + (here * size)) )
          {
            map->get_probe_kv(ev, outAppKey, outAppVal, here);
            
            sProbeMapIter_HereIx = (mork_i4) here;
            return morkBool_kTrue;
          }
          ++here; 
        } 
      }
    }
    else map->MapSeedOutOfSyncError(ev);
  }
  else map->ProbeMapBadTagError(ev);

  return morkBool_kFalse;
}

mork_bool morkProbeMapIter::IterHere(morkEnv* ev,
  void* outAppKey, void* outAppVal)
{
  morkProbeMap* map = sProbeMapIter_Map;
  
  if ( map && map->GoodProbeMap() ) 
  {    
    if ( sProbeMapIter_Seed == map->sMap_Seed ) 
    {
      mork_pos here = (mork_pos) sProbeMapIter_HereIx;
      mork_count slots = map->sMap_Slots; 
      if ( sProbeMapIter_HereIx >= 0 && (here < (mork_pos)slots))
      {
        mork_u1* k = map->sMap_Keys;  
        mork_num size = map->sMap_KeySize;  

        if ( !map->ProbeMapIsKeyNil(ev, k + (here * size)) )
        {
          map->get_probe_kv(ev, outAppKey, outAppVal, here);
          return morkBool_kTrue;
        }
      }
    }
    else map->MapSeedOutOfSyncError(ev);
  }
  else map->ProbeMapBadTagError(ev);

  return morkBool_kFalse;
}

mork_change*
morkProbeMapIter::First(morkEnv* ev, void* outKey, void* outVal)
{
  if ( this->IterFirst(ev, outKey, outVal) )
    return &sProbeMapIter_Change;
  
  return (mork_change*) 0;
}

mork_change*
morkProbeMapIter::Next(morkEnv* ev, void* outKey, void* outVal)
{
  if ( this->IterNext(ev, outKey, outVal) )
    return &sProbeMapIter_Change;
  
  return (mork_change*) 0;
}

mork_change*
morkProbeMapIter::Here(morkEnv* ev, void* outKey, void* outVal)
{
  if ( this->IterHere(ev, outKey, outVal) )
    return &sProbeMapIter_Change;
  
  return (mork_change*) 0;
}

mork_change*
morkProbeMapIter::CutHere(morkEnv* ev, void* outKey, void* outVal)
{
  morkProbeMap::ProbeMapCutError(ev);
  
  return (mork_change*) 0;
}






void* morkProbeMapIter::IterFirstVal(morkEnv* ev, void* outKey)

{
  morkProbeMap* map = sProbeMapIter_Map;
  if ( map )
  {
    if ( map->sMap_ValIsIP )
    {
      void* v = 0;
      this->IterFirst(ev, outKey, &v);
      return v;
    }
    else
      map->MapValIsNotIPError(ev);
  }
  return (void*) 0;
}

void* morkProbeMapIter::IterNextVal(morkEnv* ev, void* outKey)

{
  morkProbeMap* map = sProbeMapIter_Map;
  if ( map )
  {
    if ( map->sMap_ValIsIP )
    {
      void* v = 0;
      this->IterNext(ev, outKey, &v);
      return v;
    }
    else
      map->MapValIsNotIPError(ev);
  }
  return (void*) 0;
}

void* morkProbeMapIter::IterHereVal(morkEnv* ev, void* outKey)

{
  morkProbeMap* map = sProbeMapIter_Map;
  if ( map )
  {
    if ( map->sMap_ValIsIP )
    {
      void* v = 0;
      this->IterHere(ev, outKey, &v);
      return v;
    }
    else
      map->MapValIsNotIPError(ev);
  }
  return (void*) 0;
}




void* morkProbeMapIter::IterFirstKey(morkEnv* ev)

{
  morkProbeMap* map = sProbeMapIter_Map;
  if ( map )
  {
    if ( map->sMap_KeyIsIP )
    {
      void* k = 0;
      this->IterFirst(ev, &k, (void*) 0);
      return k;
    }
    else
      map->MapKeyIsNotIPError(ev);
  }
  return (void*) 0;
}

void* morkProbeMapIter::IterNextKey(morkEnv* ev)

{
  morkProbeMap* map = sProbeMapIter_Map;
  if ( map )
  {
    if ( map->sMap_KeyIsIP )
    {
      void* k = 0;
      this->IterNext(ev, &k, (void*) 0);
      return k;
    }
    else
      map->MapKeyIsNotIPError(ev);
  }
  return (void*) 0;
}

void* morkProbeMapIter::IterHereKey(morkEnv* ev)

{
  morkProbeMap* map = sProbeMapIter_Map;
  if ( map )
  {
    if ( map->sMap_KeyIsIP )
    {
      void* k = 0;
      this->IterHere(ev, &k, (void*) 0);
      return k;
    }
    else
      map->MapKeyIsNotIPError(ev);
  }
  return (void*) 0;
}


