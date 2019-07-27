





#ifndef pldhash_h___
#define pldhash_h___



#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h" 
#include "mozilla/fallible.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
#include "mozilla/Types.h"
#include "nscore.h"

#if defined(__GNUC__) && defined(__i386__)
#define PL_DHASH_FASTCALL __attribute__ ((regparm (3),stdcall))
#elif defined(XP_WIN)
#define PL_DHASH_FASTCALL __fastcall
#else
#define PL_DHASH_FASTCALL
#endif

typedef uint32_t PLDHashNumber;

class PLDHashTable;
struct PLDHashTableOps;



















struct PLDHashEntryHdr
{
private:
  friend class PLDHashTable;

  PLDHashNumber mKeyHash;
};

typedef size_t (*PLDHashSizeOfEntryExcludingThisFun)(
  PLDHashEntryHdr* aHdr, mozilla::MallocSizeOf aMallocSizeOf, void* aArg);

#ifdef DEBUG



























class Checker
{
public:
  MOZ_CONSTEXPR Checker() : mState(kIdle), mIsWritable(1) {}

  Checker& operator=(Checker&& aOther) {
    
    mState = uint32_t(aOther.mState);
    mIsWritable = uint32_t(aOther.mIsWritable);

    aOther.mState = kIdle;

    return *this;
  }

  static bool IsIdle(uint32_t aState)  { return aState == kIdle; }
  static bool IsRead(uint32_t aState)  { return kRead1 <= aState &&
                                                aState <= kReadMax; }
  static bool IsRead1(uint32_t aState) { return aState == kRead1; }
  static bool IsWrite(uint32_t aState) { return aState == kWrite; }

  bool IsIdle() const { return mState == kIdle; }

  bool IsWritable() const { return !!mIsWritable; }

  void SetNonWritable() { mIsWritable = 0; }

  
  
  
  
  
  
  
  
  
  
  
  

  void StartReadOp()
  {
    uint32_t oldState = mState++;     
    MOZ_ASSERT(IsIdle(oldState) || IsRead(oldState));
    MOZ_ASSERT(oldState < kReadMax);  
  }

  void EndReadOp()
  {
    uint32_t oldState = mState--;     
    MOZ_ASSERT(IsRead(oldState));
  }

  void StartWriteOp()
  {
    MOZ_ASSERT(IsWritable());
    uint32_t oldState = mState.exchange(kWrite);
    MOZ_ASSERT(IsIdle(oldState));
  }

  void EndWriteOp()
  {
    
    
    
    MOZ_ASSERT(IsWritable());
    uint32_t oldState = mState.exchange(kIdle);
    MOZ_ASSERT(IsWrite(oldState));
  }

  void StartIteratorRemovalOp()
  {
    
    
    MOZ_ASSERT(IsWritable());
    uint32_t oldState = mState.exchange(kWrite);
    MOZ_ASSERT(IsRead1(oldState));
  }

  void EndIteratorRemovalOp()
  {
    
    
    
    MOZ_ASSERT(IsWritable());
    uint32_t oldState = mState.exchange(kRead1);
    MOZ_ASSERT(IsWrite(oldState));
  }

  void StartDestructorOp()
  {
    
    
    uint32_t oldState = mState.exchange(kWrite);
    MOZ_ASSERT(IsIdle(oldState));
  }

  void EndDestructorOp()
  {
    uint32_t oldState = mState.exchange(kIdle);
    MOZ_ASSERT(IsWrite(oldState));
  }

private:
  
  
  
  
  
  static const uint32_t kIdle    = 0;
  static const uint32_t kRead1   = 1;
  static const uint32_t kReadMax = 9999;
  static const uint32_t kWrite   = 10000;

  mutable mozilla::Atomic<uint32_t> mState;
  mutable mozilla::Atomic<uint32_t> mIsWritable;
};
#endif
















class PLDHashTable
{
private:
  const PLDHashTableOps* const mOps;  
  int16_t             mHashShift;     
  const uint32_t      mEntrySize;     
  uint32_t            mEntryCount;    
  uint32_t            mRemovedCount;  
  uint32_t            mGeneration;    
  char*               mEntryStore;    

#ifdef DEBUG
  mutable Checker mChecker;
#endif

public:
  
  
  
  
  
  
  static const uint32_t kMaxCapacity = ((uint32_t)1 << 26);

  static const uint32_t kMinCapacity = 8;

  
  
  static const uint32_t kMaxInitialLength = kMaxCapacity / 2;

  
  static const uint32_t kDefaultInitialLength = 4;

  
  
  
  
  
  
  
  
  PLDHashTable(const PLDHashTableOps* aOps, uint32_t aEntrySize,
               uint32_t aLength = kDefaultInitialLength);

  PLDHashTable(PLDHashTable&& aOther)
      
      
    : mOps(aOther.mOps)
    , mEntrySize(aOther.mEntrySize)
      
      
    , mEntryStore(nullptr)
#ifdef DEBUG
    , mChecker()
#endif
  {
    *this = mozilla::Move(aOther);
  }

  PLDHashTable& operator=(PLDHashTable&& aOther);

  ~PLDHashTable();

  
  const PLDHashTableOps* const Ops() { return mOps; }

  




  uint32_t Capacity() const
  {
    return mEntryStore ? CapacityFromHashShift() : 0;
  }

  uint32_t EntrySize()  const { return mEntrySize; }
  uint32_t EntryCount() const { return mEntryCount; }
  uint32_t Generation() const { return mGeneration; }

  PLDHashEntryHdr* Search(const void* aKey);
  PLDHashEntryHdr* Add(const void* aKey, const mozilla::fallible_t&);
  PLDHashEntryHdr* Add(const void* aKey);
  void Remove(const void* aKey);

  void RawRemove(PLDHashEntryHdr* aEntry);

  
  
  void Clear();

  
  
  
  
  
  
  
  
  void ClearAndPrepareForLength(uint32_t aLength);

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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  class Iterator
  {
  public:
    explicit Iterator(PLDHashTable* aTable);
    Iterator(Iterator&& aOther);
    ~Iterator();

    bool Done() const;                
    PLDHashEntryHdr* Get() const;     
    void Next();                      

    
    
    void Remove();

  protected:
    PLDHashTable* mTable;             

  private:
    char* mStart;                     
    char* mLimit;                     
    char* mCurrent;                   
    uint32_t mNexts;                  
    uint32_t mNextsLimit;             

    bool mHaveRemoved;                

    bool IsOnNonLiveEntry() const;
    void MoveToNextEntry();

    Iterator() = delete;
    Iterator(const Iterator&) = delete;
    Iterator& operator=(const Iterator&) = delete;
    Iterator& operator=(const Iterator&&) = delete;
  };

  Iterator Iter() { return Iterator(this); }

  
  
  Iterator ConstIter() const
  {
    return Iterator(const_cast<PLDHashTable*>(this));
  }

private:
  
  
  static const uint32_t kHashBits = 32;
  static const uint32_t kGoldenRatio = 0x9E3779B9U;

  static uint32_t HashShift(uint32_t aEntrySize, uint32_t aLength);

  static bool EntryIsFree(PLDHashEntryHdr* aEntry);

  
  
  uint32_t CapacityFromHashShift() const
  {
    return ((uint32_t)1 << (kHashBits - mHashShift));
  }

  PLDHashNumber ComputeKeyHash(const void* aKey);

  enum SearchReason { ForSearchOrRemove, ForAdd };

  template <SearchReason Reason>
  PLDHashEntryHdr* PL_DHASH_FASTCALL
    SearchTable(const void* aKey, PLDHashNumber aKeyHash);

  PLDHashEntryHdr* PL_DHASH_FASTCALL FindFreeEntry(PLDHashNumber aKeyHash);

  bool ChangeTable(int aDeltaLog2);

  void ShrinkIfAppropriate();

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






const PLDHashTableOps* PL_DHashGetStubOps(void);









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

#endif 
