





































#ifndef jsdhash_h___
#define jsdhash_h___



#include "jstypes.h"

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


typedef uint32                  JSDHashNumber;
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
    int16               hashShift;      
    uint8               maxAlphaFrac;   
    uint8               minAlphaFrac;   
    uint32              entrySize;      
    uint32              entryCount;     
    uint32              removedCount;   
    uint32              generation;     
    char                *entryStore;    
#ifdef JS_DHASHMETER
    struct JSDHashStats {
        uint32          searches;       
        uint32          steps;          
        uint32          hits;           
        uint32          misses;         
        uint32          lookups;        
        uint32          addMisses;      
        uint32          addOverRemoved; 
        uint32          addHits;        
        uint32          addFailures;    
        uint32          removeHits;     
        uint32          removeMisses;   
        uint32          removeFrees;    
        uint32          removeEnums;    
        uint32          grows;          
        uint32          shrinks;        
        uint32          compresses;     
        uint32          enumShrinks;    
    } stats;
#endif
};






#define JS_DHASH_TABLE_SIZE(table)  JS_BIT(JS_DHASH_BITS - (table)->hashShift)






typedef void *
(* JS_DLL_CALLBACK JSDHashAllocTable)(JSDHashTable *table, uint32 nbytes);

typedef void
(* JS_DLL_CALLBACK JSDHashFreeTable) (JSDHashTable *table, void *ptr);







typedef const void *
(* JS_DLL_CALLBACK JSDHashGetKey)    (JSDHashTable *table,
                                      JSDHashEntryHdr *entry);





typedef JSDHashNumber
(* JS_DLL_CALLBACK JSDHashHashKey)   (JSDHashTable *table, const void *key);





typedef JSBool
(* JS_DLL_CALLBACK JSDHashMatchEntry)(JSDHashTable *table,
                                      const JSDHashEntryHdr *entry,
                                      const void *key);







typedef void
(* JS_DLL_CALLBACK JSDHashMoveEntry)(JSDHashTable *table,
                                     const JSDHashEntryHdr *from,
                                     JSDHashEntryHdr *to);






typedef void
(* JS_DLL_CALLBACK JSDHashClearEntry)(JSDHashTable *table,
                                      JSDHashEntryHdr *entry);






typedef void
(* JS_DLL_CALLBACK JSDHashFinalize)  (JSDHashTable *table);








typedef JSBool
(* JS_DLL_CALLBACK JSDHashInitEntry)(JSDHashTable *table,
                                     JSDHashEntryHdr *entry,
                                     const void *key);




























struct JSDHashTableOps {
    
    JSDHashAllocTable   allocTable;
    JSDHashFreeTable    freeTable;
    JSDHashGetKey       getKey;
    JSDHashHashKey      hashKey;
    JSDHashMatchEntry   matchEntry;
    JSDHashMoveEntry    moveEntry;
    JSDHashClearEntry   clearEntry;
    JSDHashFinalize     finalize;

    
    JSDHashInitEntry    initEntry;
};




extern JS_PUBLIC_API(void *)
JS_DHashAllocTable(JSDHashTable *table, uint32 nbytes);

extern JS_PUBLIC_API(void)
JS_DHashFreeTable(JSDHashTable *table, void *ptr);

extern JS_PUBLIC_API(JSDHashNumber)
JS_DHashStringKey(JSDHashTable *table, const void *key);


struct JSDHashEntryStub {
    JSDHashEntryHdr hdr;
    const void      *key;
};

extern JS_PUBLIC_API(const void *)
JS_DHashGetKeyStub(JSDHashTable *table, JSDHashEntryHdr *entry);

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
JS_NewDHashTable(const JSDHashTableOps *ops, void *data, uint32 entrySize,
                 uint32 capacity);





extern JS_PUBLIC_API(void)
JS_DHashTableDestroy(JSDHashTable *table);







extern JS_PUBLIC_API(JSBool)
JS_DHashTableInit(JSDHashTable *table, const JSDHashTableOps *ops, void *data,
                  uint32 entrySize, uint32 capacity);









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
    ((uint32)((double)(entryCount) / (maxAlpha)))

#define JS_DHASH_CAPACITY(entryCount, maxAlpha)                               \
    (JS_DHASH_CAP(entryCount, maxAlpha) +                                     \
     (((JS_DHASH_CAP(entryCount, maxAlpha) * (uint8)(0x100 * (maxAlpha)))     \
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
(* JS_DLL_CALLBACK JSDHashEnumerator)(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                      uint32 number, void *arg);

extern JS_PUBLIC_API(uint32)
JS_DHashTableEnumerate(JSDHashTable *table, JSDHashEnumerator etor, void *arg);

#ifdef JS_DHASHMETER
#include <stdio.h>

extern JS_PUBLIC_API(void)
JS_DHashTableDumpMeter(JSDHashTable *table, JSDHashEnumerator dump, FILE *fp);
#endif

JS_END_EXTERN_C

#endif 
