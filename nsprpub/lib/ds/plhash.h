




































#ifndef plhash_h___
#define plhash_h___



#include <stdio.h>
#include "prtypes.h"

PR_BEGIN_EXTERN_C

typedef struct PLHashEntry  PLHashEntry;
typedef struct PLHashTable  PLHashTable;
typedef PRUint32 PLHashNumber;
#define PL_HASH_BITS 32  /* Number of bits in PLHashNumber */
typedef PLHashNumber (PR_CALLBACK *PLHashFunction)(const void *key);
typedef PRIntn (PR_CALLBACK *PLHashComparator)(const void *v1, const void *v2);

typedef PRIntn (PR_CALLBACK *PLHashEnumerator)(PLHashEntry *he, PRIntn i, void *arg);


#define HT_ENUMERATE_NEXT       0       /* continue enumerating entries */
#define HT_ENUMERATE_STOP       1       /* stop enumerating entries */
#define HT_ENUMERATE_REMOVE     2       /* remove and free the current entry */
#define HT_ENUMERATE_UNHASH     4       /* just unhash the current entry */

typedef struct PLHashAllocOps {
    void *              (PR_CALLBACK *allocTable)(void *pool, PRSize size);
    void                (PR_CALLBACK *freeTable)(void *pool, void *item);
    PLHashEntry *       (PR_CALLBACK *allocEntry)(void *pool, const void *key);
    void                (PR_CALLBACK *freeEntry)(void *pool, PLHashEntry *he, PRUintn flag);
} PLHashAllocOps;

#define HT_FREE_VALUE   0               /* just free the entry's value */
#define HT_FREE_ENTRY   1               /* free value and entire entry */

struct PLHashEntry {
    PLHashEntry         *next;          
    PLHashNumber        keyHash;        
    const void          *key;           
    void                *value;         
};

struct PLHashTable {
    PLHashEntry         **buckets;      
    PRUint32              nentries;       
    PRUint32              shift;          
    PLHashFunction      keyHash;        
    PLHashComparator    keyCompare;     
    PLHashComparator    valueCompare;   
    const PLHashAllocOps *allocOps;     
    void                *allocPriv;     
#ifdef HASHMETER
    PRUint32              nlookups;       
    PRUint32              nsteps;         
    PRUint32              ngrows;         
    PRUint32              nshrinks;       
#endif
};





PR_EXTERN(PLHashTable *)
PL_NewHashTable(PRUint32 numBuckets, PLHashFunction keyHash,
                PLHashComparator keyCompare, PLHashComparator valueCompare,
                const PLHashAllocOps *allocOps, void *allocPriv);

PR_EXTERN(void)
PL_HashTableDestroy(PLHashTable *ht);


PR_EXTERN(PLHashEntry *)
PL_HashTableAdd(PLHashTable *ht, const void *key, void *value);

PR_EXTERN(PRBool)
PL_HashTableRemove(PLHashTable *ht, const void *key);

PR_EXTERN(void *)
PL_HashTableLookup(PLHashTable *ht, const void *key);

PR_EXTERN(void *)
PL_HashTableLookupConst(PLHashTable *ht, const void *key);

PR_EXTERN(PRIntn)
PL_HashTableEnumerateEntries(PLHashTable *ht, PLHashEnumerator f, void *arg);


PR_EXTERN(PLHashNumber)
PL_HashString(const void *key);


PR_EXTERN(PRIntn)
PL_CompareStrings(const void *v1, const void *v2);


PR_EXTERN(PRIntn)
PL_CompareValues(const void *v1, const void *v2);


PR_EXTERN(PLHashEntry **)
PL_HashTableRawLookup(PLHashTable *ht, PLHashNumber keyHash, const void *key);

PR_EXTERN(PLHashEntry **)
PL_HashTableRawLookupConst(PLHashTable *ht, PLHashNumber keyHash,
                           const void *key);

PR_EXTERN(PLHashEntry *)
PL_HashTableRawAdd(PLHashTable *ht, PLHashEntry **hep, PLHashNumber keyHash,
                   const void *key, void *value);

PR_EXTERN(void)
PL_HashTableRawRemove(PLHashTable *ht, PLHashEntry **hep, PLHashEntry *he);


PR_EXTERN(PRIntn)
PL_HashTableDump(PLHashTable *ht, PLHashEnumerator dump, FILE *fp);

PR_END_EXTERN_C

#endif 
