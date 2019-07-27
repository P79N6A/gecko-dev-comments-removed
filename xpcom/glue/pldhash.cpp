








#include <new>
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

using namespace mozilla;

#ifdef DEBUG

class AutoReadOp
{
  Checker& mChk;
public:
  explicit AutoReadOp(Checker& aChk) : mChk(aChk) { mChk.StartReadOp(); }
  ~AutoReadOp() { mChk.EndReadOp(); }
};

class AutoWriteOp
{
  Checker& mChk;
public:
  explicit AutoWriteOp(Checker& aChk) : mChk(aChk) { mChk.StartWriteOp(); }
  ~AutoWriteOp() { mChk.EndWriteOp(); }
};

class AutoIteratorRemovalOp
{
  Checker& mChk;
public:
  explicit AutoIteratorRemovalOp(Checker& aChk)
    : mChk(aChk)
  {
    mChk.StartIteratorRemovalOp();
  }
  ~AutoIteratorRemovalOp() { mChk.EndIteratorRemovalOp(); }
};

class AutoDestructorOp
{
  Checker& mChk;
public:
  explicit AutoDestructorOp(Checker& aChk)
    : mChk(aChk)
  {
    mChk.StartDestructorOp();
  }
  ~AutoDestructorOp() { mChk.EndDestructorOp(); }
};

#endif

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






static inline void
BestCapacity(uint32_t aLength, uint32_t* aCapacityOut,
             uint32_t* aLog2CapacityOut)
{
  
  
  uint32_t capacity = (aLength * 4 + (3 - 1)) / 3; 
  if (capacity < PL_DHASH_MIN_CAPACITY) {
    capacity = PL_DHASH_MIN_CAPACITY;
  }

  
  uint32_t log2 = CeilingLog2(capacity);
  capacity = 1u << log2;
  MOZ_ASSERT(capacity <= PL_DHASH_MAX_CAPACITY);

  *aCapacityOut = capacity;
  *aLog2CapacityOut = log2;
}

static MOZ_ALWAYS_INLINE uint32_t
HashShift(uint32_t aEntrySize, uint32_t aLength)
{
  if (aLength > PL_DHASH_MAX_INITIAL_LENGTH) {
    MOZ_CRASH("Initial length is too large");
  }

  uint32_t capacity, log2;
  BestCapacity(aLength, &capacity, &log2);

  uint32_t nbytes;
  if (!SizeOfEntryStore(capacity, aEntrySize, &nbytes)) {
    MOZ_CRASH("Initial entry store size is too large");
  }

  
  return PL_DHASH_BITS - log2;
}

PLDHashTable::PLDHashTable(const PLDHashTableOps* aOps, uint32_t aEntrySize,
                           uint32_t aLength)
  : mOps(aOps)
  , mHashShift(HashShift(aEntrySize, aLength))
  , mEntrySize(aEntrySize)
  , mEntryCount(0)
  , mRemovedCount(0)
  , mGeneration(0)
  , mEntryStore(nullptr)
#ifdef DEBUG
  , mChecker()
#endif
{
}

PLDHashTable&
PLDHashTable::operator=(PLDHashTable&& aOther)
{
  if (this == &aOther) {
    return *this;
  }

  
  this->~PLDHashTable();

  
  
  
  
  
  MOZ_RELEASE_ASSERT(mOps == aOther.mOps);
  MOZ_RELEASE_ASSERT(mEntrySize == aOther.mEntrySize);

  
  mHashShift = Move(aOther.mHashShift);
  mEntryCount = Move(aOther.mEntryCount);
  mRemovedCount = Move(aOther.mRemovedCount);
  mGeneration = Move(aOther.mGeneration);
  mEntryStore = Move(aOther.mEntryStore);
#ifdef DEBUG
  mChecker = Move(aOther.mChecker);
#endif

  
  {
#ifdef DEBUG
    AutoDestructorOp op(mChecker);
#endif
    aOther.mEntryStore = nullptr;
  }

  return *this;
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

PLDHashTable::~PLDHashTable()
{
#ifdef DEBUG
  AutoDestructorOp op(mChecker);
#endif

  if (!mEntryStore) {
    return;
  }

  
  char* entryAddr = mEntryStore;
  char* entryLimit = entryAddr + Capacity() * mEntrySize;
  while (entryAddr < entryLimit) {
    PLDHashEntryHdr* entry = (PLDHashEntryHdr*)entryAddr;
    if (ENTRY_IS_LIVE(entry)) {
      mOps->clearEntry(this, entry);
    }
    entryAddr += mEntrySize;
  }

  
  free(mEntryStore);
  mEntryStore = nullptr;
}

void
PLDHashTable::ClearAndPrepareForLength(uint32_t aLength)
{
  
  const PLDHashTableOps* ops = mOps;
  uint32_t entrySize = mEntrySize;

  this->~PLDHashTable();
  new (this) PLDHashTable(ops, entrySize, aLength);
}

void
PLDHashTable::Clear()
{
  ClearAndPrepareForLength(PL_DHASH_DEFAULT_INITIAL_LENGTH);
}






template <PLDHashTable::SearchReason Reason>
PLDHashEntryHdr* PL_DHASH_FASTCALL
PLDHashTable::SearchTable(const void* aKey, PLDHashNumber aKeyHash)
{
  MOZ_ASSERT(mEntryStore);
  NS_ASSERTION(!(aKeyHash & COLLISION_FLAG),
               "!(aKeyHash & COLLISION_FLAG)");

  
  PLDHashNumber hash1 = HASH1(aKeyHash, mHashShift);
  PLDHashEntryHdr* entry = ADDRESS_ENTRY(this, hash1);

  
  if (EntryIsFree(entry)) {
    return (Reason == ForAdd) ? entry : nullptr;
  }

  
  PLDHashMatchEntry matchEntry = mOps->matchEntry;
  if (MATCH_ENTRY_KEYHASH(entry, aKeyHash) &&
      matchEntry(this, entry, aKey)) {
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

    hash1 -= hash2;
    hash1 &= sizeMask;

    entry = ADDRESS_ENTRY(this, hash1);
    if (EntryIsFree(entry)) {
      return (Reason == ForAdd) ? (firstRemoved ? firstRemoved : entry)
                                : nullptr;
    }

    if (MATCH_ENTRY_KEYHASH(entry, aKeyHash) &&
        matchEntry(this, entry, aKey)) {
      return entry;
    }
  }

  
  return nullptr;
}











PLDHashEntryHdr* PL_DHASH_FASTCALL
PLDHashTable::FindFreeEntry(PLDHashNumber aKeyHash)
{
  MOZ_ASSERT(mEntryStore);
  NS_ASSERTION(!(aKeyHash & COLLISION_FLAG),
               "!(aKeyHash & COLLISION_FLAG)");

  
  PLDHashNumber hash1 = HASH1(aKeyHash, mHashShift);
  PLDHashEntryHdr* entry = ADDRESS_ENTRY(this, hash1);

  
  if (EntryIsFree(entry)) {
    return entry;
  }

  
  int sizeLog2 = PL_DHASH_BITS - mHashShift;
  PLDHashNumber hash2 = HASH2(aKeyHash, sizeLog2, mHashShift);
  uint32_t sizeMask = (1u << sizeLog2) - 1;

  for (;;) {
    NS_ASSERTION(!ENTRY_IS_REMOVED(entry),
                 "!ENTRY_IS_REMOVED(entry)");
    entry->mKeyHash |= COLLISION_FLAG;

    hash1 -= hash2;
    hash1 &= sizeMask;

    entry = ADDRESS_ENTRY(this, hash1);
    if (EntryIsFree(entry)) {
      return entry;
    }
  }

  
  return nullptr;
}

bool
PLDHashTable::ChangeTable(int32_t aDeltaLog2)
{
  MOZ_ASSERT(mEntryStore);

  
  int32_t oldLog2 = PL_DHASH_BITS - mHashShift;
  int32_t newLog2 = oldLog2 + aDeltaLog2;
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
#ifdef DEBUG
  AutoReadOp op(mChecker);
#endif

  PLDHashEntryHdr* entry =
    mEntryStore ? SearchTable<ForSearchOrRemove>(aKey, ComputeKeyHash(aKey))
                : nullptr;

  return entry;
}

MOZ_ALWAYS_INLINE PLDHashEntryHdr*
PLDHashTable::Add(const void* aKey, const mozilla::fallible_t&)
{
#ifdef DEBUG
  AutoWriteOp op(mChecker);
#endif

  
  if (!mEntryStore) {
    uint32_t nbytes;
    
    MOZ_RELEASE_ASSERT(SizeOfEntryStore(CapacityFromHashShift(), mEntrySize,
                                        &nbytes));
    mEntryStore = (char*)malloc(nbytes);
    if (!mEntryStore) {
      return nullptr;
    }
    memset(mEntryStore, 0, nbytes);
  }

  




  uint32_t capacity = Capacity();
  if (mEntryCount + mRemovedCount >= MaxLoad(capacity)) {
    
    int deltaLog2;
    if (mRemovedCount >= capacity >> 2) {
      deltaLog2 = 0;
    } else {
      deltaLog2 = 1;
    }

    




    if (!ChangeTable(deltaLog2) &&
        mEntryCount + mRemovedCount >= MaxLoadOnGrowthFailure(capacity)) {
      return nullptr;
    }
  }

  



  PLDHashNumber keyHash = ComputeKeyHash(aKey);
  PLDHashEntryHdr* entry = SearchTable<ForAdd>(aKey, keyHash);
  if (!ENTRY_IS_LIVE(entry)) {
    
    if (ENTRY_IS_REMOVED(entry)) {
      mRemovedCount--;
      keyHash |= COLLISION_FLAG;
    }
    if (mOps->initEntry) {
      mOps->initEntry(entry, aKey);
    }
    entry->mKeyHash = keyHash;
    mEntryCount++;
  }

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
#ifdef DEBUG
  AutoWriteOp op(mChecker);
#endif

  PLDHashEntryHdr* entry =
    mEntryStore ? SearchTable<ForSearchOrRemove>(aKey, ComputeKeyHash(aKey))
                : nullptr;
  if (entry) {
    
    PL_DHashTableRawRemove(this, entry);

    
    uint32_t capacity = Capacity();
    if (capacity > PL_DHASH_MIN_CAPACITY &&
        mEntryCount <= MinLoad(capacity)) {
      (void) ChangeTable(-1);
    }
  }
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
  
  
  
  MOZ_ASSERT(mChecker.IsWritable());

  MOZ_ASSERT(mEntryStore);

  NS_ASSERTION(ENTRY_IS_LIVE(aEntry), "ENTRY_IS_LIVE(aEntry)");

  
  PLDHashNumber keyHash = aEntry->mKeyHash;
  mOps->clearEntry(this, aEntry);
  if (keyHash & COLLISION_FLAG) {
    MARK_ENTRY_REMOVED(aEntry);
    mRemovedCount++;
  } else {
    MARK_ENTRY_FREE(aEntry);
  }
  mEntryCount--;
}

void
PL_DHashTableRawRemove(PLDHashTable* aTable, PLDHashEntryHdr* aEntry)
{
  aTable->RawRemove(aEntry);
}




void
PLDHashTable::ShrinkIfAppropriate()
{
  uint32_t capacity = Capacity();
  if (mRemovedCount >= capacity >> 2 ||
      (capacity > PL_DHASH_MIN_CAPACITY && mEntryCount <= MinLoad(capacity))) {

    uint32_t log2;
    BestCapacity(mEntryCount, &capacity, &log2);

    int32_t deltaLog2 = log2 - (PL_DHASH_BITS - mHashShift);
    MOZ_ASSERT(deltaLog2 <= 0);

    (void) ChangeTable(deltaLog2);
  }
}

MOZ_ALWAYS_INLINE uint32_t
PLDHashTable::Enumerate(PLDHashEnumerator aEtor, void* aArg)
{
#ifdef DEBUG
  
  
  
  
  bool wasIdleAndWritableAtStart = mChecker.IsIdle() && mChecker.IsWritable();
  AutoReadOp op(mChecker);
#endif

  if (!mEntryStore) {
    return 0;
  }

  char* entryAddr = mEntryStore;
  uint32_t capacity = Capacity();
  uint32_t tableSize = capacity * mEntrySize;
  char* entryLimit = mEntryStore + tableSize;
  uint32_t i = 0;
  bool didRemove = false;

  if (ChaosMode::isActive(ChaosMode::HashTableIteration)) {
    
    
    
    entryAddr += ChaosMode::randomUint32LessThan(capacity) * mEntrySize;
  }

  for (uint32_t e = 0; e < capacity; ++e) {
    PLDHashEntryHdr* entry = (PLDHashEntryHdr*)entryAddr;
    if (ENTRY_IS_LIVE(entry)) {
      PLDHashOperator op = aEtor(this, entry, i++, aArg);
      if (op & PL_DHASH_REMOVE) {
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

  
  
  MOZ_ASSERT_IF(didRemove, wasIdleAndWritableAtStart);

  
  
  
  if (didRemove) {
    ShrinkIfAppropriate();
  }

  return i;
}

uint32_t
PL_DHashTableEnumerate(PLDHashTable* aTable, PLDHashEnumerator aEtor,
                       void* aArg)
{
  return aTable->Enumerate(aEtor, aArg);
}

MOZ_ALWAYS_INLINE size_t
PLDHashTable::SizeOfExcludingThis(
    PLDHashSizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
    MallocSizeOf aMallocSizeOf, void* aArg ) const
{
#ifdef DEBUG
  AutoReadOp op(mChecker);
#endif

  if (!mEntryStore) {
    return 0;
  }

  size_t n = aMallocSizeOf(mEntryStore);
  if (aSizeOfEntryExcludingThis) {
    for (auto iter = Iter(); !iter.Done(); iter.Next()) {
      n += aSizeOfEntryExcludingThis(iter.Get(), aMallocSizeOf, aArg);
    }
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

PLDHashTable::Iterator::Iterator(Iterator&& aOther)
  : mTable(aOther.mTable)
  , mCurrent(aOther.mCurrent)
  , mLimit(aOther.mLimit)
{
  
  aOther.mTable = nullptr;
  aOther.mCurrent = nullptr;
  aOther.mLimit = nullptr;
}

PLDHashTable::Iterator::Iterator(const PLDHashTable* aTable)
  : mTable(aTable)
  , mCurrent(mTable->mEntryStore)
  , mLimit(mTable->mEntryStore + mTable->Capacity() * mTable->mEntrySize)
{
#ifdef DEBUG
  mTable->mChecker.StartReadOp();
#endif

  
  while (IsOnNonLiveEntry()) {
    mCurrent += mTable->mEntrySize;
  }
}

PLDHashTable::Iterator::~Iterator()
{
#ifdef DEBUG
  if (mTable) {
    mTable->mChecker.EndReadOp();
  }
#endif
}

bool
PLDHashTable::Iterator::Done() const
{
  return mCurrent == mLimit;
}

MOZ_ALWAYS_INLINE bool
PLDHashTable::Iterator::IsOnNonLiveEntry() const
{
  return !Done() && !ENTRY_IS_LIVE(reinterpret_cast<PLDHashEntryHdr*>(mCurrent));
}

PLDHashEntryHdr*
PLDHashTable::Iterator::Get() const
{
  MOZ_ASSERT(!Done());

  PLDHashEntryHdr* entry = reinterpret_cast<PLDHashEntryHdr*>(mCurrent);
  MOZ_ASSERT(ENTRY_IS_LIVE(entry));
  return entry;
}

void
PLDHashTable::Iterator::Next()
{
  MOZ_ASSERT(!Done());

  do {
    mCurrent += mTable->mEntrySize;
  } while (IsOnNonLiveEntry());
}

PLDHashTable::RemovingIterator::RemovingIterator(RemovingIterator&& aOther)
  : Iterator(mozilla::Move(aOther))
  , mHaveRemoved(aOther.mHaveRemoved)
{
  
  
  
}

PLDHashTable::RemovingIterator::RemovingIterator(PLDHashTable* aTable)
  : Iterator(aTable)
  , mHaveRemoved(false)
{
}

PLDHashTable::RemovingIterator::~RemovingIterator()
{
  if (mHaveRemoved) {
#ifdef DEBUG
    AutoIteratorRemovalOp op(mTable->mChecker);
#endif

    
    
    
    
    const_cast<PLDHashTable*>(mTable)->ShrinkIfAppropriate();
  }
}

void
PLDHashTable::RemovingIterator::Remove()
{
  
  const_cast<PLDHashTable*>(mTable)->RawRemove(Get());
  mHaveRemoved = true;
}

#ifdef DEBUG
MOZ_ALWAYS_INLINE void
PLDHashTable::MarkImmutable()
{
  mChecker.SetNonWritable();
}

void
PL_DHashMarkTableImmutable(PLDHashTable* aTable)
{
  aTable->MarkImmutable();
}
#endif

