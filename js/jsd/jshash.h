





#ifndef jshash_h___
#define jshash_h___




#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {

typedef uint32_t JSHashNumber;
typedef struct JSHashEntry JSHashEntry;
typedef struct JSHashTable JSHashTable;

#define JS_HASH_BITS 32
#define JS_GOLDEN_RATIO 0x9E3779B9U

typedef JSHashNumber (* JSHashFunction)(const void *key);
typedef int (* JSHashComparator)(const void *v1, const void *v2);
typedef int (* JSHashEnumerator)(JSHashEntry *he, int i, void *arg);


#define HT_ENUMERATE_NEXT       0       /* continue enumerating entries */
#define HT_ENUMERATE_STOP       1       /* stop enumerating entries */
#define HT_ENUMERATE_REMOVE     2       /* remove and free the current entry */

typedef struct JSHashAllocOps {
    void *              (*allocTable)(void *pool, size_t size);
    void                (*freeTable)(void *pool, void *item, size_t size);
    JSHashEntry *       (*allocEntry)(void *pool, const void *key);
    void                (*freeEntry)(void *pool, JSHashEntry *he, unsigned flag);
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
    uint32_t            nentries;       
    uint32_t            shift;          
    JSHashFunction      keyHash;        
    JSHashComparator    keyCompare;     
    JSHashComparator    valueCompare;   
    const JSHashAllocOps *allocOps;     
    void                *allocPriv;     
#ifdef JS_HASHMETER
    uint32_t            nlookups;       
    uint32_t            nsteps;         
    uint32_t            ngrows;         
    uint32_t            nshrinks;       
#endif
};





extern JSHashTable * 
JS_NewHashTable(uint32_t n, JSHashFunction keyHash,
                JSHashComparator keyCompare, JSHashComparator valueCompare,
                const JSHashAllocOps *allocOps, void *allocPriv);

extern void
JS_HashTableDestroy(JSHashTable *ht);


extern JSHashEntry **
JS_HashTableRawLookup(JSHashTable *ht, JSHashNumber keyHash, const void *key);

extern JSHashEntry *
JS_HashTableRawAdd(JSHashTable *ht, JSHashEntry **&hep, JSHashNumber keyHash,
                   const void *key, void *value);

extern void
JS_HashTableRawRemove(JSHashTable *ht, JSHashEntry **hep, JSHashEntry *he);


extern JSHashEntry *
JS_HashTableAdd(JSHashTable *ht, const void *key, void *value);

extern bool
JS_HashTableRemove(JSHashTable *ht, const void *key);

extern int
JS_HashTableEnumerateEntries(JSHashTable *ht, JSHashEnumerator f, void *arg);

extern void *
JS_HashTableLookup(JSHashTable *ht, const void *key);

extern int
JS_HashTableDump(JSHashTable *ht, JSHashEnumerator dump, FILE *fp);


extern JSHashNumber
JS_HashString(const void *key);


extern int
JS_CompareValues(const void *v1, const void *v2);

} 

#endif 
