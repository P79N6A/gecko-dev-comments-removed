





#ifndef pldhash_h___
#define pldhash_h___



#include "mozilla/fallible.h"
#include "mozilla/MemoryReporting.h"
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


typedef uint32_t                PLDHashNumber;
typedef struct PLDHashEntryHdr  PLDHashEntryHdr;
typedef struct PLDHashEntryStub PLDHashEntryStub;
typedef struct PLDHashTable     PLDHashTable;
typedef struct PLDHashTableOps  PLDHashTableOps;

























struct PLDHashEntryHdr
{
  PLDHashNumber keyHash;  
};

MOZ_ALWAYS_INLINE bool
PL_DHASH_ENTRY_IS_FREE(PLDHashEntryHdr* aEntry)
{
  return aEntry->keyHash == 0;
}

MOZ_ALWAYS_INLINE bool
PL_DHASH_ENTRY_IS_BUSY(PLDHashEntryHdr* aEntry)
{
  return !PL_DHASH_ENTRY_IS_FREE(aEntry);
}







typedef enum PLDHashOperator
{
  PL_DHASH_LOOKUP = 0,        
  PL_DHASH_ADD = 1,           
  PL_DHASH_REMOVE = 2,        
  PL_DHASH_NEXT = 0,          
  PL_DHASH_STOP = 1           
} PLDHashOperator;








































typedef PLDHashOperator (*PLDHashEnumerator)(PLDHashTable* aTable,
                                             PLDHashEntryHdr* aHdr,
                                             uint32_t aNumber, void* aArg);

typedef size_t (*PLDHashSizeOfEntryExcludingThisFun)(
  PLDHashEntryHdr* aHdr, mozilla::MallocSizeOf aMallocSizeOf, void* aArg);















struct PLDHashTable
{
  



  const PLDHashTableOps* ops;

  void*               data;           

private:
  int16_t             mHashShift;     
  





  mutable uint16_t    mRecursionLevel;
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
    uint32_t        mLookups;       
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

public:
  




  uint32_t Capacity() const
  {
    return ((uint32_t)1 << (PL_DHASH_BITS - mHashShift));
  }

  uint32_t EntrySize()  const { return mEntrySize; }
  uint32_t EntryCount() const { return mEntryCount; }
  uint32_t Generation() const { return mGeneration; }

  bool Init(const PLDHashTableOps* aOps, void* aData, uint32_t aEntrySize,
            const mozilla::fallible_t&, uint32_t aLength);

  void Finish();

  PLDHashEntryHdr* Operate(const void* aKey, PLDHashOperator aOp);

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
  PLDHashEntryHdr* PL_DHASH_FASTCALL
    SearchTable(const void* aKey, PLDHashNumber aKeyHash, PLDHashOperator aOp);

  PLDHashEntryHdr* PL_DHASH_FASTCALL FindFreeEntry(PLDHashNumber aKeyHash);

  bool ChangeTable(int aDeltaLog2);
};






typedef void* (*PLDHashAllocTable)(PLDHashTable* aTable, uint32_t aNBytes);

typedef void (*PLDHashFreeTable)(PLDHashTable* aTable, void* aPtr);





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






typedef void (*PLDHashFinalize)(PLDHashTable* aTable);








typedef bool (*PLDHashInitEntry)(PLDHashTable* aTable, PLDHashEntryHdr* aEntry,
                                 const void* aKey);




























struct PLDHashTableOps
{
  
  PLDHashAllocTable   allocTable;
  PLDHashFreeTable    freeTable;
  PLDHashHashKey      hashKey;
  PLDHashMatchEntry   matchEntry;
  PLDHashMoveEntry    moveEntry;
  PLDHashClearEntry   clearEntry;
  PLDHashFinalize     finalize;

  
  PLDHashInitEntry    initEntry;
};




void* PL_DHashAllocTable(PLDHashTable* aTable, uint32_t aNBytes);

void PL_DHashFreeTable(PLDHashTable* aTable, void* aPtr);

PLDHashNumber PL_DHashStringKey(PLDHashTable* aTable, const void* aKey);


struct PLDHashEntryStub
{
  PLDHashEntryHdr hdr;
  const void*     key;
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

void PL_DHashFinalizeStub(PLDHashTable* aTable);






const PLDHashTableOps* PL_DHashGetStubOps(void);







PLDHashTable* PL_NewDHashTable(
  const PLDHashTableOps* aOps, void* aData, uint32_t aEntrySize,
  uint32_t aLength = PL_DHASH_DEFAULT_INITIAL_LENGTH);





void PL_DHashTableDestroy(PLDHashTable* aTable);










void PL_DHashTableInit(
  PLDHashTable* aTable, const PLDHashTableOps* aOps, void* aData,
  uint32_t aEntrySize, uint32_t aLength = PL_DHASH_DEFAULT_INITIAL_LENGTH);





MOZ_WARN_UNUSED_RESULT bool PL_DHashTableInit(
  PLDHashTable* aTable, const PLDHashTableOps* aOps, void* aData,
  uint32_t aEntrySize, const mozilla::fallible_t&,
  uint32_t aLength = PL_DHASH_DEFAULT_INITIAL_LENGTH);







void PL_DHashTableFinish(PLDHashTable* aTable);































PLDHashEntryHdr* PL_DHASH_FASTCALL
PL_DHashTableOperate(PLDHashTable* aTable, const void* aKey,
                     PLDHashOperator aOp);










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
