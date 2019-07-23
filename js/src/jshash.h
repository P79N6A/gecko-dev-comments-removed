






































#ifndef jshash_h___
#define jshash_h___



#include <stddef.h>
#include <stdio.h>
#include "jstypes.h"
#include "jscompat.h"

JS_BEGIN_EXTERN_C

typedef uint32 JSHashNumber;
typedef struct JSHashEntry JSHashEntry;
typedef struct JSHashTable JSHashTable;

#define JS_HASH_BITS 32
#define JS_GOLDEN_RATIO 0x9E3779B9U

typedef JSHashNumber (* JS_DLL_CALLBACK JSHashFunction)(const void *key);
typedef intN (* JS_DLL_CALLBACK JSHashComparator)(const void *v1, const void *v2);
typedef intN (* JS_DLL_CALLBACK JSHashEnumerator)(JSHashEntry *he, intN i, void *arg);


#define HT_ENUMERATE_NEXT       0       /* continue enumerating entries */
#define HT_ENUMERATE_STOP       1       /* stop enumerating entries */
#define HT_ENUMERATE_REMOVE     2       /* remove and free the current entry */

typedef struct JSHashAllocOps {
    void *              (*allocTable)(void *pool, size_t size);
    void                (*freeTable)(void *pool, void *item);
    JSHashEntry *       (*allocEntry)(void *pool, const void *key);
    void                (*freeEntry)(void *pool, JSHashEntry *he, uintN flag);
} JSHashAllocOps;

#define HT_FREE_VALUE   0               /* just free the entry's value */
#define HT_FREE_ENTRY   1               /* free value and entire entry */

struct JSHashEntry {
    JSHashEntry         *next;          
    JSHashNumber        keyHash;        
    const void          *key;           
    void                *value;         
};

struct JSHashTable {
    JSHashEntry         **buckets;      
    uint32              nentries;       
    uint32              shift;          
    JSHashFunction      keyHash;        
    JSHashComparator    keyCompare;     
    JSHashComparator    valueCompare;   
    JSHashAllocOps      *allocOps;      
    void                *allocPriv;     
#ifdef HASHMETER
    uint32              nlookups;       
    uint32              nsteps;         
    uint32              ngrows;         
    uint32              nshrinks;       
#endif
};





extern JS_PUBLIC_API(JSHashTable *)
JS_NewHashTable(uint32 n, JSHashFunction keyHash,
                JSHashComparator keyCompare, JSHashComparator valueCompare,
                JSHashAllocOps *allocOps, void *allocPriv);

extern JS_PUBLIC_API(void)
JS_HashTableDestroy(JSHashTable *ht);


extern JS_PUBLIC_API(JSHashEntry **)
JS_HashTableRawLookup(JSHashTable *ht, JSHashNumber keyHash, const void *key);

extern JS_PUBLIC_API(JSHashEntry *)
JS_HashTableRawAdd(JSHashTable *ht, JSHashEntry **hep, JSHashNumber keyHash,
                   const void *key, void *value);

extern JS_PUBLIC_API(void)
JS_HashTableRawRemove(JSHashTable *ht, JSHashEntry **hep, JSHashEntry *he);


extern JS_PUBLIC_API(JSHashEntry *)
JS_HashTableAdd(JSHashTable *ht, const void *key, void *value);

extern JS_PUBLIC_API(JSBool)
JS_HashTableRemove(JSHashTable *ht, const void *key);

extern JS_PUBLIC_API(intN)
JS_HashTableEnumerateEntries(JSHashTable *ht, JSHashEnumerator f, void *arg);

extern JS_PUBLIC_API(void *)
JS_HashTableLookup(JSHashTable *ht, const void *key);

extern JS_PUBLIC_API(intN)
JS_HashTableDump(JSHashTable *ht, JSHashEnumerator dump, FILE *fp);


extern JS_PUBLIC_API(JSHashNumber)
JS_HashString(const void *key);


extern JS_PUBLIC_API(intN)
JS_CompareValues(const void *v1, const void *v2);

JS_END_EXTERN_C

#endif 
