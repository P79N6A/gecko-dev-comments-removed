





#ifndef pldhash_h___
#define pldhash_h___



#include "mozilla/fallible.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Types.h"
#include "nscore.h"

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

MOZ_ALWAYS_INLINE bool
PL_DHASH_ENTRY_IS_LIVE(PLDHashEntryHdr* aEntry)
{
  return aEntry->keyHash >= 2;
}









































































struct PLDHashTable
{
  const PLDHashTableOps* ops;         
  void*               data;           
  int16_t             hashShift;      
  





  uint16_t            recursionLevel; 
  uint32_t            entrySize;      
  uint32_t            entryCount;     
  uint32_t            removedCount;   
  uint32_t            generation;     
  char*               entryStore;     
#ifdef PL_DHASHMETER
  struct PLDHashStats
  {
    uint32_t        searches;       
    uint32_t        steps;          
    uint32_t        hits;           
    uint32_t        misses;         
    uint32_t        lookups;        
    uint32_t        addMisses;      
    uint32_t        addOverRemoved; 
    uint32_t        addHits;        
    uint32_t        addFailures;    
    uint32_t        removeHits;     
    uint32_t        removeMisses;   
    uint32_t        removeFrees;    
    uint32_t        removeEnums;    
    uint32_t        grows;          
    uint32_t        shrinks;        
    uint32_t        compresses;     
    uint32_t        enumShrinks;    
  } stats;
#endif
};






#define PL_DHASH_TABLE_CAPACITY(table) \
    ((uint32_t)1 << (PL_DHASH_BITS - (table)->hashShift))






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




NS_COM_GLUE void* PL_DHashAllocTable(PLDHashTable* aTable, uint32_t aNBytes);

NS_COM_GLUE void PL_DHashFreeTable(PLDHashTable* aTable, void* aPtr);

NS_COM_GLUE PLDHashNumber PL_DHashStringKey(PLDHashTable* aTable,
                                            const void* aKey);


struct PLDHashEntryStub
{
  PLDHashEntryHdr hdr;
  const void*     key;
};

NS_COM_GLUE PLDHashNumber PL_DHashVoidPtrKeyStub(PLDHashTable* aTable,
                                                 const void* aKey);

NS_COM_GLUE bool PL_DHashMatchEntryStub(PLDHashTable* aTable,
                                        const PLDHashEntryHdr* aEntry,
                                        const void* aKey);

NS_COM_GLUE bool PL_DHashMatchStringKey(PLDHashTable* aTable,
                                        const PLDHashEntryHdr* aEntry,
                                        const void* aKey);

NS_COM_GLUE void
PL_DHashMoveEntryStub(PLDHashTable* aTable,
                      const PLDHashEntryHdr* aFrom,
                      PLDHashEntryHdr* aTo);

NS_COM_GLUE void PL_DHashClearEntryStub(PLDHashTable* aTable,
                                        PLDHashEntryHdr* aEntry);

NS_COM_GLUE void PL_DHashFreeStringKey(PLDHashTable* aTable,
                                       PLDHashEntryHdr* aEntry);

NS_COM_GLUE void PL_DHashFinalizeStub(PLDHashTable* aTable);






NS_COM_GLUE const PLDHashTableOps* PL_DHashGetStubOps(void);







NS_COM_GLUE PLDHashTable* PL_NewDHashTable(
  const PLDHashTableOps* aOps, void* aData, uint32_t aEntrySize,
  uint32_t aLength = PL_DHASH_DEFAULT_INITIAL_LENGTH);





NS_COM_GLUE void PL_DHashTableDestroy(PLDHashTable* aTable);










NS_COM_GLUE void PL_DHashTableInit(
  PLDHashTable* aTable, const PLDHashTableOps* aOps, void* aData,
  uint32_t aEntrySize, uint32_t aLength = PL_DHASH_DEFAULT_INITIAL_LENGTH);





MOZ_WARN_UNUSED_RESULT NS_COM_GLUE bool PL_DHashTableInit(
  PLDHashTable* aTable, const PLDHashTableOps* aOps, void* aData,
  uint32_t aEntrySize, const mozilla::fallible_t&,
  uint32_t aLength = PL_DHASH_DEFAULT_INITIAL_LENGTH);







NS_COM_GLUE void PL_DHashTableFinish(PLDHashTable* aTable);







typedef enum PLDHashOperator
{
  PL_DHASH_LOOKUP = 0,        
  PL_DHASH_ADD = 1,           
  PL_DHASH_REMOVE = 2,        
  PL_DHASH_NEXT = 0,          
  PL_DHASH_STOP = 1           
} PLDHashOperator;































NS_COM_GLUE PLDHashEntryHdr* PL_DHASH_FASTCALL
PL_DHashTableOperate(PLDHashTable* aTable, const void* aKey,
                     PLDHashOperator aOp);










NS_COM_GLUE void PL_DHashTableRawRemove(PLDHashTable* aTable,
                                        PLDHashEntryHdr* aEntry);








































typedef PLDHashOperator (*PLDHashEnumerator)(PLDHashTable* aTable,
                                             PLDHashEntryHdr* aHdr,
                                             uint32_t aNumber, void* aArg);

NS_COM_GLUE uint32_t
PL_DHashTableEnumerate(PLDHashTable* aTable, PLDHashEnumerator aEtor,
                       void* aArg);

typedef size_t (*PLDHashSizeOfEntryExcludingThisFun)(
  PLDHashEntryHdr* aHdr, mozilla::MallocSizeOf aMallocSizeOf, void* aArg);







NS_COM_GLUE size_t PL_DHashTableSizeOfExcludingThis(
  const PLDHashTable* aTable,
  PLDHashSizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
  mozilla::MallocSizeOf aMallocSizeOf, void* aArg = nullptr);




NS_COM_GLUE size_t PL_DHashTableSizeOfIncludingThis(
  const PLDHashTable* aTable,
  PLDHashSizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
  mozilla::MallocSizeOf aMallocSizeOf, void* aArg = nullptr);

#ifdef DEBUG














NS_COM_GLUE void PL_DHashMarkTableImmutable(PLDHashTable* aTable);
#endif

#ifdef PL_DHASHMETER
#include <stdio.h>

NS_COM_GLUE void PL_DHashTableDumpMeter(PLDHashTable* aTable,
                                        PLDHashEnumerator aDump, FILE* aFp);
#endif

#endif 
