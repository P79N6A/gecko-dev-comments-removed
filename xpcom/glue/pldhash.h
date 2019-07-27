





#ifndef pldhash_h___
#define pldhash_h___



#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h" 
#include "mozilla/fallible.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
#include "mozilla/Types.h"
#include "nscore.h"

#ifdef PL_DHASHMETER
#include <stdio.h>
#endif

#if defined(__GNUC__) && defined(__i386__)
#define PL_DHASH_FASTCALL __attribute__ ((regparm (3),stdcall))
#elif defined(XP_WIN)
#define PL_DHASH_FASTCALL __fastcall
#else
#define PL_DHASH_FASTCALL
#endif








#define PL_DHASH_MAX_CAPACITY           ((uint32_t)1 << 26)

#define PL_DHASH_MIN_CAPACITY           8





#define PL_DHASH_MAX_INITIAL_LENGTH     (PL_DHASH_MAX_CAPACITY / 2)


#define PL_DHASH_DEFAULT_INITIAL_LENGTH 4





#define PL_DHASH_BITS           32
#define PL_DHASH_GOLDEN_RATIO   0x9E3779B9U

typedef uint32_t PLDHashNumber;

class PLDHashTable;
struct PLDHashTableOps;



















struct PLDHashEntryHdr
{
private:
  friend class PLDHashTable;

  PLDHashNumber mKeyHash;
};





enum PLDHashOperator
{
  PL_DHASH_NEXT = 0,          
  PL_DHASH_STOP = 1,          
  PL_DHASH_REMOVE = 2         
};








































typedef PLDHashOperator (*PLDHashEnumerator)(PLDHashTable* aTable,
                                             PLDHashEntryHdr* aHdr,
                                             uint32_t aNumber, void* aArg);

typedef size_t (*PLDHashSizeOfEntryExcludingThisFun)(
  PLDHashEntryHdr* aHdr, mozilla::MallocSizeOf aMallocSizeOf, void* aArg);


















class PLDHashTable
{
private:
  const PLDHashTableOps* mOps;        
  int16_t             mHashShift;     
  uint32_t            mEntrySize;     
  uint32_t            mEntryCount;    
  uint32_t            mRemovedCount;  
  uint32_t            mGeneration;    
  char*               mEntryStore;    
#ifdef PL_DHASHMETER
  struct PLDHashStats
  {
    uint32_t        mSearches;      
    uint32_t        mSteps;         
    uint32_t        mHits;          
    uint32_t        mMisses;        
    uint32_t        mSearches;      
    uint32_t        mAddMisses;     
    uint32_t        mAddOverRemoved;
    uint32_t        mAddHits;       
    uint32_t        mAddFailures;   
    uint32_t        mRemoveHits;    
    uint32_t        mRemoveMisses;  
    uint32_t        mRemoveFrees;   
    uint32_t        mRemoveEnums;   
    uint32_t        mGrows;         
    uint32_t        mShrinks;       
    uint32_t        mCompresses;    
    uint32_t        mEnumShrinks;   
  } mStats;
#endif

#ifdef DEBUG
  
  
  
  
  
  mutable mozilla::Atomic<uint32_t> mRecursionLevel;
#endif

public:
  
  
  
  MOZ_CONSTEXPR PLDHashTable()
    : mOps(nullptr)
    , mHashShift(0)
    , mEntrySize(0)
    , mEntryCount(0)
    , mRemovedCount(0)
    , mGeneration(0)
    , mEntryStore(nullptr)
#ifdef PL_DHASHMETER
    , mStats()
#endif
#ifdef DEBUG
    , mRecursionLevel()
#endif
  {}

  PLDHashTable(PLDHashTable&& aOther) { *this = mozilla::Move(aOther); }

  PLDHashTable& operator=(PLDHashTable&& aOther)
  {
    using mozilla::Move;

    mOps = Move(aOther.mOps);
    mHashShift = Move(aOther.mHashShift);
    mEntrySize = Move(aOther.mEntrySize);
    mEntryCount = Move(aOther.mEntryCount);
    mRemovedCount = Move(aOther.mRemovedCount);
    mGeneration = Move(aOther.mGeneration);
    mEntryStore = Move(aOther.mEntryStore);

#ifdef PL_DHASHMETER
    mStats = Move(aOther.mStats);
#endif

#ifdef DEBUG
    
    mRecursionLevel = uint32_t(aOther.mRecursionLevel);
#endif

    return *this;
  }

  bool IsInitialized() const { return !!mOps; }

  
  const PLDHashTableOps* const Ops() { return mOps; }
  void SetOps(const PLDHashTableOps* aOps) { mOps = aOps; }

  




  uint32_t Capacity() const
  {
    return mEntryStore ? CapacityFromHashShift() : 0;
  }

  uint32_t EntrySize()  const { return mEntrySize; }
  uint32_t EntryCount() const { return mEntryCount; }
  uint32_t Generation() const { return mGeneration; }

  void Init(const PLDHashTableOps* aOps, uint32_t aEntrySize, uint32_t aLength);

  void Finish();

  PLDHashEntryHdr* Search(const void* aKey);
  PLDHashEntryHdr* Add(const void* aKey, const mozilla::fallible_t&);
  PLDHashEntryHdr* Add(const void* aKey);
  void Remove(const void* aKey);

  void RawRemove(PLDHashEntryHdr* aEntry);

  uint32_t Enumerate(PLDHashEnumerator aEtor, void* aArg);

  size_t SizeOfIncludingThis(
    PLDHashSizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
    mozilla::MallocSizeOf aMallocSizeOf, void* aArg = nullptr) const;

  size_t SizeOfExcludingThis(
    PLDHashSizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
    mozilla::MallocSizeOf aMallocSizeOf, void* aArg = nullptr) const;

#ifdef DEBUG
  void MarkImmutable();
#endif

  void MoveEntryStub(const PLDHashEntryHdr* aFrom, PLDHashEntryHdr* aTo);

  void ClearEntryStub(PLDHashEntryHdr* aEntry);

  void FreeStringKey(PLDHashEntryHdr* aEntry);

#ifdef PL_DHASHMETER
  void DumpMeter(PLDHashEnumerator aDump, FILE* aFp);
#endif

  




  class Iterator {
  public:
    explicit Iterator(const PLDHashTable* aTable);
    Iterator(const Iterator& aIterator);
    ~Iterator();
    bool HasMoreEntries() const;
    PLDHashEntryHdr* NextEntry();

  private:
    const PLDHashTable* mTable;       
    char* mEntryAddr;                 
    uint32_t mEntryOffset;            
  };

  Iterator Iterate() const { return Iterator(this); }

private:
  static bool EntryIsFree(PLDHashEntryHdr* aEntry);

  
  
  uint32_t CapacityFromHashShift() const
  {
    return ((uint32_t)1 << (PL_DHASH_BITS - mHashShift));
  }

  PLDHashNumber ComputeKeyHash(const void* aKey);

  enum SearchReason { ForSearchOrRemove, ForAdd };

  template <SearchReason Reason>
  PLDHashEntryHdr* PL_DHASH_FASTCALL
    SearchTable(const void* aKey, PLDHashNumber aKeyHash);

  PLDHashEntryHdr* PL_DHASH_FASTCALL FindFreeEntry(PLDHashNumber aKeyHash);

  bool ChangeTable(int aDeltaLog2);

  PLDHashTable(const PLDHashTable& aOther) = delete;
  PLDHashTable& operator=(const PLDHashTable& aOther) = delete;
};





typedef PLDHashNumber (*PLDHashHashKey)(PLDHashTable* aTable,
                                        const void* aKey);





typedef bool (*PLDHashMatchEntry)(PLDHashTable* aTable,
                                  const PLDHashEntryHdr* aEntry,
                                  const void* aKey);







typedef void (*PLDHashMoveEntry)(PLDHashTable* aTable,
                                 const PLDHashEntryHdr* aFrom,
                                 PLDHashEntryHdr* aTo);






typedef void (*PLDHashClearEntry)(PLDHashTable* aTable,
                                  PLDHashEntryHdr* aEntry);







typedef void (*PLDHashInitEntry)(PLDHashEntryHdr* aEntry, const void* aKey);























struct PLDHashTableOps
{
  
  PLDHashHashKey      hashKey;
  PLDHashMatchEntry   matchEntry;
  PLDHashMoveEntry    moveEntry;
  PLDHashClearEntry   clearEntry;

  
  PLDHashInitEntry    initEntry;
};





PLDHashNumber PL_DHashStringKey(PLDHashTable* aTable, const void* aKey);


struct PLDHashEntryStub : public PLDHashEntryHdr
{
  const void* key;
};

PLDHashNumber PL_DHashVoidPtrKeyStub(PLDHashTable* aTable, const void* aKey);

bool PL_DHashMatchEntryStub(PLDHashTable* aTable,
                            const PLDHashEntryHdr* aEntry,
                            const void* aKey);

bool PL_DHashMatchStringKey(PLDHashTable* aTable,
                            const PLDHashEntryHdr* aEntry,
                            const void* aKey);

void
PL_DHashMoveEntryStub(PLDHashTable* aTable,
                      const PLDHashEntryHdr* aFrom,
                      PLDHashEntryHdr* aTo);

void PL_DHashClearEntryStub(PLDHashTable* aTable, PLDHashEntryHdr* aEntry);

void PL_DHashFreeStringKey(PLDHashTable* aTable, PLDHashEntryHdr* aEntry);






const PLDHashTableOps* PL_DHashGetStubOps(void);





PLDHashTable* PL_NewDHashTable(
  const PLDHashTableOps* aOps, uint32_t aEntrySize,
  uint32_t aLength = PL_DHASH_DEFAULT_INITIAL_LENGTH);






void PL_DHashTableDestroy(PLDHashTable* aTable);











void PL_DHashTableInit(
  PLDHashTable* aTable, const PLDHashTableOps* aOps,
  uint32_t aEntrySize, uint32_t aLength = PL_DHASH_DEFAULT_INITIAL_LENGTH);






void PL_DHashTableFinish(PLDHashTable* aTable);









PLDHashEntryHdr* PL_DHASH_FASTCALL
PL_DHashTableSearch(PLDHashTable* aTable, const void* aKey);















PLDHashEntryHdr* PL_DHASH_FASTCALL
PL_DHashTableAdd(PLDHashTable* aTable, const void* aKey,
                 const mozilla::fallible_t&);





PLDHashEntryHdr* PL_DHASH_FASTCALL
PL_DHashTableAdd(PLDHashTable* aTable, const void* aKey);










void PL_DHASH_FASTCALL
PL_DHashTableRemove(PLDHashTable* aTable, const void* aKey);










void PL_DHashTableRawRemove(PLDHashTable* aTable, PLDHashEntryHdr* aEntry);

uint32_t
PL_DHashTableEnumerate(PLDHashTable* aTable, PLDHashEnumerator aEtor,
                       void* aArg);







size_t PL_DHashTableSizeOfExcludingThis(
  const PLDHashTable* aTable,
  PLDHashSizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
  mozilla::MallocSizeOf aMallocSizeOf, void* aArg = nullptr);




size_t PL_DHashTableSizeOfIncludingThis(
  const PLDHashTable* aTable,
  PLDHashSizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
  mozilla::MallocSizeOf aMallocSizeOf, void* aArg = nullptr);

#ifdef DEBUG














void PL_DHashMarkTableImmutable(PLDHashTable* aTable);
#endif

#ifdef PL_DHASHMETER
void PL_DHashTableDumpMeter(PLDHashTable* aTable,
                            PLDHashEnumerator aDump, FILE* aFp);
#endif

#endif 
