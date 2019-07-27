








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









#define IMMUTABLE_RECURSION_LEVEL ((uint16_t)-1)

#define RECURSION_LEVEL_SAFE_TO_FINISH(table_)                                \
    (table_->recursionLevel == 0 ||                                           \
     table_->recursionLevel == IMMUTABLE_RECURSION_LEVEL)

#define INCREMENT_RECURSION_LEVEL(table_)                                     \
    do {                                                                      \
        if (table_->recursionLevel != IMMUTABLE_RECURSION_LEVEL)              \
            ++table_->recursionLevel;                                         \
    } while(0)
#define DECREMENT_RECURSION_LEVEL(table_)                                     \
    do {                                                                      \
        if (table_->recursionLevel != IMMUTABLE_RECURSION_LEVEL) {            \
            MOZ_ASSERT(table_->recursionLevel > 0);                           \
            --table_->recursionLevel;                                         \
        }                                                                     \
    } while(0)

#else

#define INCREMENT_RECURSION_LEVEL(table_)   do { } while(0)
#define DECREMENT_RECURSION_LEVEL(table_)   do { } while(0)

#endif 

using namespace mozilla;

void*
PL_DHashAllocTable(PLDHashTable* aTable, uint32_t aNBytes)
{
  return malloc(aNBytes);
}

void
PL_DHashFreeTable(PLDHashTable* aTable, void* aPtr)
{
  free(aPtr);
}

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
  memcpy(aTo, aFrom, entrySize);
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
  memset(aEntry, 0, entrySize);
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
  memset(aEntry, 0, entrySize);
}

void
PL_DHashFreeStringKey(PLDHashTable* aTable, PLDHashEntryHdr* aEntry)
{
  aTable->FreeStringKey(aEntry);
}

void
PL_DHashFinalizeStub(PLDHashTable* aTable)
{
}

static const PLDHashTableOps stub_ops = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  PL_DHashMatchEntryStub,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
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
PL_NewDHashTable(const PLDHashTableOps* aOps, void* aData, uint32_t aEntrySize,
                 uint32_t aLength)
{
  PLDHashTable* table = (PLDHashTable*)malloc(sizeof(*table));
  if (!table) {
    return nullptr;
  }
  if (!PL_DHashTableInit(table, aOps, aData, aEntrySize, fallible_t(),
                         aLength)) {
    free(table);
    return nullptr;
  }
  return table;
}

void
PL_DHashTableDestroy(PLDHashTable* aTable)
{
  PL_DHashTableFinish(aTable);
  free(aTable);
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

MOZ_ALWAYS_INLINE bool
PLDHashTable::Init(const PLDHashTableOps* aOps, void* aData,
                   uint32_t aEntrySize, const fallible_t&, uint32_t aLength)
{
#ifdef DEBUG
  if (aEntrySize > 16 * sizeof(void*)) {
    printf_stderr(
      "pldhash: for the table at address %p, the given aEntrySize"
      " of %lu definitely favors chaining over double hashing.\n",
      (void*)this,
      (unsigned long) aEntrySize);
  }
#endif

  if (aLength > PL_DHASH_MAX_INITIAL_LENGTH) {
    return false;
  }

  ops = aOps;
  data = aData;

  
  
  uint32_t capacity = MinCapacity(aLength);
  if (capacity < PL_DHASH_MIN_CAPACITY) {
    capacity = PL_DHASH_MIN_CAPACITY;
  }

  int log2 = CeilingLog2(capacity);

  capacity = 1u << log2;
  MOZ_ASSERT(capacity <= PL_DHASH_MAX_CAPACITY);
  hashShift = PL_DHASH_BITS - log2;
  entrySize = aEntrySize;
  entryCount = removedCount = 0;
  generation = 0;
  uint32_t nbytes;
  if (!SizeOfEntryStore(capacity, aEntrySize, &nbytes)) {
    return false;  
  }

  entryStore = (char*)aOps->allocTable(this, nbytes);
  if (!entryStore) {
    return false;
  }
  memset(entryStore, 0, nbytes);
  METER(memset(&stats, 0, sizeof(stats)));

#ifdef DEBUG
  recursionLevel = 0;
#endif

  return true;
}

bool
PL_DHashTableInit(PLDHashTable* aTable, const PLDHashTableOps* aOps,
                  void* aData, uint32_t aEntrySize,
                  const fallible_t& aFallible, uint32_t aLength)
{
  return aTable->Init(aOps, aData, aEntrySize, aFallible, aLength);
}

void
PL_DHashTableInit(PLDHashTable* aTable, const PLDHashTableOps* aOps,
                  void* aData, uint32_t aEntrySize, uint32_t aLength)
{
  if (!PL_DHashTableInit(aTable, aOps, aData, aEntrySize, fallible_t(),
                         aLength)) {
    if (aLength > PL_DHASH_MAX_INITIAL_LENGTH) {
      MOZ_CRASH();          
    }
    uint32_t capacity = MinCapacity(aLength), nbytes;
    if (!SizeOfEntryStore(capacity, aEntrySize, &nbytes)) {
      MOZ_CRASH();          
    }
    NS_ABORT_OOM(nbytes);   
  }
}





#define HASH1(hash0, shift)         ((hash0) >> (shift))
#define HASH2(hash0,log2,shift)     ((((hash0) << (log2)) >> (shift)) | 1)









#define COLLISION_FLAG              ((PLDHashNumber) 1)
#define MARK_ENTRY_FREE(entry)      ((entry)->keyHash = 0)
#define MARK_ENTRY_REMOVED(entry)   ((entry)->keyHash = 1)
#define ENTRY_IS_REMOVED(entry)     ((entry)->keyHash == 1)
#define ENTRY_IS_LIVE(entry)        ((entry)->keyHash >= 2)
#define ENSURE_LIVE_KEYHASH(hash0)  if (hash0 < 2) hash0 -= 2; else (void)0


#define MATCH_ENTRY_KEYHASH(entry,hash0) \
    (((entry)->keyHash & ~COLLISION_FLAG) == (hash0))


#define ADDRESS_ENTRY(table, index) \
    ((PLDHashEntryHdr *)((table)->entryStore + (index) * (table)->entrySize))

MOZ_ALWAYS_INLINE void
PLDHashTable::Finish()
{
  INCREMENT_RECURSION_LEVEL(this);

  
  ops->finalize(this);

  
  char* entryAddr = entryStore;
  char* entryLimit = entryAddr + Capacity() * entrySize;
  while (entryAddr < entryLimit) {
    PLDHashEntryHdr* entry = (PLDHashEntryHdr*)entryAddr;
    if (ENTRY_IS_LIVE(entry)) {
      METER(stats.removeEnums++);
      ops->clearEntry(this, entry);
    }
    entryAddr += entrySize;
  }

  DECREMENT_RECURSION_LEVEL(this);
  MOZ_ASSERT(RECURSION_LEVEL_SAFE_TO_FINISH(this));

  
  ops->freeTable(this, entryStore);
}

void
PL_DHashTableFinish(PLDHashTable* aTable)
{
  aTable->Finish();
}

PLDHashEntryHdr* PL_DHASH_FASTCALL
PLDHashTable::SearchTable(const void* aKey, PLDHashNumber aKeyHash,
                          PLDHashOperator aOp)
{
  METER(stats.searches++);
  NS_ASSERTION(!(aKeyHash & COLLISION_FLAG),
               "!(aKeyHash & COLLISION_FLAG)");

  
  PLDHashNumber hash1 = HASH1(aKeyHash, hashShift);
  PLDHashEntryHdr* entry = ADDRESS_ENTRY(this, hash1);

  
  if (PL_DHASH_ENTRY_IS_FREE(entry)) {
    METER(stats.misses++);
    return entry;
  }

  
  PLDHashMatchEntry matchEntry = ops->matchEntry;
  if (MATCH_ENTRY_KEYHASH(entry, aKeyHash) &&
      matchEntry(this, entry, aKey)) {
    METER(stats.hits++);
    return entry;
  }

  
  int sizeLog2 = PL_DHASH_BITS - hashShift;
  PLDHashNumber hash2 = HASH2(aKeyHash, sizeLog2, hashShift);
  uint32_t sizeMask = (1u << sizeLog2) - 1;

  
  PLDHashEntryHdr* firstRemoved = nullptr;

  for (;;) {
    if (MOZ_UNLIKELY(ENTRY_IS_REMOVED(entry))) {
      if (!firstRemoved) {
        firstRemoved = entry;
      }
    } else {
      if (aOp == PL_DHASH_ADD) {
        entry->keyHash |= COLLISION_FLAG;
      }
    }

    METER(stats.steps++);
    hash1 -= hash2;
    hash1 &= sizeMask;

    entry = ADDRESS_ENTRY(this, hash1);
    if (PL_DHASH_ENTRY_IS_FREE(entry)) {
      METER(stats.misses++);
      return (firstRemoved && aOp == PL_DHASH_ADD) ? firstRemoved : entry;
    }

    if (MATCH_ENTRY_KEYHASH(entry, aKeyHash) &&
        matchEntry(this, entry, aKey)) {
      METER(stats.hits++);
      return entry;
    }
  }

  
  return nullptr;
}











PLDHashEntryHdr* PL_DHASH_FASTCALL
PLDHashTable::FindFreeEntry(PLDHashNumber aKeyHash)
{
  METER(stats.searches++);
  NS_ASSERTION(!(aKeyHash & COLLISION_FLAG),
               "!(aKeyHash & COLLISION_FLAG)");

  
  PLDHashNumber hash1 = HASH1(aKeyHash, hashShift);
  PLDHashEntryHdr* entry = ADDRESS_ENTRY(this, hash1);

  
  if (PL_DHASH_ENTRY_IS_FREE(entry)) {
    METER(stats.misses++);
    return entry;
  }

  
  int sizeLog2 = PL_DHASH_BITS - hashShift;
  PLDHashNumber hash2 = HASH2(aKeyHash, sizeLog2, hashShift);
  uint32_t sizeMask = (1u << sizeLog2) - 1;

  for (;;) {
    NS_ASSERTION(!ENTRY_IS_REMOVED(entry),
                 "!ENTRY_IS_REMOVED(entry)");
    entry->keyHash |= COLLISION_FLAG;

    METER(stats.steps++);
    hash1 -= hash2;
    hash1 &= sizeMask;

    entry = ADDRESS_ENTRY(this, hash1);
    if (PL_DHASH_ENTRY_IS_FREE(entry)) {
      METER(stats.misses++);
      return entry;
    }
  }

  
  return nullptr;
}

bool
PLDHashTable::ChangeTable(int aDeltaLog2)
{
  
  int oldLog2 = PL_DHASH_BITS - hashShift;
  int newLog2 = oldLog2 + aDeltaLog2;
  uint32_t newCapacity = 1u << newLog2;
  if (newCapacity > PL_DHASH_MAX_CAPACITY) {
    return false;
  }

  uint32_t nbytes;
  if (!SizeOfEntryStore(newCapacity, entrySize, &nbytes)) {
    return false;   
  }

  char* newEntryStore = (char*)ops->allocTable(this, nbytes);
  if (!newEntryStore) {
    return false;
  }

  
#ifdef DEBUG
  uint32_t recursionLevelTmp = recursionLevel;
#endif
  hashShift = PL_DHASH_BITS - newLog2;
  removedCount = 0;
  generation++;

  
  memset(newEntryStore, 0, nbytes);
  char* oldEntryStore;
  char* oldEntryAddr;
  oldEntryAddr = oldEntryStore = entryStore;
  entryStore = newEntryStore;
  PLDHashMoveEntry moveEntry = ops->moveEntry;
#ifdef DEBUG
  recursionLevel = recursionLevelTmp;
#endif

  
  uint32_t oldCapacity = 1u << oldLog2;
  for (uint32_t i = 0; i < oldCapacity; ++i) {
    PLDHashEntryHdr* oldEntry = (PLDHashEntryHdr*)oldEntryAddr;
    if (ENTRY_IS_LIVE(oldEntry)) {
      oldEntry->keyHash &= ~COLLISION_FLAG;
      PLDHashEntryHdr* newEntry = FindFreeEntry(oldEntry->keyHash);
      NS_ASSERTION(PL_DHASH_ENTRY_IS_FREE(newEntry),
                   "PL_DHASH_ENTRY_IS_FREE(newEntry)");
      moveEntry(this, oldEntry, newEntry);
      newEntry->keyHash = oldEntry->keyHash;
    }
    oldEntryAddr += entrySize;
  }

  ops->freeTable(this, oldEntryStore);
  return true;
}

MOZ_ALWAYS_INLINE PLDHashEntryHdr*
PLDHashTable::Operate(const void* aKey, PLDHashOperator aOp)
{
  PLDHashEntryHdr* entry;

  MOZ_ASSERT(aOp == PL_DHASH_LOOKUP || recursionLevel == 0);
  INCREMENT_RECURSION_LEVEL(this);

  PLDHashNumber keyHash = ops->hashKey(this, aKey);
  keyHash *= PL_DHASH_GOLDEN_RATIO;

  
  ENSURE_LIVE_KEYHASH(keyHash);
  keyHash &= ~COLLISION_FLAG;

  switch (aOp) {
    case PL_DHASH_LOOKUP:
      METER(stats.lookups++);
      entry = SearchTable(aKey, keyHash, aOp);
      break;

    case PL_DHASH_ADD: {
      




      uint32_t capacity = Capacity();
      if (entryCount + removedCount >= MaxLoad(capacity)) {
        
        int deltaLog2;
        if (removedCount >= capacity >> 2) {
          METER(stats.compresses++);
          deltaLog2 = 0;
        } else {
          METER(stats.grows++);
          deltaLog2 = 1;
        }

        




        if (!ChangeTable(deltaLog2) &&
            entryCount + removedCount >=
            MaxLoadOnGrowthFailure(capacity)) {
          METER(stats.addFailures++);
          entry = nullptr;
          break;
        }
      }

      



      entry = SearchTable(aKey, keyHash, aOp);
      if (!ENTRY_IS_LIVE(entry)) {
        
        METER(stats.addMisses++);
        if (ENTRY_IS_REMOVED(entry)) {
          METER(stats.addOverRemoved++);
          removedCount--;
          keyHash |= COLLISION_FLAG;
        }
        if (ops->initEntry && !ops->initEntry(this, entry, aKey)) {
          
          memset(entry + 1, 0, entrySize - sizeof(*entry));
          entry = nullptr;
          break;
        }
        entry->keyHash = keyHash;
        entryCount++;
      }
      METER(else {
        stats.addHits++;
      });
      break;
    }

    case PL_DHASH_REMOVE:
      entry = SearchTable(aKey, keyHash, aOp);
      if (ENTRY_IS_LIVE(entry)) {
        
        METER(stats.removeHits++);
        PL_DHashTableRawRemove(this, entry);

        
        uint32_t capacity = Capacity();
        if (capacity > PL_DHASH_MIN_CAPACITY &&
            entryCount <= MinLoad(capacity)) {
          METER(stats.shrinks++);
          (void) ChangeTable(-1);
        }
      }
      METER(else {
        stats.removeMisses++;
      });
      entry = nullptr;
      break;

    default:
      NS_NOTREACHED("0");
      entry = nullptr;
  }

  DECREMENT_RECURSION_LEVEL(this);

  return entry;
}

PLDHashEntryHdr* PL_DHASH_FASTCALL
PL_DHashTableOperate(PLDHashTable* aTable, const void* aKey, PLDHashOperator aOp)
{
  return aTable->Operate(aKey, aOp);
}

MOZ_ALWAYS_INLINE void
PLDHashTable::RawRemove(PLDHashEntryHdr* aEntry)
{
  MOZ_ASSERT(recursionLevel != IMMUTABLE_RECURSION_LEVEL);

  NS_ASSERTION(ENTRY_IS_LIVE(aEntry), "ENTRY_IS_LIVE(aEntry)");

  
  PLDHashNumber keyHash = aEntry->keyHash;
  ops->clearEntry(this, aEntry);
  if (keyHash & COLLISION_FLAG) {
    MARK_ENTRY_REMOVED(aEntry);
    removedCount++;
  } else {
    METER(stats.removeFrees++);
    MARK_ENTRY_FREE(aEntry);
  }
  entryCount--;
}

void
PL_DHashTableRawRemove(PLDHashTable* aTable, PLDHashEntryHdr* aEntry)
{
  aTable->RawRemove(aEntry);
}

MOZ_ALWAYS_INLINE uint32_t
PLDHashTable::Enumerate(PLDHashEnumerator aEtor, void* aArg)
{
  INCREMENT_RECURSION_LEVEL(this);

  char* entryAddr = entryStore;
  uint32_t capacity = Capacity();
  uint32_t tableSize = capacity * entrySize;
  char* entryLimit = entryAddr + tableSize;
  uint32_t i = 0;
  bool didRemove = false;

  if (ChaosMode::isActive()) {
    
    
    
    entryAddr += ChaosMode::randomUint32LessThan(capacity) * entrySize;
    if (entryAddr >= entryLimit) {
      entryAddr -= tableSize;
    }
  }

  for (uint32_t e = 0; e < capacity; ++e) {
    PLDHashEntryHdr* entry = (PLDHashEntryHdr*)entryAddr;
    if (ENTRY_IS_LIVE(entry)) {
      PLDHashOperator op = aEtor(this, entry, i++, aArg);
      if (op & PL_DHASH_REMOVE) {
        METER(stats.removeEnums++);
        PL_DHashTableRawRemove(this, entry);
        didRemove = true;
      }
      if (op & PL_DHASH_STOP) {
        break;
      }
    }
    entryAddr += entrySize;
    if (entryAddr >= entryLimit) {
      entryAddr -= tableSize;
    }
  }

  MOZ_ASSERT(!didRemove || recursionLevel == 1);

  






  if (didRemove &&
      (removedCount >= capacity >> 2 ||
       (capacity > PL_DHASH_MIN_CAPACITY &&
        entryCount <= MinLoad(capacity)))) {
    METER(stats.enumShrinks++);
    capacity = entryCount;
    capacity += capacity >> 1;
    if (capacity < PL_DHASH_MIN_CAPACITY) {
      capacity = PL_DHASH_MIN_CAPACITY;
    }

    uint32_t ceiling = CeilingLog2(capacity);
    ceiling -= PL_DHASH_BITS - hashShift;

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
  size_t n = 0;
  n += aMallocSizeOf(entryStore);
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

#ifdef DEBUG
MOZ_ALWAYS_INLINE void
PLDHashTable::MarkImmutable()
{
  recursionLevel = IMMUTABLE_RECURSION_LEVEL;
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
  PLDHashNumber hash1, hash2, maxChainHash1, maxChainHash2;
  double sqsum, mean, variance, sigma;
  PLDHashEntryHdr* entry;

  char* entryAddr = entryStore;
  int sizeLog2 = PL_DHASH_BITS - hashShift;
  uint32_t capacity = Capacity();
  uint32_t sizeMask = (1u << sizeLog2) - 1;
  uint32_t chainCount = 0, maxChainLen = 0;
  hash2 = 0;
  sqsum = 0;

  for (uint32_t i = 0; i < capacity; i++) {
    entry = (PLDHashEntryHdr*)entryAddr;
    entryAddr += entrySize;
    if (!ENTRY_IS_LIVE(entry)) {
      continue;
    }
    hash1 = HASH1(entry->keyHash & ~COLLISION_FLAG, hashShift);
    PLDHashNumber saveHash1 = hash1;
    PLDHashEntryHdr* probe = ADDRESS_ENTRY(this, hash1);
    uint32_t chainLen = 1;
    if (probe == entry) {
      
      chainCount++;
    } else {
      hash2 = HASH2(entry->keyHash & ~COLLISION_FLAG, sizeLog2,
                    hashShift);
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

  if (entryCount && chainCount) {
    mean = (double)entryCount / chainCount;
    variance = chainCount * sqsum - entryCount * entryCount;
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
  fprintf(aFp, "    table size (in entries): %u\n", tableSize);
  fprintf(aFp, "          number of entries: %u\n", entryCount);
  fprintf(aFp, "  number of removed entries: %u\n", removedCount);
  fprintf(aFp, "         number of searches: %u\n", stats.searches);
  fprintf(aFp, "             number of hits: %u\n", stats.hits);
  fprintf(aFp, "           number of misses: %u\n", stats.misses);
  fprintf(aFp, "      mean steps per search: %g\n",
          stats.searches ? (double)stats.steps / stats.searches : 0.);
  fprintf(aFp, "     mean hash chain length: %g\n", mean);
  fprintf(aFp, "         standard deviation: %g\n", sigma);
  fprintf(aFp, "  maximum hash chain length: %u\n", maxChainLen);
  fprintf(aFp, "          number of lookups: %u\n", stats.lookups);
  fprintf(aFp, " adds that made a new entry: %u\n", stats.addMisses);
  fprintf(aFp, "adds that recycled removeds: %u\n", stats.addOverRemoved);
  fprintf(aFp, "   adds that found an entry: %u\n", stats.addHits);
  fprintf(aFp, "               add failures: %u\n", stats.addFailures);
  fprintf(aFp, "             useful removes: %u\n", stats.removeHits);
  fprintf(aFp, "            useless removes: %u\n", stats.removeMisses);
  fprintf(aFp, "removes that freed an entry: %u\n", stats.removeFrees);
  fprintf(aFp, "  removes while enumerating: %u\n", stats.removeEnums);
  fprintf(aFp, "            number of grows: %u\n", stats.grows);
  fprintf(aFp, "          number of shrinks: %u\n", stats.shrinks);
  fprintf(aFp, "       number of compresses: %u\n", stats.compresses);
  fprintf(aFp, "number of enumerate shrinks: %u\n", stats.enumShrinks);

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
    } while (PL_DHASH_ENTRY_IS_BUSY(entry));
  }
}

void
PL_DHashTableDumpMeter(PLDHashTable* aTable, PLDHashEnumerator aDump, FILE* aFp)
{
  aTable->DumpMeter(aDump, aFp);
}
#endif 
