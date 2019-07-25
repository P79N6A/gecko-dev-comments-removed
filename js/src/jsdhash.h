





































#ifndef jsdhash_h___
#define jsdhash_h___






#include "jstypes.h"
#include "jsutil.h"

JS_BEGIN_EXTERN_C

#if defined(__GNUC__) && defined(__i386__) && (__GNUC__ >= 3) && !defined(XP_OS2)
#define JS_DHASH_FASTCALL __attribute__ ((regparm (3),stdcall))
#elif defined(XP_WIN)
#define JS_DHASH_FASTCALL __fastcall
#else
#define JS_DHASH_FASTCALL
#endif

#ifdef DEBUG_XXXbrendan
#define JS_DHASHMETER 1
#endif


#undef JS_DHASH_SIZE_LIMIT
#define JS_DHASH_SIZE_LIMIT     JS_BIT(24)


#ifndef JS_DHASH_MIN_SIZE
#define JS_DHASH_MIN_SIZE 16
#elif (JS_DHASH_MIN_SIZE & (JS_DHASH_MIN_SIZE - 1)) != 0
#error "JS_DHASH_MIN_SIZE must be a power of two!"
#endif





#define JS_DHASH_BITS           32
#define JS_DHASH_GOLDEN_RATIO   0x9E3779B9U


typedef uint32_t                JSDHashNumber;
typedef struct JSDHashEntryHdr  JSDHashEntryHdr;
typedef struct JSDHashEntryStub JSDHashEntryStub;
typedef struct JSDHashTable     JSDHashTable;
typedef struct JSDHashTableOps  JSDHashTableOps;






























struct JSDHashEntryHdr {
    JSDHashNumber       keyHash;        
};

#define JS_DHASH_ENTRY_IS_FREE(entry)   ((entry)->keyHash == 0)
#define JS_DHASH_ENTRY_IS_BUSY(entry)   (!JS_DHASH_ENTRY_IS_FREE(entry))
#define JS_DHASH_ENTRY_IS_LIVE(entry)   ((entry)->keyHash >= 2)









































































struct JSDHashTable {
    const JSDHashTableOps *ops;         
    void                *data;          
    int16_t             hashShift;      
    uint8_t             maxAlphaFrac;   
    uint8_t             minAlphaFrac;   
    uint32_t            entrySize;      
    uint32_t            entryCount;     
    uint32_t            removedCount;   
    uint32_t            generation;     
    char                *entryStore;    
#ifdef JS_DHASHMETER
    struct JSDHashStats {
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






#define JS_DHASH_TABLE_SIZE(table)  JS_BIT(JS_DHASH_BITS - (table)->hashShift)






typedef void *
(* JSDHashAllocTable)(JSDHashTable *table, uint32_t nbytes);

typedef void
(* JSDHashFreeTable) (JSDHashTable *table, void *ptr);





typedef JSDHashNumber
(* JSDHashHashKey)   (JSDHashTable *table, const void *key);





typedef JSBool
(* JSDHashMatchEntry)(JSDHashTable *table, const JSDHashEntryHdr *entry,
                      const void *key);







typedef void
(* JSDHashMoveEntry)(JSDHashTable *table, const JSDHashEntryHdr *from,
                     JSDHashEntryHdr *to);






typedef void
(* JSDHashClearEntry)(JSDHashTable *table, JSDHashEntryHdr *entry);






typedef void
(* JSDHashFinalize)  (JSDHashTable *table);








typedef JSBool
(* JSDHashInitEntry)(JSDHashTable *table, JSDHashEntryHdr *entry,
                     const void *key);




























struct JSDHashTableOps {
    
    JSDHashAllocTable   allocTable;
    JSDHashFreeTable    freeTable;
    JSDHashHashKey      hashKey;
    JSDHashMatchEntry   matchEntry;
    JSDHashMoveEntry    moveEntry;
    JSDHashClearEntry   clearEntry;
    JSDHashFinalize     finalize;

    
    JSDHashInitEntry    initEntry;
};




extern JS_PUBLIC_API(void *)
JS_DHashAllocTable(JSDHashTable *table, uint32_t nbytes);

extern JS_PUBLIC_API(void)
JS_DHashFreeTable(JSDHashTable *table, void *ptr);

extern JS_PUBLIC_API(JSDHashNumber)
JS_DHashStringKey(JSDHashTable *table, const void *key);


struct JSDHashEntryStub {
    JSDHashEntryHdr hdr;
    const void      *key;
};

extern JS_PUBLIC_API(JSDHashNumber)
JS_DHashVoidPtrKeyStub(JSDHashTable *table, const void *key);

extern JS_PUBLIC_API(JSBool)
JS_DHashMatchEntryStub(JSDHashTable *table,
                       const JSDHashEntryHdr *entry,
                       const void *key);

extern JS_PUBLIC_API(JSBool)
JS_DHashMatchStringKey(JSDHashTable *table,
                       const JSDHashEntryHdr *entry,
                       const void *key);

extern JS_PUBLIC_API(void)
JS_DHashMoveEntryStub(JSDHashTable *table,
                      const JSDHashEntryHdr *from,
                      JSDHashEntryHdr *to);

extern JS_PUBLIC_API(void)
JS_DHashClearEntryStub(JSDHashTable *table, JSDHashEntryHdr *entry);

extern JS_PUBLIC_API(void)
JS_DHashFreeStringKey(JSDHashTable *table, JSDHashEntryHdr *entry);

extern JS_PUBLIC_API(void)
JS_DHashFinalizeStub(JSDHashTable *table);






extern JS_PUBLIC_API(const JSDHashTableOps *)
JS_DHashGetStubOps(void);







extern JS_PUBLIC_API(JSDHashTable *)
JS_NewDHashTable(const JSDHashTableOps *ops, void *data, uint32_t entrySize,
                 uint32_t capacity);





extern JS_PUBLIC_API(void)
JS_DHashTableDestroy(JSDHashTable *table);







extern JS_PUBLIC_API(JSBool)
JS_DHashTableInit(JSDHashTable *table, const JSDHashTableOps *ops, void *data,
                  uint32_t entrySize, uint32_t capacity);









extern JS_PUBLIC_API(void)
JS_DHashTableSetAlphaBounds(JSDHashTable *table,
                            float maxAlpha,
                            float minAlpha);






#define JS_DHASH_MIN_ALPHA(table, k)                                          \
    ((float)((table)->entrySize / sizeof(void *) - 1)                         \
     / ((table)->entrySize / sizeof(void *) + (k)))











#define JS_DHASH_DEFAULT_MAX_ALPHA 0.75
#define JS_DHASH_DEFAULT_MIN_ALPHA 0.25

#define JS_DHASH_CAP(entryCount, maxAlpha)                                    \
    ((uint32_t)((double)(entryCount) / (maxAlpha)))

#define JS_DHASH_CAPACITY(entryCount, maxAlpha)                               \
    (JS_DHASH_CAP(entryCount, maxAlpha) +                                     \
     (((JS_DHASH_CAP(entryCount, maxAlpha) * (uint8_t)(0x100 * (maxAlpha)))   \
       >> 8) < (entryCount)))

#define JS_DHASH_DEFAULT_CAPACITY(entryCount)                                 \
    JS_DHASH_CAPACITY(entryCount, JS_DHASH_DEFAULT_MAX_ALPHA)







extern JS_PUBLIC_API(void)
JS_DHashTableFinish(JSDHashTable *table);







typedef enum JSDHashOperator {
    JS_DHASH_LOOKUP = 0,        
    JS_DHASH_ADD = 1,           
    JS_DHASH_REMOVE = 2,        
    JS_DHASH_NEXT = 0,          
    JS_DHASH_STOP = 1           
} JSDHashOperator;































extern JS_PUBLIC_API(JSDHashEntryHdr *) JS_DHASH_FASTCALL
JS_DHashTableOperate(JSDHashTable *table, const void *key, JSDHashOperator op);










extern JS_PUBLIC_API(void)
JS_DHashTableRawRemove(JSDHashTable *table, JSDHashEntryHdr *entry);








































typedef JSDHashOperator
(* JSDHashEnumerator)(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32_t number, void *arg);

extern JS_PUBLIC_API(uint32_t)
JS_DHashTableEnumerate(JSDHashTable *table, JSDHashEnumerator etor, void *arg);

typedef size_t
(* JSDHashSizeOfEntryFun)(JSDHashEntryHdr *hdr, JSMallocSizeOfFun mallocSizeOf);







extern JS_PUBLIC_API(size_t)
JS_DHashTableSizeOfExcludingThis(const JSDHashTable *table,
                                 JSDHashSizeOfEntryFun sizeOfEntry,
                                 JSMallocSizeOfFun mallocSizeOf);




extern JS_PUBLIC_API(size_t)
JS_DHashTableSizeOfIncludingThis(const JSDHashTable *table,
                                 JSDHashSizeOfEntryFun sizeOfEntry,
                                 JSMallocSizeOfFun mallocSizeOf);

#ifdef DEBUG














extern JS_PUBLIC_API(void)
JS_DHashMarkTableImmutable(JSDHashTable *table);
#endif

#ifdef JS_DHASHMETER
#include <stdio.h>

extern JS_PUBLIC_API(void)
JS_DHashTableDumpMeter(JSDHashTable *table, JSDHashEnumerator dump, FILE *fp);
#endif

JS_END_EXTERN_C

#endif 
