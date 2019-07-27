








#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pldhash.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/MathAlgorithms.h"
#include "nsDebug.h"     
#include "nsAlgorithm.h"
#include "mozilla/Likely.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/ChaosMode.h"

#ifdef PL_DHASHMETER
# define METER(x)       x
#else
# define METER(x)
#endif






#ifdef DEBUG









#define IMMUTABLE_RECURSION_LEVEL UINT32_MAX

#define RECURSION_LEVEL_SAFE_TO_FINISH(table_)                                \
    (table_->mRecursionLevel == 0 ||                                          \
     table_->mRecursionLevel == IMMUTABLE_RECURSION_LEVEL)

#define INCREMENT_RECURSION_LEVEL(table_)                                     \
    do {                                                                      \
        if (table_->mRecursionLevel != IMMUTABLE_RECURSION_LEVEL) {           \
            const uint32_t oldRecursionLevel = table_->mRecursionLevel++;     \
            MOZ_ASSERT(oldRecursionLevel < IMMUTABLE_RECURSION_LEVEL - 1);    \
        }                                                                     \
    } while(0)
#define DECREMENT_RECURSION_LEVEL(table_)                                     \
    do {                                                                      \
        if (table_->mRecursionLevel != IMMUTABLE_RECURSION_LEVEL) {           \
            const uint32_t oldRecursionLevel = table_->mRecursionLevel--;     \
            MOZ_ASSERT(oldRecursionLevel > 0);                                \
        }                                                                     \
    } while(0)

#else

#define INCREMENT_RECURSION_LEVEL(table_)   do { } while(0)
#define DECREMENT_RECURSION_LEVEL(table_)   do { } while(0)

#endif 

using namespace mozilla;

PLDHashNumber
PL_DHashStringKey(PLDHashTable* aTable, const void* aKey)
{
  return HashString(static_cast<const char*>(aKey));
}

PLDHashNumber
PL_DHashVoidPtrKeyStub(PLDHashTable* aTable, const void* aKey)
{
  return (PLDHashNumber)(ptrdiff_t)aKey >> 2;
}

bool
PL_DHashMatchEntryStub(PLDHashTable* aTable,
                       const PLDHashEntryHdr* aEntry,
                       const void* aKey)
{
  const PLDHashEntryStub* stub = (const PLDHashEntryStub*)aEntry;

  return stub->key == aKey;
}

bool
PL_DHashMatchStringKey(PLDHashTable* aTable,
                       const PLDHashEntryHdr* aEntry,
                       const void* aKey)
{
  const PLDHashEntryStub* stub = (const PLDHashEntryStub*)aEntry;

  
  return stub->key == aKey ||
         (stub->key && aKey &&
          strcmp((const char*)stub->key, (const char*)aKey) == 0);
}

MOZ_ALWAYS_INLINE void
PLDHashTable::MoveEntryStub(const PLDHashEntryHdr* aFrom,
                            PLDHashEntryHdr* aTo)
{
  memcpy(aTo, aFrom, mEntrySize);
}

void
PL_DHashMoveEntryStub(PLDHashTable* aTable,
                      const PLDHashEntryHdr* aFrom,
                      PLDHashEntryHdr* aTo)
{
  aTable->MoveEntryStub(aFrom, aTo);
}

MOZ_ALWAYS_INLINE void
PLDHashTable::ClearEntryStub(PLDHashEntryHdr* aEntry)
{
  memset(aEntry, 0, mEntrySize);
}

void
PL_DHashClearEntryStub(PLDHashTable* aTable, PLDHashEntryHdr* aEntry)
{
  aTable->ClearEntryStub(aEntry);
}

MOZ_ALWAYS_INLINE void
PLDHashTable::FreeStringKey(PLDHashEntryHdr* aEntry)
{
  const PLDHashEntryStub* stub = (const PLDHashEntryStub*)aEntry;

  free((void*)stub->key);
  memset(aEntry, 0, mEntrySize);
}

void
PL_DHashFreeStringKey(PLDHashTable* aTable, PLDHashEntryHdr* aEntry)
{
  aTable->FreeStringKey(aEntry);
}

static const PLDHashTableOps stub_ops = {
  PL_DHashVoidPtrKeyStub,
  PL_DHashMatchEntryStub,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  nullptr
};

const PLDHashTableOps*
PL_DHashGetStubOps(void)
{
  return &stub_ops;
}

static bool
SizeOfEntryStore(uint32_t aCapacity, uint32_t aEntrySize, uint32_t* aNbytes)
{
  uint64_t nbytes64 = uint64_t(aCapacity) * uint64_t(aEntrySize);
  *aNbytes = aCapacity * aEntrySize;
  return uint64_t(*aNbytes) == nbytes64;   
}

PLDHashTable*
PL_NewDHashTable(const PLDHashTableOps* aOps, uint32_t aEntrySize,
                 uint32_t aLength)
{
  PLDHashTable* table = new PLDHashTable();
  PL_DHashTableInit(table, aOps, aEntrySize, aLength);
  return table;
}

void
PL_DHashTableDestroy(PLDHashTable* aTable)
{
  PL_DHashTableFinish(aTable);
  delete aTable;
}








static inline uint32_t
MaxLoad(uint32_t aCapacity)
{
  return aCapacity - (aCapacity >> 2);  
}
static inline uint32_t
MaxLoadOnGrowthFailure(uint32_t aCapacity)
{
  return aCapacity - (aCapacity >> 5);  
}
static inline uint32_t
MinLoad(uint32_t aCapacity)
{
  return aCapacity >> 2;                
}

static inline uint32_t
MinCapacity(uint32_t aLength)
{
  return (aLength * 4 + (3 - 1)) / 3;   
}

MOZ_ALWAYS_INLINE void
PLDHashTable::Init(const PLDHashTableOps* aOps,
                   uint32_t aEntrySize, uint32_t aLength)
{
  MOZ_ASSERT(!IsInitialized());

  
  MOZ_ASSERT(mOps == nullptr);
  MOZ_ASSERT(mRecursionLevel == 0);
  MOZ_ASSERT(mEntryStore == nullptr);

  if (aLength > PL_DHASH_MAX_INITIAL_LENGTH) {
    MOZ_CRASH("Initial length is too large");
  }

  
  
  uint32_t capacity = MinCapacity(aLength);
  if (capacity < PL_DHASH_MIN_CAPACITY) {
    capacity = PL_DHASH_MIN_CAPACITY;
  }

  int log2 = CeilingLog2(capacity);

  capacity = 1u << log2;
  MOZ_ASSERT(capacity <= PL_DHASH_MAX_CAPACITY);
  mOps = aOps;
  mHashShift = PL_DHASH_BITS - log2;
  mEntrySize = aEntrySize;
  mEntryCount = mRemovedCount = 0;
  mGeneration = 0;
  uint32_t nbytes;
  if (!SizeOfEntryStore(capacity, aEntrySize, &nbytes)) {
    MOZ_CRASH("Initial entry store size is too large");
  }

  mEntryStore = nullptr;

  METER(memset(&mStats, 0, sizeof(mStats)));

#ifdef DEBUG
  mRecursionLevel = 0;
#endif
}

void
PL_DHashTableInit(PLDHashTable* aTable, const PLDHashTableOps* aOps,
                  uint32_t aEntrySize, uint32_t aLength)
{
  aTable->Init(aOps, aEntrySize, aLength);
}





#define HASH1(hash0, shift)         ((hash0) >> (shift))
#define HASH2(hash0,log2,shift)     ((((hash0) << (log2)) >> (shift)) | 1)









#define COLLISION_FLAG              ((PLDHashNumber) 1)
#define MARK_ENTRY_FREE(entry)      ((entry)->mKeyHash = 0)
#define MARK_ENTRY_REMOVED(entry)   ((entry)->mKeyHash = 1)
#define ENTRY_IS_REMOVED(entry)     ((entry)->mKeyHash == 1)
#define ENTRY_IS_LIVE(entry)        ((entry)->mKeyHash >= 2)
#define ENSURE_LIVE_KEYHASH(hash0)  if (hash0 < 2) hash0 -= 2; else (void)0


#define MATCH_ENTRY_KEYHASH(entry,hash0) \
    (((entry)->mKeyHash & ~COLLISION_FLAG) == (hash0))


#define ADDRESS_ENTRY(table, index) \
    ((PLDHashEntryHdr *)((table)->mEntryStore + (index) * (table)->mEntrySize))

 MOZ_ALWAYS_INLINE bool
PLDHashTable::EntryIsFree(PLDHashEntryHdr* aEntry)
{
  return aEntry->mKeyHash == 0;
}

MOZ_ALWAYS_INLINE void
PLDHashTable::Finish()
{
  MOZ_ASSERT(IsInitialized());

  INCREMENT_RECURSION_LEVEL(this);

  
  char* entryAddr = mEntryStore;
  char* entryLimit = entryAddr + Capacity() * mEntrySize;
  while (entryAddr < entryLimit) {
    PLDHashEntryHdr* entry = (PLDHashEntryHdr*)entryAddr;
    if (ENTRY_IS_LIVE(entry)) {
      METER(mStats.mRemoveEnums++);
      mOps->clearEntry(this, entry);
    }
    entryAddr += mEntrySize;
  }

  mOps = nullptr;

  DECREMENT_RECURSION_LEVEL(this);
  MOZ_ASSERT(RECURSION_LEVEL_SAFE_TO_FINISH(this));

  
  free(mEntryStore);
  mEntryStore = nullptr;
}

void
PL_DHashTableFinish(PLDHashTable* aTable)
{
  aTable->Finish();
}






template <PLDHashTable::SearchReason Reason>
PLDHashEntryHdr* PL_DHASH_FASTCALL
PLDHashTable::SearchTable(const void* aKey, PLDHashNumber aKeyHash)
{
  MOZ_ASSERT(mEntryStore);
  METER(mStats.mSearches++);
  NS_ASSERTION(!(aKeyHash & COLLISION_FLAG),
               "!(aKeyHash & COLLISION_FLAG)");

  
  PLDHashNumber hash1 = HASH1(aKeyHash, mHashShift);
  PLDHashEntryHdr* entry = ADDRESS_ENTRY(this, hash1);

  
  if (EntryIsFree(entry)) {
    METER(mStats.mMisses++);
    return (Reason == ForAdd) ? entry : nullptr;
  }

  
  PLDHashMatchEntry matchEntry = mOps->matchEntry;
  if (MATCH_ENTRY_KEYHASH(entry, aKeyHash) &&
      matchEntry(this, entry, aKey)) {
    METER(mStats.mHits++);
    return entry;
  }

  
  int sizeLog2 = PL_DHASH_BITS - mHashShift;
  PLDHashNumber hash2 = HASH2(aKeyHash, sizeLog2, mHashShift);
  uint32_t sizeMask = (1u << sizeLog2) - 1;

  



  PLDHashEntryHdr* firstRemoved = nullptr;

  for (;;) {
    if (Reason == ForAdd) {
      if (MOZ_UNLIKELY(ENTRY_IS_REMOVED(entry))) {
        if (!firstRemoved) {
          firstRemoved = entry;
        }
      } else {
        entry->mKeyHash |= COLLISION_FLAG;
      }
    }

    METER(mStats.mSteps++);
    hash1 -= hash2;
    hash1 &= sizeMask;

    entry = ADDRESS_ENTRY(this, hash1);
    if (EntryIsFree(entry)) {
      METER(mStats.mMisses++);
      return (Reason == ForAdd) ? (firstRemoved ? firstRemoved : entry)
                                : nullptr;
    }

    if (MATCH_ENTRY_KEYHASH(entry, aKeyHash) &&
        matchEntry(this, entry, aKey)) {
      METER(mStats.mHits++);
      return entry;
    }
  }

  
  return nullptr;
}











PLDHashEntryHdr* PL_DHASH_FASTCALL
PLDHashTable::FindFreeEntry(PLDHashNumber aKeyHash)
{
  METER(mStats.mSearches++);
  MOZ_ASSERT(mEntryStore);
  NS_ASSERTION(!(aKeyHash & COLLISION_FLAG),
               "!(aKeyHash & COLLISION_FLAG)");

  
  PLDHashNumber hash1 = HASH1(aKeyHash, mHashShift);
  PLDHashEntryHdr* entry = ADDRESS_ENTRY(this, hash1);

  
  if (EntryIsFree(entry)) {
    METER(mStats.mMisses++);
    return entry;
  }

  
  int sizeLog2 = PL_DHASH_BITS - mHashShift;
  PLDHashNumber hash2 = HASH2(aKeyHash, sizeLog2, mHashShift);
  uint32_t sizeMask = (1u << sizeLog2) - 1;

  for (;;) {
    NS_ASSERTION(!ENTRY_IS_REMOVED(entry),
                 "!ENTRY_IS_REMOVED(entry)");
    entry->mKeyHash |= COLLISION_FLAG;

    METER(mStats.mSteps++);
    hash1 -= hash2;
    hash1 &= sizeMask;

    entry = ADDRESS_ENTRY(this, hash1);
    if (EntryIsFree(entry)) {
      METER(mStats.mMisses++);
      return entry;
    }
  }

  
  return nullptr;
}

bool
PLDHashTable::ChangeTable(int aDeltaLog2)
{
  MOZ_ASSERT(mEntryStore);

  
  int oldLog2 = PL_DHASH_BITS - mHashShift;
  int newLog2 = oldLog2 + aDeltaLog2;
  uint32_t newCapacity = 1u << newLog2;
  if (newCapacity > PL_DHASH_MAX_CAPACITY) {
    return false;
  }

  uint32_t nbytes;
  if (!SizeOfEntryStore(newCapacity, mEntrySize, &nbytes)) {
    return false;   
  }

  char* newEntryStore = (char*)malloc(nbytes);
  if (!newEntryStore) {
    return false;
  }

  
  mHashShift = PL_DHASH_BITS - newLog2;
  mRemovedCount = 0;
  mGeneration++;

  
  memset(newEntryStore, 0, nbytes);
  char* oldEntryStore;
  char* oldEntryAddr;
  oldEntryAddr = oldEntryStore = mEntryStore;
  mEntryStore = newEntryStore;
  PLDHashMoveEntry moveEntry = mOps->moveEntry;

  
  uint32_t oldCapacity = 1u << oldLog2;
  for (uint32_t i = 0; i < oldCapacity; ++i) {
    PLDHashEntryHdr* oldEntry = (PLDHashEntryHdr*)oldEntryAddr;
    if (ENTRY_IS_LIVE(oldEntry)) {
      oldEntry->mKeyHash &= ~COLLISION_FLAG;
      PLDHashEntryHdr* newEntry = FindFreeEntry(oldEntry->mKeyHash);
      NS_ASSERTION(EntryIsFree(newEntry), "EntryIsFree(newEntry)");
      moveEntry(this, oldEntry, newEntry);
      newEntry->mKeyHash = oldEntry->mKeyHash;
    }
    oldEntryAddr += mEntrySize;
  }

  free(oldEntryStore);
  return true;
}

MOZ_ALWAYS_INLINE PLDHashNumber
PLDHashTable::ComputeKeyHash(const void* aKey)
{
  MOZ_ASSERT(mEntryStore);

  PLDHashNumber keyHash = mOps->hashKey(this, aKey);
  keyHash *= PL_DHASH_GOLDEN_RATIO;

  
  ENSURE_LIVE_KEYHASH(keyHash);
  keyHash &= ~COLLISION_FLAG;

  return keyHash;
}

MOZ_ALWAYS_INLINE PLDHashEntryHdr*
PLDHashTable::Search(const void* aKey)
{
  MOZ_ASSERT(IsInitialized());

  INCREMENT_RECURSION_LEVEL(this);

  METER(mStats.mSearches++);

  PLDHashEntryHdr* entry =
    mEntryStore ? SearchTable<ForSearchOrRemove>(aKey, ComputeKeyHash(aKey))
                : nullptr;

  DECREMENT_RECURSION_LEVEL(this);

  return entry;
}

MOZ_ALWAYS_INLINE PLDHashEntryHdr*
PLDHashTable::Add(const void* aKey, const mozilla::fallible_t&)
{
  MOZ_ASSERT(IsInitialized());

  PLDHashNumber keyHash;
  PLDHashEntryHdr* entry;
  uint32_t capacity;

  MOZ_ASSERT(mRecursionLevel == 0);
  INCREMENT_RECURSION_LEVEL(this);

  
  if (!mEntryStore) {
    uint32_t nbytes;
    
    MOZ_RELEASE_ASSERT(SizeOfEntryStore(CapacityFromHashShift(), mEntrySize,
                                        &nbytes));
    mEntryStore = (char*)malloc(nbytes);
    if (!mEntryStore) {
      METER(mStats.mAddFailures++);
      entry = nullptr;
      goto exit;
    }
    memset(mEntryStore, 0, nbytes);
  }

  




  capacity = Capacity();
  if (mEntryCount + mRemovedCount >= MaxLoad(capacity)) {
    
    int deltaLog2;
    if (mRemovedCount >= capacity >> 2) {
      METER(mStats.mCompresses++);
      deltaLog2 = 0;
    } else {
      METER(mStats.mGrows++);
      deltaLog2 = 1;
    }

    




    if (!ChangeTable(deltaLog2) &&
        mEntryCount + mRemovedCount >= MaxLoadOnGrowthFailure(capacity)) {
      METER(mStats.mAddFailures++);
      entry = nullptr;
      goto exit;
    }
  }

  



  keyHash = ComputeKeyHash(aKey);
  entry = SearchTable<ForAdd>(aKey, keyHash);
  if (!ENTRY_IS_LIVE(entry)) {
    
    METER(mStats.mAddMisses++);
    if (ENTRY_IS_REMOVED(entry)) {
      METER(mStats.mAddOverRemoved++);
      mRemovedCount--;
      keyHash |= COLLISION_FLAG;
    }
    if (mOps->initEntry) {
      mOps->initEntry(entry, aKey);
    }
    entry->mKeyHash = keyHash;
    mEntryCount++;
  }
  METER(else {
    mStats.mAddHits++;
  });

exit:
  DECREMENT_RECURSION_LEVEL(this);
  return entry;
}

MOZ_ALWAYS_INLINE PLDHashEntryHdr*
PLDHashTable::Add(const void* aKey)
{
  PLDHashEntryHdr* entry = Add(aKey, fallible);
  if (!entry) {
    if (!mEntryStore) {
      
      uint32_t nbytes;
      (void) SizeOfEntryStore(CapacityFromHashShift(), mEntrySize, &nbytes);
      NS_ABORT_OOM(nbytes);
    } else {
      
      
      
      
      NS_ABORT_OOM(2 * EntrySize() * EntryCount());
    }
  }
  return entry;
}

MOZ_ALWAYS_INLINE void
PLDHashTable::Remove(const void* aKey)
{
  MOZ_ASSERT(IsInitialized());

  MOZ_ASSERT(mRecursionLevel == 0);
  INCREMENT_RECURSION_LEVEL(this);

  PLDHashEntryHdr* entry =
    mEntryStore ? SearchTable<ForSearchOrRemove>(aKey, ComputeKeyHash(aKey))
                : nullptr;
  if (entry) {
    
    METER(mStats.mRemoveHits++);
    PL_DHashTableRawRemove(this, entry);

    
    uint32_t capacity = Capacity();
    if (capacity > PL_DHASH_MIN_CAPACITY &&
        mEntryCount <= MinLoad(capacity)) {
      METER(mStats.mShrinks++);
      (void) ChangeTable(-1);
    }
  }
  METER(else {
    mStats.mRemoveMisses++;
  });

  DECREMENT_RECURSION_LEVEL(this);
}

PLDHashEntryHdr* PL_DHASH_FASTCALL
PL_DHashTableSearch(PLDHashTable* aTable, const void* aKey)
{
  return aTable->Search(aKey);
}

PLDHashEntryHdr* PL_DHASH_FASTCALL
PL_DHashTableAdd(PLDHashTable* aTable, const void* aKey,
                 const fallible_t& aFallible)
{
  return aTable->Add(aKey, aFallible);
}

PLDHashEntryHdr* PL_DHASH_FASTCALL
PL_DHashTableAdd(PLDHashTable* aTable, const void* aKey)
{
  return aTable->Add(aKey);
}

void PL_DHASH_FASTCALL
PL_DHashTableRemove(PLDHashTable* aTable, const void* aKey)
{
  aTable->Remove(aKey);
}

MOZ_ALWAYS_INLINE void
PLDHashTable::RawRemove(PLDHashEntryHdr* aEntry)
{
  MOZ_ASSERT(IsInitialized());
  MOZ_ASSERT(mEntryStore);

  MOZ_ASSERT(mRecursionLevel != IMMUTABLE_RECURSION_LEVEL);

  NS_ASSERTION(ENTRY_IS_LIVE(aEntry), "ENTRY_IS_LIVE(aEntry)");

  
  PLDHashNumber keyHash = aEntry->mKeyHash;
  mOps->clearEntry(this, aEntry);
  if (keyHash & COLLISION_FLAG) {
    MARK_ENTRY_REMOVED(aEntry);
    mRemovedCount++;
  } else {
    METER(mStats.mRemoveFrees++);
    MARK_ENTRY_FREE(aEntry);
  }
  mEntryCount--;
}

void
PL_DHashTableRawRemove(PLDHashTable* aTable, PLDHashEntryHdr* aEntry)
{
  aTable->RawRemove(aEntry);
}

MOZ_ALWAYS_INLINE uint32_t
PLDHashTable::Enumerate(PLDHashEnumerator aEtor, void* aArg)
{
  MOZ_ASSERT(IsInitialized());

  if (!mEntryStore) {
    return 0;
  }

  INCREMENT_RECURSION_LEVEL(this);

  
  
  char* entryAddr = mEntryStore;
  uint32_t capacity = Capacity();
  uint32_t tableSize = capacity * mEntrySize;
  char* entryLimit = entryAddr + tableSize;
  uint32_t i = 0;
  bool didRemove = false;

  if (ChaosMode::isActive(ChaosMode::HashTableIteration)) {
    
    
    
    entryAddr += ChaosMode::randomUint32LessThan(capacity) * mEntrySize;
    if (entryAddr >= entryLimit) {
      entryAddr -= tableSize;
    }
  }

  for (uint32_t e = 0; e < capacity; ++e) {
    PLDHashEntryHdr* entry = (PLDHashEntryHdr*)entryAddr;
    if (ENTRY_IS_LIVE(entry)) {
      PLDHashOperator op = aEtor(this, entry, i++, aArg);
      if (op & PL_DHASH_REMOVE) {
        METER(mStats.mRemoveEnums++);
        PL_DHashTableRawRemove(this, entry);
        didRemove = true;
      }
      if (op & PL_DHASH_STOP) {
        break;
      }
    }
    entryAddr += mEntrySize;
    if (entryAddr >= entryLimit) {
      entryAddr -= tableSize;
    }
  }

  MOZ_ASSERT(!didRemove || mRecursionLevel == 1);

  






  if (didRemove &&
      (mRemovedCount >= capacity >> 2 ||
       (capacity > PL_DHASH_MIN_CAPACITY &&
        mEntryCount <= MinLoad(capacity)))) {
    METER(mStats.mEnumShrinks++);
    capacity = mEntryCount;
    capacity += capacity >> 1;
    if (capacity < PL_DHASH_MIN_CAPACITY) {
      capacity = PL_DHASH_MIN_CAPACITY;
    }

    uint32_t ceiling = CeilingLog2(capacity);
    ceiling -= PL_DHASH_BITS - mHashShift;

    (void) ChangeTable(ceiling);
  }

  DECREMENT_RECURSION_LEVEL(this);

  return i;
}

uint32_t
PL_DHashTableEnumerate(PLDHashTable* aTable, PLDHashEnumerator aEtor,
                       void* aArg)
{
  return aTable->Enumerate(aEtor, aArg);
}

struct SizeOfEntryExcludingThisArg
{
  size_t total;
  PLDHashSizeOfEntryExcludingThisFun sizeOfEntryExcludingThis;
  MallocSizeOf mallocSizeOf;
  void* arg;  
};

static PLDHashOperator
SizeOfEntryExcludingThisEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aHdr,
                                   uint32_t aNumber, void* aArg)
{
  SizeOfEntryExcludingThisArg* e = (SizeOfEntryExcludingThisArg*)aArg;
  e->total += e->sizeOfEntryExcludingThis(aHdr, e->mallocSizeOf, e->arg);
  return PL_DHASH_NEXT;
}

MOZ_ALWAYS_INLINE size_t
PLDHashTable::SizeOfExcludingThis(
    PLDHashSizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
    MallocSizeOf aMallocSizeOf, void* aArg ) const
{
  MOZ_ASSERT(IsInitialized());

  if (!mEntryStore) {
    return 0;
  }

  size_t n = 0;
  n += aMallocSizeOf(mEntryStore);
  if (aSizeOfEntryExcludingThis) {
    SizeOfEntryExcludingThisArg arg2 = {
      0, aSizeOfEntryExcludingThis, aMallocSizeOf, aArg
    };
    PL_DHashTableEnumerate(const_cast<PLDHashTable*>(this),
                           SizeOfEntryExcludingThisEnumerator, &arg2);
    n += arg2.total;
  }
  return n;
}

MOZ_ALWAYS_INLINE size_t
PLDHashTable::SizeOfIncludingThis(
    PLDHashSizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
    MallocSizeOf aMallocSizeOf, void* aArg ) const
{
  MOZ_ASSERT(IsInitialized());

  return aMallocSizeOf(this) +
         SizeOfExcludingThis(aSizeOfEntryExcludingThis, aMallocSizeOf, aArg);
}

size_t
PL_DHashTableSizeOfExcludingThis(
    const PLDHashTable* aTable,
    PLDHashSizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
    MallocSizeOf aMallocSizeOf, void* aArg )
{
  return aTable->SizeOfExcludingThis(aSizeOfEntryExcludingThis,
                                     aMallocSizeOf, aArg);
}

size_t
PL_DHashTableSizeOfIncludingThis(
    const PLDHashTable* aTable,
    PLDHashSizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
    MallocSizeOf aMallocSizeOf, void* aArg )
{
  return aTable->SizeOfIncludingThis(aSizeOfEntryExcludingThis,
                                     aMallocSizeOf, aArg);
}

PLDHashTable::Iterator::Iterator(const PLDHashTable* aTable)
: mTable(aTable),
  mEntryAddr(mTable->mEntryStore),
  mEntryOffset(0)
{
  MOZ_ASSERT(mTable->IsInitialized());

  
  
  INCREMENT_RECURSION_LEVEL(mTable);

  
  
  
  
  uint32_t capacity = mTable->Capacity();
  uint32_t tableSize = capacity * mTable->EntrySize();
  char* entryLimit = mEntryAddr + tableSize;

  if (ChaosMode::isActive(ChaosMode::HashTableIteration)) {
    
    
    
    mEntryAddr += ChaosMode::randomUint32LessThan(capacity) * mTable->mEntrySize;
    if (mEntryAddr >= entryLimit) {
      mEntryAddr -= tableSize;
    }
  }
}

PLDHashTable::Iterator::Iterator(const Iterator& aIterator)
: mTable(aIterator.mTable),
  mEntryAddr(aIterator.mEntryAddr),
  mEntryOffset(aIterator.mEntryOffset)
{
  MOZ_ASSERT(mTable->IsInitialized());

  
  
  INCREMENT_RECURSION_LEVEL(mTable);
}

PLDHashTable::Iterator::~Iterator()
{
  MOZ_ASSERT(mTable->IsInitialized());

  DECREMENT_RECURSION_LEVEL(mTable);
}

bool PLDHashTable::Iterator::HasMoreEntries() const
{
  MOZ_ASSERT(mTable->IsInitialized());

  
  
  
  
  return mEntryOffset < mTable->EntryCount();
}

PLDHashEntryHdr* PLDHashTable::Iterator::NextEntry()
{
  MOZ_ASSERT(HasMoreEntries());

  
  
  
  
  uint32_t capacity = mTable->Capacity();
  uint32_t tableSize = capacity * mTable->mEntrySize;
  char* entryLimit = mEntryAddr + tableSize;

  
  
  
  
  
  
  MOZ_ASSERT_IF(capacity > 0, mTable->mEntryStore);
  for (uint32_t e = 0; e < capacity; ++e) {
    PLDHashEntryHdr* entry = (PLDHashEntryHdr*)mEntryAddr;

    
    
    mEntryAddr += mTable->mEntrySize;
    if (mEntryAddr >= entryLimit) {
      mEntryAddr -= tableSize;
    }
    if (ENTRY_IS_LIVE(entry)) {
      ++mEntryOffset;
      return entry;
    }
  }

  
  
  
  MOZ_CRASH("Flagrant misuse of hashtable iterators not caught by checks.");
}

#ifdef DEBUG
MOZ_ALWAYS_INLINE void
PLDHashTable::MarkImmutable()
{
  MOZ_ASSERT(IsInitialized());

  mRecursionLevel = IMMUTABLE_RECURSION_LEVEL;
}

void
PL_DHashMarkTableImmutable(PLDHashTable* aTable)
{
  aTable->MarkImmutable();
}
#endif

#ifdef PL_DHASHMETER
#include <math.h>

void
PLDHashTable::DumpMeter(PLDHashEnumerator aDump, FILE* aFp)
{
  MOZ_ASSERT(IsInitialized());

  PLDHashNumber hash1, hash2, maxChainHash1, maxChainHash2;
  double sqsum, mean, variance, sigma;
  PLDHashEntryHdr* entry;

  char* entryAddr = mEntryStore;
  int sizeLog2 = PL_DHASH_BITS - mHashShift;
  uint32_t capacity = Capacity();
  uint32_t sizeMask = (1u << sizeLog2) - 1;
  uint32_t chainCount = 0, maxChainLen = 0;
  hash2 = 0;
  sqsum = 0;

  MOZ_ASSERT_IF(capacity > 0, mEntryStore);
  for (uint32_t i = 0; i < capacity; i++) {
    entry = (PLDHashEntryHdr*)entryAddr;
    entryAddr += mEntrySize;
    if (!ENTRY_IS_LIVE(entry)) {
      continue;
    }
    hash1 = HASH1(entry->mKeyHash & ~COLLISION_FLAG, mHashShift);
    PLDHashNumber saveHash1 = hash1;
    PLDHashEntryHdr* probe = ADDRESS_ENTRY(this, hash1);
    uint32_t chainLen = 1;
    if (probe == entry) {
      
      chainCount++;
    } else {
      hash2 = HASH2(entry->mKeyHash & ~COLLISION_FLAG, sizeLog2, mHashShift);
      do {
        chainLen++;
        hash1 -= hash2;
        hash1 &= sizeMask;
        probe = ADDRESS_ENTRY(this, hash1);
      } while (probe != entry);
    }
    sqsum += chainLen * chainLen;
    if (chainLen > maxChainLen) {
      maxChainLen = chainLen;
      maxChainHash1 = saveHash1;
      maxChainHash2 = hash2;
    }
  }

  if (mEntryCount && chainCount) {
    mean = (double)mEntryCount / chainCount;
    variance = chainCount * sqsum - mEntryCount * mEntryCount;
    if (variance < 0 || chainCount == 1) {
      variance = 0;
    } else {
      variance /= chainCount * (chainCount - 1);
    }
    sigma = sqrt(variance);
  } else {
    mean = sigma = 0;
  }

  fprintf(aFp, "Double hashing statistics:\n");
  fprintf(aFp, "      capacity (in entries): %u\n", Capacity());
  fprintf(aFp, "          number of entries: %u\n", mEntryCount);
  fprintf(aFp, "  number of removed entries: %u\n", mRemovedCount);
  fprintf(aFp, "         number of searches: %u\n", mStats.mSearches);
  fprintf(aFp, "             number of hits: %u\n", mStats.mHits);
  fprintf(aFp, "           number of misses: %u\n", mStats.mMisses);
  fprintf(aFp, "      mean steps per search: %g\n",
          mStats.mSearches ? (double)mStats.mSteps / mStats.mSearches : 0.);
  fprintf(aFp, "     mean hash chain length: %g\n", mean);
  fprintf(aFp, "         standard deviation: %g\n", sigma);
  fprintf(aFp, "  maximum hash chain length: %u\n", maxChainLen);
  fprintf(aFp, "         number of searches: %u\n", mStats.mSearches);
  fprintf(aFp, " adds that made a new entry: %u\n", mStats.mAddMisses);
  fprintf(aFp, "adds that recycled removeds: %u\n", mStats.mAddOverRemoved);
  fprintf(aFp, "   adds that found an entry: %u\n", mStats.mAddHits);
  fprintf(aFp, "               add failures: %u\n", mStats.mAddFailures);
  fprintf(aFp, "             useful removes: %u\n", mStats.mRemoveHits);
  fprintf(aFp, "            useless removes: %u\n", mStats.mRemoveMisses);
  fprintf(aFp, "removes that freed an entry: %u\n", mStats.mRemoveFrees);
  fprintf(aFp, "  removes while enumerating: %u\n", mStats.mRemoveEnums);
  fprintf(aFp, "            number of grows: %u\n", mStats.mGrows);
  fprintf(aFp, "          number of shrinks: %u\n", mStats.mShrinks);
  fprintf(aFp, "       number of compresses: %u\n", mStats.mCompresses);
  fprintf(aFp, "number of enumerate shrinks: %u\n", mStats.mEnumShrinks);

  if (aDump && maxChainLen && hash2) {
    fputs("Maximum hash chain:\n", aFp);
    hash1 = maxChainHash1;
    hash2 = maxChainHash2;
    entry = ADDRESS_ENTRY(this, hash1);
    uint32_t i = 0;
    do {
      if (aDump(this, entry, i++, aFp) != PL_DHASH_NEXT) {
        break;
      }
      hash1 -= hash2;
      hash1 &= sizeMask;
      entry = ADDRESS_ENTRY(this, hash1);
    } while (!EntryIsFree(entry));
  }
}

void
PL_DHashTableDumpMeter(PLDHashTable* aTable, PLDHashEnumerator aDump, FILE* aFp)
{
  aTable->DumpMeter(aDump, aFp);
}
#endif 
