















































#ifndef jsj_hash_h___
#define jsj_hash_h___



#include <stddef.h>
#include <stdio.h>
#include "jstypes.h"

JS_BEGIN_EXTERN_C

typedef struct JSJHashEntry  JSJHashEntry;
typedef struct JSJHashTable  JSJHashTable;
typedef JSUint32 JSJHashNumber;
#define JSJ_HASH_BITS 32
typedef JSJHashNumber (* JS_DLL_CALLBACK JSJHashFunction)(const void *key, void *arg);
typedef JSIntn (* JS_DLL_CALLBACK JSJHashComparator)(const void *v1, const void *v2, void *arg);
typedef JSIntn (* JS_DLL_CALLBACK JSJHashEnumerator)(JSJHashEntry *he, JSIntn i, void *arg);


#define HT_ENUMERATE_NEXT       0       /* continue enumerating entries */
#define HT_ENUMERATE_STOP       1       /* stop enumerating entries */
#define HT_ENUMERATE_REMOVE     2       /* remove and free the current entry */
#define HT_ENUMERATE_UNHASH     4       /* just unhash the current entry */

typedef struct JSJHashAllocOps {
    void *              (*allocTable)(void *pool, size_t size);
    void                (*freeTable)(void *pool, void *item);
    JSJHashEntry *      (*allocEntry)(void *pool, const void *key);
    void                (*freeEntry)(void *pool, JSJHashEntry *he, JSUintn flag);
} JSJHashAllocOps;

#define HT_FREE_VALUE   0               /* just free the entry's value */
#define HT_FREE_ENTRY   1               /* free value and entire entry */

struct JSJHashEntry {
    JSJHashEntry        *next;          
    JSJHashNumber       keyHash;        
    const void          *key;           
    void                *value;         
};

struct JSJHashTable {
    JSJHashEntry         **buckets;      
    JSUint32              nentries;       
    JSUint32              shift;          
    JSJHashFunction     keyHash;        
    JSJHashComparator   keyCompare;     
    JSJHashComparator   valueCompare;   
    JSJHashAllocOps     *allocOps;      
    void                *allocPriv;     
#ifdef HASHMETER
    JSUint32              nlookups;       
    JSUint32              nsteps;         
    JSUint32              ngrows;         
    JSUint32              nshrinks;       
#endif
};





JS_EXTERN_API(JSJHashTable *)
JSJ_NewHashTable(JSUint32 n, JSJHashFunction keyHash,
                JSJHashComparator keyCompare, JSJHashComparator valueCompare,
                JSJHashAllocOps *allocOps, void *allocPriv);

JS_EXTERN_API(void)
JSJ_HashTableDestroy(JSJHashTable *ht);


JS_EXTERN_API(JSJHashEntry **)
JSJ_HashTableRawLookup(JSJHashTable *ht, JSJHashNumber keyHash, const void *key, void *arg);

JS_EXTERN_API(JSJHashEntry *)
JSJ_HashTableRawAdd(JSJHashTable *ht, JSJHashEntry **hep, JSJHashNumber keyHash,
                   const void *key, void *value, void *arg);

JS_EXTERN_API(void)
JSJ_HashTableRawRemove(JSJHashTable *ht, JSJHashEntry **hep, JSJHashEntry *he, void *arg);


JS_EXTERN_API(JSJHashEntry *)
JSJ_HashTableAdd(JSJHashTable *ht, const void *key, void *value, void *arg);

JS_EXTERN_API(JSBool)
JSJ_HashTableRemove(JSJHashTable *ht, const void *key, void *arg);

JS_EXTERN_API(JSIntn)
JSJ_HashTableEnumerateEntries(JSJHashTable *ht, JSJHashEnumerator f, void *arg);

JS_EXTERN_API(void *)
JSJ_HashTableLookup(JSJHashTable *ht, const void *key, void *arg);

JS_EXTERN_API(JSIntn)
JSJ_HashTableDump(JSJHashTable *ht, JSJHashEnumerator dump, FILE *fp);


JS_EXTERN_API(JSJHashNumber)
JSJ_HashString(const void *key);


JS_EXTERN_API(int)
JSJ_CompareStrings(const void *v1, const void *v2);


JS_EXTERN_API(JSIntn)
JSJ_CompareValues(const void *v1, const void *v2);

JS_END_EXTERN_C

#endif 
