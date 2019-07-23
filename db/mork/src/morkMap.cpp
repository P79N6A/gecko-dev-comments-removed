







































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

#ifndef _MORKENV_
#include "morkEnv.h"
#endif


class morkHashArrays {
public:
  nsIMdbHeap*   mHashArrays_Heap;     
  mork_count    mHashArrays_Slots;    
  
  mork_u1*      mHashArrays_Keys;     
  mork_u1*      mHashArrays_Vals;     
  morkAssoc*    mHashArrays_Assocs;   
  mork_change*  mHashArrays_Changes;  
  morkAssoc**   mHashArrays_Buckets;  
  morkAssoc*    mHashArrays_FreeList; 
  
public:
  void finalize(morkEnv* ev);
};

void morkHashArrays::finalize(morkEnv* ev)
{
  nsIMdbEnv* menv = ev->AsMdbEnv();
  nsIMdbHeap* heap = mHashArrays_Heap;
  
  if ( heap )
  {
    if ( mHashArrays_Keys )
      heap->Free(menv, mHashArrays_Keys);
    if ( mHashArrays_Vals )
      heap->Free(menv, mHashArrays_Vals);
    if ( mHashArrays_Assocs )
      heap->Free(menv, mHashArrays_Assocs);
    if ( mHashArrays_Changes )
      heap->Free(menv, mHashArrays_Changes);
    if ( mHashArrays_Buckets )
      heap->Free(menv, mHashArrays_Buckets);
  }
}






 void
morkMap::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseMap(ev);
    this->MarkShut();
  }
}


morkMap::~morkMap() 
{
  MORK_ASSERT(mMap_FreeList==0);
  MORK_ASSERT(mMap_Buckets==0);
  MORK_ASSERT(mMap_Keys==0);
  MORK_ASSERT(mMap_Vals==0);
  MORK_ASSERT(mMap_Changes==0);
  MORK_ASSERT(mMap_Assocs==0);
}

 void
morkMap::CloseMap(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      nsIMdbHeap* heap = mMap_Heap;
      if ( heap ) 
      {
        nsIMdbEnv* menv = ev->AsMdbEnv();
        
        if ( mMap_Keys )
          heap->Free(menv, mMap_Keys);
          
        if ( mMap_Vals )
          heap->Free(menv, mMap_Vals);
          
        if ( mMap_Assocs )
          heap->Free(menv, mMap_Assocs);
          
        if ( mMap_Buckets )
          heap->Free(menv, mMap_Buckets);
          
        if ( mMap_Changes )
          heap->Free(menv, mMap_Changes);
      }
      mMap_Keys = 0;
      mMap_Vals = 0;
      mMap_Buckets = 0;
      mMap_Assocs = 0;
      mMap_Changes = 0;
      mMap_FreeList = 0;
      MORK_MEMSET(&mMap_Form, 0, sizeof(morkMapForm));
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




void
morkMap::clear_map(morkEnv* ev, nsIMdbHeap* ioSlotHeap)
{
  mMap_Tag = 0;
  mMap_Seed = 0;
  mMap_Slots = 0;
  mMap_Fill = 0;
  mMap_Keys = 0;
  mMap_Vals = 0;
  mMap_Assocs = 0;
  mMap_Changes = 0;
  mMap_Buckets = 0;
  mMap_FreeList = 0;
  MORK_MEMSET(&mMap_Form, 0, sizeof(morkMapForm));
  
  mMap_Heap = 0;
  if ( ioSlotHeap )
  {
    nsIMdbHeap_SlotStrongHeap(ioSlotHeap, ev, &mMap_Heap);
  }
  else
    ev->NilPointerError();
}

morkMap::morkMap(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap, 
  mork_size inKeySize, mork_size inValSize,
  mork_size inSlots, nsIMdbHeap* ioSlotHeap, mork_bool inHoldChanges)
: morkNode(ev, inUsage, ioHeap)
, mMap_Heap( 0 )
{
  if ( ev->Good() )
  {
    this->clear_map(ev, ioSlotHeap);
    if ( ev->Good() )
    {
      mMap_Form.mMapForm_HoldChanges = inHoldChanges;
      
      mMap_Form.mMapForm_KeySize = inKeySize;
      mMap_Form.mMapForm_ValSize = inValSize;
      mMap_Form.mMapForm_KeyIsIP = ( inKeySize == sizeof(mork_ip) );
      mMap_Form.mMapForm_ValIsIP = ( inValSize == sizeof(mork_ip) );
      
      this->InitMap(ev, inSlots);
      if ( ev->Good() )
        mNode_Derived = morkDerived_kMap;
    }
  }
}

void
morkMap::NewIterOutOfSyncError(morkEnv* ev)
{
  ev->NewError("map iter out of sync");
}

void morkMap::NewBadMapError(morkEnv* ev)
{
  ev->NewError("bad morkMap tag");
  if ( !this )
    ev->NewError("nil morkMap instance");
}

void morkMap::NewSlotsUnderflowWarning(morkEnv* ev)
{
  ev->NewWarning("member count underflow");
}

void morkMap::InitMap(morkEnv* ev, mork_size inSlots)
{
  if ( ev->Good() )
  {
    morkHashArrays old;
    
    if ( inSlots < 3 ) 
      inSlots = 3; 
    else if ( inSlots > (128 * 1024) ) 
      inSlots = (128 * 1024); 
      
    if ( this->new_arrays(ev, &old, inSlots) )
      mMap_Tag = morkMap_kTag;

    MORK_MEMSET(&old, 0, sizeof(morkHashArrays)); 
  }
}

morkAssoc**
morkMap::find(morkEnv* ev, const void* inKey, mork_u4 inHash) const
{
  mork_u1* keys = mMap_Keys;
  mork_num keySize = this->FormKeySize();
  
  
  morkAssoc** ref = mMap_Buckets + (inHash % mMap_Slots);
  morkAssoc* assoc = *ref;
  while ( assoc ) 
  {
    mork_pos i = assoc - mMap_Assocs; 
    if ( this->Equal(ev, keys + (i * keySize), inKey) ) 
      return ref;
      
    ref = &assoc->mAssoc_Next; 
    assoc = *ref; 
  }
  return 0;
}



void
morkMap::get_assoc(void* outKey, void* outVal, mork_pos inPos) const
{
  mork_num valSize = this->FormValSize();
  if ( valSize && outVal ) 
  {
    const mork_u1* value = mMap_Vals + (valSize * inPos);
    if ( valSize == sizeof(mork_ip) && this->FormValIsIP() ) 
      *((mork_ip*) outVal) = *((const mork_ip*) value);
    else
      MORK_MEMCPY(outVal, value, valSize);
  }
  if ( outKey ) 
  {
    mork_num keySize = this->FormKeySize();
    const mork_u1* key = mMap_Keys + (keySize * inPos);
    if ( keySize == sizeof(mork_ip) && this->FormKeyIsIP() ) 
      *((mork_ip*) outKey) = *((const mork_ip*) key);
    else
      MORK_MEMCPY(outKey, key, keySize);
  }
}



void
morkMap::put_assoc(const void* inKey, const void* inVal, mork_pos inPos) const
{
  mork_num valSize = this->FormValSize();
  if ( valSize && inVal ) 
  {
    mork_u1* value = mMap_Vals + (valSize * inPos);
    if ( valSize == sizeof(mork_ip) && this->FormValIsIP() ) 
      *((mork_ip*) value) = *((const mork_ip*) inVal);
    else
      MORK_MEMCPY(value, inVal, valSize);
  }
  if ( inKey ) 
  {
    mork_num keySize = this->FormKeySize();
    mork_u1* key = mMap_Keys + (keySize * inPos);
    if ( keySize == sizeof(mork_ip) && this->FormKeyIsIP() ) 
      *((mork_ip*) key) = *((const mork_ip*) inKey);
    else
      MORK_MEMCPY(key, inKey, keySize);
  }
}

void*
morkMap::clear_alloc(morkEnv* ev, mork_size inSize)
{
  void* p = 0;
  nsIMdbHeap* heap = mMap_Heap;
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

void*
morkMap::alloc(morkEnv* ev, mork_size inSize)
{
  void* p = 0;
  nsIMdbHeap* heap = mMap_Heap;
  if ( heap )
  {
    if ( heap->Alloc(ev->AsMdbEnv(), inSize, (void**) &p) == 0 && p )
      return p;
  }
  else
    ev->NilPointerError();

  return (void*) 0;
}



mork_u1*
morkMap::new_keys(morkEnv* ev, mork_num inSlots)
{
  mork_num size = inSlots * this->FormKeySize();
  return (mork_u1*) this->clear_alloc(ev, size);
}




mork_u1*
morkMap::new_values(morkEnv* ev, mork_num inSlots)
{
  mork_u1* values = 0;
  mork_num size = inSlots * this->FormValSize();
  if ( size )
    values = (mork_u1*) this->clear_alloc(ev, size);
  return values;
}

mork_change*
morkMap::new_changes(morkEnv* ev, mork_num inSlots)
{
  mork_change* changes = 0;
  mork_num size = inSlots * sizeof(mork_change);
  if ( size && mMap_Form.mMapForm_HoldChanges )
    changes = (mork_change*) this->clear_alloc(ev, size);
  return changes;
}



morkAssoc**
morkMap::new_buckets(morkEnv* ev, mork_num inSlots)
{
  mork_num size = inSlots * sizeof(morkAssoc*);
  return (morkAssoc**) this->clear_alloc(ev, size);
}





morkAssoc*
morkMap::new_assocs(morkEnv* ev, mork_num inSlots)
{
  mork_num size = inSlots * sizeof(morkAssoc);
  morkAssoc* assocs = (morkAssoc*) this->alloc(ev, size);
  if ( assocs ) 
  {
    morkAssoc* a = assocs + (inSlots - 1); 
    a->mAssoc_Next = 0; 
    while ( --a >= assocs ) 
      a->mAssoc_Next = a + 1; 
  }
  return assocs;
}

mork_bool
morkMap::new_arrays(morkEnv* ev, morkHashArrays* old, mork_num inSlots)
{
  mork_bool outNew = morkBool_kFalse;
    
  
  morkAssoc** newBuckets = this->new_buckets(ev, inSlots);
  morkAssoc* newAssocs = this->new_assocs(ev, inSlots);
  mork_u1* newKeys = this->new_keys(ev, inSlots);
  mork_u1* newValues = this->new_values(ev, inSlots);
  mork_change* newChanges = this->new_changes(ev, inSlots);
  
  
  mork_bool okayChanges = ( newChanges || !this->FormHoldChanges() );
  
  
  mork_bool okayValues = ( newValues || !this->FormValSize() );
  
  if ( newBuckets && newAssocs && newKeys && okayChanges && okayValues )
  {
    outNew = morkBool_kTrue; 

    
    old->mHashArrays_Heap = mMap_Heap;
    
    old->mHashArrays_Slots = mMap_Slots;
    old->mHashArrays_Keys = mMap_Keys;
    old->mHashArrays_Vals = mMap_Vals;
    old->mHashArrays_Assocs = mMap_Assocs;
    old->mHashArrays_Buckets = mMap_Buckets;
    old->mHashArrays_Changes = mMap_Changes;
    
    
    ++mMap_Seed; 
    mMap_Keys = newKeys;
    mMap_Vals = newValues;
    mMap_Buckets = newBuckets;
    mMap_Assocs = newAssocs;
    mMap_FreeList = newAssocs; 
    mMap_Changes = newChanges;
    mMap_Slots = inSlots;
  }
  else 
  {
    nsIMdbEnv* menv = ev->AsMdbEnv();
    nsIMdbHeap* heap = mMap_Heap;
    if ( newBuckets )
      heap->Free(menv, newBuckets);
    if ( newAssocs )
      heap->Free(menv, newAssocs);
    if ( newKeys )
      heap->Free(menv, newKeys);
    if ( newValues )
      heap->Free(menv, newValues);
    if ( newChanges )
      heap->Free(menv, newChanges);
    
    MORK_MEMSET(old, 0, sizeof(morkHashArrays));
  }
  
  return outNew;
}






























mork_bool morkMap::grow(morkEnv* ev)
{
  if ( mMap_Heap ) 
  {
    mork_num newSlots = (mMap_Slots * 2); 
    morkHashArrays old; 
    if ( this->new_arrays(ev, &old, newSlots) ) 
    {
      
      
      
      mork_num oldSlots = old.mHashArrays_Slots; 
      mork_num keyBulk = oldSlots * this->FormKeySize(); 
      mork_num valBulk = oldSlots * this->FormValSize(); 
      
      
      morkAssoc** newBuckets = mMap_Buckets; 
      morkAssoc* newAssocs = mMap_Assocs; 
      morkAssoc* newFreeList = newAssocs + oldSlots; 
      mork_u1* key = mMap_Keys; 
      --newAssocs; 
      
      
      MORK_MEMCPY(mMap_Keys, old.mHashArrays_Keys, keyBulk);
      if ( valBulk ) 
        MORK_MEMCPY(mMap_Vals, old.mHashArrays_Vals, valBulk);
        
      mMap_FreeList = newFreeList; 
      
      while ( ++newAssocs < newFreeList ) 
      {
        morkAssoc** top = newBuckets + (this->Hash(ev, key) % newSlots);
        key += this->FormKeySize(); 
        newAssocs->mAssoc_Next = *top; 
        *top = newAssocs; 
      }
      ++mMap_Seed; 
      old.finalize(ev); 
    }
  }
  else ev->OutOfMemoryError();
  
  return ev->Good();
}


mork_bool
morkMap::Put(morkEnv* ev, const void* inKey, const void* inVal,
  void* outKey, void* outVal, mork_change** outChange)
{
  mork_bool outPut = morkBool_kFalse;
  
  if ( this->GoodMap() ) 
  {
    mork_u4 hash = this->Hash(ev, inKey);
    morkAssoc** ref = this->find(ev, inKey, hash);
    if ( ref ) 
    {
      outPut = morkBool_kTrue; 
    }
    else 
    {
      morkAssoc* assoc = this->pop_free_assoc();
      if ( !assoc ) 
      {
        if ( this->grow(ev) ) 
          assoc = this->pop_free_assoc();
      }
      if ( assoc ) 
      {
        ref = mMap_Buckets + (hash % mMap_Slots);
        assoc->mAssoc_Next = *ref; 
        *ref = assoc; 
          
        ++mMap_Fill; 
        ++mMap_Seed; 
      }
    }
    if ( ref ) 
    {
      mork_pos i = (*ref) - mMap_Assocs; 
      if ( outPut && (outKey || outVal) ) 
        this->get_assoc(outKey, outVal, i);

      this->put_assoc(inKey, inVal, i);
      ++mMap_Seed; 
      
      if ( outChange )
      {
        if ( mMap_Changes )
          *outChange = mMap_Changes + i;
        else
          *outChange = this->FormDummyChange();
      }
    }
  }
  else this->NewBadMapError(ev);
  
  return outPut;
}

mork_num
morkMap::CutAll(morkEnv* ev)
{
  mork_num outCutAll = 0;
  
  if ( this->GoodMap() ) 
  {
    mork_num slots = mMap_Slots;
    morkAssoc* before = mMap_Assocs - 1; 
    morkAssoc* assoc = before + slots; 

    ++mMap_Seed; 

    
    assoc->mAssoc_Next = 0; 
    while ( --assoc > before ) 
      assoc->mAssoc_Next = assoc + 1;
    mMap_FreeList = mMap_Assocs; 

    outCutAll = mMap_Fill; 

    mMap_Fill = 0; 
  }
  else this->NewBadMapError(ev);
  
  return outCutAll;
}

mork_bool
morkMap::Cut(morkEnv* ev, const void* inKey,
  void* outKey, void* outVal, mork_change** outChange)
{
  mork_bool outCut = morkBool_kFalse;
  
  if ( this->GoodMap() ) 
  {
    mork_u4 hash = this->Hash(ev, inKey);
    morkAssoc** ref = this->find(ev, inKey, hash);
    if ( ref ) 
    {
      outCut = morkBool_kTrue;
      morkAssoc* assoc = *ref;
      mork_pos i = assoc - mMap_Assocs; 
      if ( outKey || outVal )
        this->get_assoc(outKey, outVal, i);

      *ref = assoc->mAssoc_Next; 
      this->push_free_assoc(assoc); 

      if ( outChange )
      {
        if ( mMap_Changes )
          *outChange = mMap_Changes + i;
        else
          *outChange = this->FormDummyChange();
      }
      
      ++mMap_Seed; 
      if ( mMap_Fill ) 
        --mMap_Fill; 
      else
        this->NewSlotsUnderflowWarning(ev);
    }
  }
  else this->NewBadMapError(ev);
  
  return outCut;
}

mork_bool
morkMap::Get(morkEnv* ev, const void* inKey,
  void* outKey, void* outVal, mork_change** outChange)
{
  mork_bool outGet = morkBool_kFalse;
  
  if ( this->GoodMap() ) 
  {
    mork_u4 hash = this->Hash(ev, inKey);
    morkAssoc** ref = this->find(ev, inKey, hash);
    if ( ref ) 
    {
      mork_pos i = (*ref) - mMap_Assocs; 
      outGet = morkBool_kTrue;
      this->get_assoc(outKey, outVal, i);
      if ( outChange )
      {
        if ( mMap_Changes )
          *outChange = mMap_Changes + i;
        else
          *outChange = this->FormDummyChange();
      }
    }
  }
  else this->NewBadMapError(ev);
  
  return outGet;
}


morkMapIter::morkMapIter( )
: mMapIter_Map( 0 )
, mMapIter_Seed( 0 )
  
, mMapIter_Bucket( 0 )
, mMapIter_AssocRef( 0 )
, mMapIter_Assoc( 0 )
, mMapIter_Next( 0 )
{
}

void
morkMapIter::InitMapIter(morkEnv* ev, morkMap* ioMap)
{
  mMapIter_Map = 0;
  mMapIter_Seed = 0;

  mMapIter_Bucket = 0;
  mMapIter_AssocRef = 0;
  mMapIter_Assoc = 0;
  mMapIter_Next = 0;

  if ( ioMap )
  {
    if ( ioMap->GoodMap() )
    {
      mMapIter_Map = ioMap;
      mMapIter_Seed = ioMap->mMap_Seed;
    }
    else ioMap->NewBadMapError(ev);
  }
  else ev->NilPointerError();
}

morkMapIter::morkMapIter(morkEnv* ev, morkMap* ioMap)
: mMapIter_Map( 0 )
, mMapIter_Seed( 0 )
  
, mMapIter_Bucket( 0 )
, mMapIter_AssocRef( 0 )
, mMapIter_Assoc( 0 )
, mMapIter_Next( 0 )
{
  if ( ioMap )
  {
    if ( ioMap->GoodMap() )
    {
      mMapIter_Map = ioMap;
      mMapIter_Seed = ioMap->mMap_Seed;
    }
    else ioMap->NewBadMapError(ev);
  }
  else ev->NilPointerError();
}

void
morkMapIter::CloseMapIter(morkEnv* ev)
{
  MORK_USED_1(ev);
  mMapIter_Map = 0;
  mMapIter_Seed = 0;
  
  mMapIter_Bucket = 0;
  mMapIter_AssocRef = 0;
  mMapIter_Assoc = 0;
  mMapIter_Next = 0;
}

mork_change*
morkMapIter::First(morkEnv* ev, void* outKey, void* outVal)
{
  mork_change* outFirst = 0;
  
  morkMap* map = mMapIter_Map;
  
  if ( map && map->GoodMap() ) 
  {
    morkAssoc** bucket = map->mMap_Buckets;
    morkAssoc** end = bucket + map->mMap_Slots; 
 
    mMapIter_Seed = map->mMap_Seed; 
   
    while ( bucket < end ) 
    {
      morkAssoc* assoc = *bucket++;
      if ( assoc ) 
      {
        mork_pos i = assoc - map->mMap_Assocs;
        mork_change* c = map->mMap_Changes;
        outFirst = ( c )? (c + i) : map->FormDummyChange();
        
        mMapIter_Assoc = assoc; 
        mMapIter_Next = assoc->mAssoc_Next; 
        mMapIter_Bucket = --bucket; 
        mMapIter_AssocRef = bucket; 
        
        map->get_assoc(outKey, outVal, i);
          
        break; 
      }
    }
  }
  else map->NewBadMapError(ev);

  return outFirst;
}

mork_change*
morkMapIter::Next(morkEnv* ev, void* outKey, void* outVal)
{
  mork_change* outNext = 0;
  
  morkMap* map = mMapIter_Map;
  
  if ( map && map->GoodMap() ) 
  {
    if ( mMapIter_Seed == map->mMap_Seed ) 
    {
      morkAssoc* here = mMapIter_Assoc; 
      if ( here ) 
      {
        morkAssoc* next = mMapIter_Next;
        morkAssoc* assoc = next;   
        if ( next ) 
        {
          morkAssoc** ref = mMapIter_AssocRef;
          
          




          if ( *ref != next ) 
            mMapIter_AssocRef = &here->mAssoc_Next;
            
          mMapIter_Next = next->mAssoc_Next;
        }
        else 
        {
          morkAssoc** bucket = map->mMap_Buckets;
          morkAssoc** end = bucket + map->mMap_Slots; 
          mMapIter_Assoc = 0; 
          bucket = mMapIter_Bucket; 
          mMapIter_Assoc = 0; 
          
          while ( ++bucket < end ) 
          {
            assoc = *bucket;
            if ( assoc ) 
            {
              mMapIter_Bucket = bucket;
              mMapIter_AssocRef = bucket; 
              mMapIter_Next = assoc->mAssoc_Next; 
              
              break; 
            }
          }
        }
        if ( assoc ) 
        {
          mMapIter_Assoc = assoc; 
          mork_pos i = assoc - map->mMap_Assocs;
          mork_change* c = map->mMap_Changes;
          outNext = ( c )? (c + i) : map->FormDummyChange();
        
          map->get_assoc( outKey, outVal, i);
        }
      }
    }
    else map->NewIterOutOfSyncError(ev);
  }
  else map->NewBadMapError(ev);
  
  return outNext;
}

mork_change*
morkMapIter::Here(morkEnv* ev, void* outKey, void* outVal)
{
  mork_change* outHere = 0;
  
  morkMap* map = mMapIter_Map;
  
  if ( map && map->GoodMap() ) 
  {
    if ( mMapIter_Seed == map->mMap_Seed ) 
    {
      morkAssoc* here = mMapIter_Assoc; 
      if ( here ) 
      {
        mork_pos i = here - map->mMap_Assocs;
        mork_change* c = map->mMap_Changes;
        outHere = ( c )? (c + i) : map->FormDummyChange();
          
        map->get_assoc(outKey, outVal, i);
      }
    }
    else map->NewIterOutOfSyncError(ev);
  }
  else map->NewBadMapError(ev);
  
  return outHere;
}

mork_change*
morkMapIter::CutHere(morkEnv* ev, void* outKey, void* outVal)
{
  mork_change* outCutHere = 0;
  morkMap* map = mMapIter_Map;
  
  if ( map && map->GoodMap() ) 
  {
    if ( mMapIter_Seed == map->mMap_Seed ) 
    {
      morkAssoc* here = mMapIter_Assoc; 
      if ( here ) 
      {
        morkAssoc** ref = mMapIter_AssocRef;
        if ( *ref != mMapIter_Next ) 
        {
          mork_pos i = here - map->mMap_Assocs;
          mork_change* c = map->mMap_Changes;
          outCutHere = ( c )? (c + i) : map->FormDummyChange();
          if ( outKey || outVal )
            map->get_assoc(outKey, outVal, i);
            
          map->push_free_assoc(here); 
          *ref = mMapIter_Next; 
          
          
          mMapIter_Seed = ++map->mMap_Seed; 
          
          if ( map->mMap_Fill ) 
            --map->mMap_Fill; 
          else
            map->NewSlotsUnderflowWarning(ev);
        }
      }
    }
    else map->NewIterOutOfSyncError(ev);
  }
  else map->NewBadMapError(ev);
  
  return outCutHere;
}


