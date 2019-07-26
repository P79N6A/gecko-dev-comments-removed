




#ifndef pldhash_h___
#define pldhash_h___





#include "mozilla/Types.h"
#include "nscore.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && defined(__i386__) && !defined(XP_OS2)
#define PL_DHASH_FASTCALL __attribute__ ((regparm (3),stdcall))
#elif defined(XP_WIN)
#define PL_DHASH_FASTCALL __fastcall
#else
#define PL_DHASH_FASTCALL
#endif

#ifdef DEBUG_XXXbrendan
#define PL_DHASHMETER 1
#endif


#undef PL_DHASH_SIZE_LIMIT
#define PL_DHASH_SIZE_LIMIT     ((uint32_t)1 << 24)


#ifndef PL_DHASH_MIN_SIZE
#define PL_DHASH_MIN_SIZE 16
#elif (PL_DHASH_MIN_SIZE & (PL_DHASH_MIN_SIZE - 1)) != 0
#error "PL_DHASH_MIN_SIZE must be a power of two!"
#endif





#define PL_DHASH_BITS           32
#define PL_DHASH_GOLDEN_RATIO   0x9E3779B9U


typedef uint32_t                PLDHashNumber;
typedef struct PLDHashEntryHdr  PLDHashEntryHdr;
typedef struct PLDHashEntryStub PLDHashEntryStub;
typedef struct PLDHashTable     PLDHashTable;
typedef struct PLDHashTableOps  PLDHashTableOps;






























struct PLDHashEntryHdr {
    PLDHashNumber       keyHash;        
};

#define PL_DHASH_ENTRY_IS_FREE(entry)   ((entry)->keyHash == 0)
#define PL_DHASH_ENTRY_IS_BUSY(entry)   (!PL_DHASH_ENTRY_IS_FREE(entry))
#define PL_DHASH_ENTRY_IS_LIVE(entry)   ((entry)->keyHash >= 2)









































































struct PLDHashTable {
    const PLDHashTableOps *ops;         
    void                *data;          
    int16_t             hashShift;      
    uint8_t             maxAlphaFrac;   
    uint8_t             minAlphaFrac;   
    uint32_t            entrySize;      
    uint32_t            entryCount;     
    uint32_t            removedCount;   
    uint32_t            generation;     
    char                *entryStore;    
#ifdef PL_DHASHMETER
    struct PLDHashStats {
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






#define PL_DHASH_TABLE_SIZE(table) \
    ((uint32_t)1 << (PL_DHASH_BITS - (table)->hashShift))






typedef void *
(* PLDHashAllocTable)(PLDHashTable *table, uint32_t nbytes);

typedef void
(* PLDHashFreeTable) (PLDHashTable *table, void *ptr);





typedef PLDHashNumber
(* PLDHashHashKey)   (PLDHashTable *table, const void *key);





typedef bool
(* PLDHashMatchEntry)(PLDHashTable *table, const PLDHashEntryHdr *entry,
                      const void *key);







typedef void
(* PLDHashMoveEntry)(PLDHashTable *table, const PLDHashEntryHdr *from,
                     PLDHashEntryHdr *to);






typedef void
(* PLDHashClearEntry)(PLDHashTable *table, PLDHashEntryHdr *entry);






typedef void
(* PLDHashFinalize)  (PLDHashTable *table);








typedef bool
(* PLDHashInitEntry)(PLDHashTable *table, PLDHashEntryHdr *entry,
                     const void *key);




























struct PLDHashTableOps {
    
    PLDHashAllocTable   allocTable;
    PLDHashFreeTable    freeTable;
    PLDHashHashKey      hashKey;
    PLDHashMatchEntry   matchEntry;
    PLDHashMoveEntry    moveEntry;
    PLDHashClearEntry   clearEntry;
    PLDHashFinalize     finalize;

    
    PLDHashInitEntry    initEntry;
};




NS_COM_GLUE void *
PL_DHashAllocTable(PLDHashTable *table, uint32_t nbytes);

NS_COM_GLUE void
PL_DHashFreeTable(PLDHashTable *table, void *ptr);

NS_COM_GLUE PLDHashNumber
PL_DHashStringKey(PLDHashTable *table, const void *key);


struct PLDHashEntryStub {
    PLDHashEntryHdr hdr;
    const void      *key;
};

NS_COM_GLUE PLDHashNumber
PL_DHashVoidPtrKeyStub(PLDHashTable *table, const void *key);

NS_COM_GLUE bool
PL_DHashMatchEntryStub(PLDHashTable *table,
                       const PLDHashEntryHdr *entry,
                       const void *key);

NS_COM_GLUE bool
PL_DHashMatchStringKey(PLDHashTable *table,
                       const PLDHashEntryHdr *entry,
                       const void *key);

NS_COM_GLUE void
PL_DHashMoveEntryStub(PLDHashTable *table,
                      const PLDHashEntryHdr *from,
                      PLDHashEntryHdr *to);

NS_COM_GLUE void
PL_DHashClearEntryStub(PLDHashTable *table, PLDHashEntryHdr *entry);

NS_COM_GLUE void
PL_DHashFreeStringKey(PLDHashTable *table, PLDHashEntryHdr *entry);

NS_COM_GLUE void
PL_DHashFinalizeStub(PLDHashTable *table);






NS_COM_GLUE const PLDHashTableOps *
PL_DHashGetStubOps(void);







NS_COM_GLUE PLDHashTable *
PL_NewDHashTable(const PLDHashTableOps *ops, void *data, uint32_t entrySize,
                 uint32_t capacity);





NS_COM_GLUE void
PL_DHashTableDestroy(PLDHashTable *table);







NS_COM_GLUE bool
PL_DHashTableInit(PLDHashTable *table, const PLDHashTableOps *ops, void *data,
                  uint32_t entrySize, uint32_t capacity);









NS_COM_GLUE void
PL_DHashTableSetAlphaBounds(PLDHashTable *table,
                            float maxAlpha,
                            float minAlpha);






#define PL_DHASH_MIN_ALPHA(table, k)                                          \
    ((float)((table)->entrySize / sizeof(void *) - 1)                         \
     / ((table)->entrySize / sizeof(void *) + (k)))











#define PL_DHASH_DEFAULT_MAX_ALPHA 0.75
#define PL_DHASH_DEFAULT_MIN_ALPHA 0.25

#define PL_DHASH_CAP(entryCount, maxAlpha)                                    \
    ((uint32_t)((double)(entryCount) / (maxAlpha)))

#define PL_DHASH_CAPACITY(entryCount, maxAlpha)                               \
    (PL_DHASH_CAP(entryCount, maxAlpha) +                                     \
     (((PL_DHASH_CAP(entryCount, maxAlpha) * (uint8_t)(0x100 * (maxAlpha)))     \
       >> 8) < (entryCount)))

#define PL_DHASH_DEFAULT_CAPACITY(entryCount)                                 \
    PL_DHASH_CAPACITY(entryCount, PL_DHASH_DEFAULT_MAX_ALPHA)







NS_COM_GLUE void
PL_DHashTableFinish(PLDHashTable *table);







typedef enum PLDHashOperator {
    PL_DHASH_LOOKUP = 0,        
    PL_DHASH_ADD = 1,           
    PL_DHASH_REMOVE = 2,        
    PL_DHASH_NEXT = 0,          
    PL_DHASH_STOP = 1           
} PLDHashOperator;































NS_COM_GLUE PLDHashEntryHdr * PL_DHASH_FASTCALL
PL_DHashTableOperate(PLDHashTable *table, const void *key, PLDHashOperator op);










NS_COM_GLUE void
PL_DHashTableRawRemove(PLDHashTable *table, PLDHashEntryHdr *entry);








































typedef PLDHashOperator
(* PLDHashEnumerator)(PLDHashTable *table, PLDHashEntryHdr *hdr, uint32_t number,
                      void *arg);

NS_COM_GLUE uint32_t
PL_DHashTableEnumerate(PLDHashTable *table, PLDHashEnumerator etor, void *arg);

typedef size_t
(* PLDHashSizeOfEntryExcludingThisFun)(PLDHashEntryHdr *hdr,
                                       nsMallocSizeOfFun mallocSizeOf,
                                       void *arg);







NS_COM_GLUE size_t
PL_DHashTableSizeOfExcludingThis(const PLDHashTable *table,
                                 PLDHashSizeOfEntryExcludingThisFun sizeOfEntryExcludingThis,
                                 nsMallocSizeOfFun mallocSizeOf,
                                 void *arg = NULL);




NS_COM_GLUE size_t
PL_DHashTableSizeOfIncludingThis(const PLDHashTable *table,
                                 PLDHashSizeOfEntryExcludingThisFun sizeOfEntryExcludingThis,
                                 nsMallocSizeOfFun mallocSizeOf,
                                 void *arg = NULL);

#ifdef DEBUG














NS_COM_GLUE void
PL_DHashMarkTableImmutable(PLDHashTable *table);
#endif

#ifdef PL_DHASHMETER
#include <stdio.h>

NS_COM_GLUE void
PL_DHashTableDumpMeter(PLDHashTable *table, PLDHashEnumerator dump, FILE *fp);
#endif

#ifdef __cplusplus
}
#endif

#endif
