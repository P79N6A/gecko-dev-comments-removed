







































#include "plhash.h"
#include "prbit.h"
#include "prlog.h"
#include "prmem.h"
#include "prtypes.h"
#include <stdlib.h>
#include <string.h>


#define NBUCKETS(ht)    (1 << (PL_HASH_BITS - (ht)->shift))


#define MINBUCKETSLOG2  4
#define MINBUCKETS      (1 << MINBUCKETSLOG2)


#define OVERLOADED(n)   ((n) - ((n) >> 3))


#define UNDERLOADED(n)  (((n) > MINBUCKETS) ? ((n) >> 2) : 0)




static void * PR_CALLBACK
DefaultAllocTable(void *pool, PRSize size)
{
    return PR_MALLOC(size);
}

static void PR_CALLBACK
DefaultFreeTable(void *pool, void *item)
{
    PR_Free(item);
}

static PLHashEntry * PR_CALLBACK
DefaultAllocEntry(void *pool, const void *key)
{
    return PR_NEW(PLHashEntry);
}

static void PR_CALLBACK
DefaultFreeEntry(void *pool, PLHashEntry *he, PRUintn flag)
{
    if (flag == HT_FREE_ENTRY)
        PR_Free(he);
}

static PLHashAllocOps defaultHashAllocOps = {
    DefaultAllocTable, DefaultFreeTable,
    DefaultAllocEntry, DefaultFreeEntry
};

PR_IMPLEMENT(PLHashTable *)
PL_NewHashTable(PRUint32 n, PLHashFunction keyHash,
                PLHashComparator keyCompare, PLHashComparator valueCompare,
                const PLHashAllocOps *allocOps, void *allocPriv)
{
    PLHashTable *ht;
    PRSize nb;

    if (n <= MINBUCKETS) {
        n = MINBUCKETSLOG2;
    } else {
        n = PR_CeilingLog2(n);
        if ((PRInt32)n < 0)
            return 0;
    }

    if (!allocOps) allocOps = &defaultHashAllocOps;

    ht = (PLHashTable*)((*allocOps->allocTable)(allocPriv, sizeof *ht));
    if (!ht)
	return 0;
    memset(ht, 0, sizeof *ht);
    ht->shift = PL_HASH_BITS - n;
    n = 1 << n;
    nb = n * sizeof(PLHashEntry *);
    ht->buckets = (PLHashEntry**)((*allocOps->allocTable)(allocPriv, nb));
    if (!ht->buckets) {
        (*allocOps->freeTable)(allocPriv, ht);
        return 0;
    }
    memset(ht->buckets, 0, nb);

    ht->keyHash = keyHash;
    ht->keyCompare = keyCompare;
    ht->valueCompare = valueCompare;
    ht->allocOps = allocOps;
    ht->allocPriv = allocPriv;
    return ht;
}

PR_IMPLEMENT(void)
PL_HashTableDestroy(PLHashTable *ht)
{
    PRUint32 i, n;
    PLHashEntry *he, *next;
    const PLHashAllocOps *allocOps = ht->allocOps;
    void *allocPriv = ht->allocPriv;

    n = NBUCKETS(ht);
    for (i = 0; i < n; i++) {
        for (he = ht->buckets[i]; he; he = next) {
            next = he->next;
            (*allocOps->freeEntry)(allocPriv, he, HT_FREE_ENTRY);
        }
    }
#ifdef DEBUG
    memset(ht->buckets, 0xDB, n * sizeof ht->buckets[0]);
#endif
    (*allocOps->freeTable)(allocPriv, ht->buckets);
#ifdef DEBUG
    memset(ht, 0xDB, sizeof *ht);
#endif
    (*allocOps->freeTable)(allocPriv, ht);
}




#define GOLDEN_RATIO    0x9E3779B9U  /* 2/(1+sqrt(5))*(2^32) */

PR_IMPLEMENT(PLHashEntry **)
PL_HashTableRawLookup(PLHashTable *ht, PLHashNumber keyHash, const void *key)
{
    PLHashEntry *he, **hep, **hep0;
    PLHashNumber h;

#ifdef HASHMETER
    ht->nlookups++;
#endif
    h = keyHash * GOLDEN_RATIO;
    h >>= ht->shift;
    hep = hep0 = &ht->buckets[h];
    while ((he = *hep) != 0) {
        if (he->keyHash == keyHash && (*ht->keyCompare)(key, he->key)) {
            
            if (hep != hep0) {
                *hep = he->next;
                he->next = *hep0;
                *hep0 = he;
            }
            return hep0;
        }
        hep = &he->next;
#ifdef HASHMETER
        ht->nsteps++;
#endif
    }
    return hep;
}




PR_IMPLEMENT(PLHashEntry **)
PL_HashTableRawLookupConst(PLHashTable *ht, PLHashNumber keyHash,
                           const void *key)
{
    PLHashEntry *he, **hep;
    PLHashNumber h;

#ifdef HASHMETER
    ht->nlookups++;
#endif
    h = keyHash * GOLDEN_RATIO;
    h >>= ht->shift;
    hep = &ht->buckets[h];
    while ((he = *hep) != 0) {
        if (he->keyHash == keyHash && (*ht->keyCompare)(key, he->key)) {
            break;
        }
        hep = &he->next;
#ifdef HASHMETER
        ht->nsteps++;
#endif
    }
    return hep;
}

PR_IMPLEMENT(PLHashEntry *)
PL_HashTableRawAdd(PLHashTable *ht, PLHashEntry **hep,
                   PLHashNumber keyHash, const void *key, void *value)
{
    PRUint32 i, n;
    PLHashEntry *he, *next, **oldbuckets;
    PRSize nb;

    
    n = NBUCKETS(ht);
    if (ht->nentries >= OVERLOADED(n)) {
        oldbuckets = ht->buckets;
        nb = 2 * n * sizeof(PLHashEntry *);
        ht->buckets = (PLHashEntry**)
            ((*ht->allocOps->allocTable)(ht->allocPriv, nb));
        if (!ht->buckets) {
            ht->buckets = oldbuckets;
            return 0;
        }
        memset(ht->buckets, 0, nb);
#ifdef HASHMETER
        ht->ngrows++;
#endif
        ht->shift--;

        for (i = 0; i < n; i++) {
            for (he = oldbuckets[i]; he; he = next) {
                next = he->next;
                hep = PL_HashTableRawLookup(ht, he->keyHash, he->key);
                PR_ASSERT(*hep == 0);
                he->next = 0;
                *hep = he;
            }
        }
#ifdef DEBUG
        memset(oldbuckets, 0xDB, n * sizeof oldbuckets[0]);
#endif
        (*ht->allocOps->freeTable)(ht->allocPriv, oldbuckets);
        hep = PL_HashTableRawLookup(ht, keyHash, key);
    }

    
    he = (*ht->allocOps->allocEntry)(ht->allocPriv, key);
    if (!he)
	return 0;
    he->keyHash = keyHash;
    he->key = key;
    he->value = value;
    he->next = *hep;
    *hep = he;
    ht->nentries++;
    return he;
}

PR_IMPLEMENT(PLHashEntry *)
PL_HashTableAdd(PLHashTable *ht, const void *key, void *value)
{
    PLHashNumber keyHash;
    PLHashEntry *he, **hep;

    keyHash = (*ht->keyHash)(key);
    hep = PL_HashTableRawLookup(ht, keyHash, key);
    if ((he = *hep) != 0) {
        
        if ((*ht->valueCompare)(he->value, value)) {
            
            return he;
        }
        if (he->value)
            (*ht->allocOps->freeEntry)(ht->allocPriv, he, HT_FREE_VALUE);
        he->value = value;
        return he;
    }
    return PL_HashTableRawAdd(ht, hep, keyHash, key, value);
}

PR_IMPLEMENT(void)
PL_HashTableRawRemove(PLHashTable *ht, PLHashEntry **hep, PLHashEntry *he)
{
    PRUint32 i, n;
    PLHashEntry *next, **oldbuckets;
    PRSize nb;

    *hep = he->next;
    (*ht->allocOps->freeEntry)(ht->allocPriv, he, HT_FREE_ENTRY);

    
    n = NBUCKETS(ht);
    if (--ht->nentries < UNDERLOADED(n)) {
        oldbuckets = ht->buckets;
        nb = n * sizeof(PLHashEntry*) / 2;
        ht->buckets = (PLHashEntry**)(
            (*ht->allocOps->allocTable)(ht->allocPriv, nb));
        if (!ht->buckets) {
            ht->buckets = oldbuckets;
            return;
        }
        memset(ht->buckets, 0, nb);
#ifdef HASHMETER
        ht->nshrinks++;
#endif
        ht->shift++;

        for (i = 0; i < n; i++) {
            for (he = oldbuckets[i]; he; he = next) {
                next = he->next;
                hep = PL_HashTableRawLookup(ht, he->keyHash, he->key);
                PR_ASSERT(*hep == 0);
                he->next = 0;
                *hep = he;
            }
        }
#ifdef DEBUG
        memset(oldbuckets, 0xDB, n * sizeof oldbuckets[0]);
#endif
        (*ht->allocOps->freeTable)(ht->allocPriv, oldbuckets);
    }
}

PR_IMPLEMENT(PRBool)
PL_HashTableRemove(PLHashTable *ht, const void *key)
{
    PLHashNumber keyHash;
    PLHashEntry *he, **hep;

    keyHash = (*ht->keyHash)(key);
    hep = PL_HashTableRawLookup(ht, keyHash, key);
    if ((he = *hep) == 0)
        return PR_FALSE;

    
    PL_HashTableRawRemove(ht, hep, he);
    return PR_TRUE;
}

PR_IMPLEMENT(void *)
PL_HashTableLookup(PLHashTable *ht, const void *key)
{
    PLHashNumber keyHash;
    PLHashEntry *he, **hep;

    keyHash = (*ht->keyHash)(key);
    hep = PL_HashTableRawLookup(ht, keyHash, key);
    if ((he = *hep) != 0) {
        return he->value;
    }
    return 0;
}




PR_IMPLEMENT(void *)
PL_HashTableLookupConst(PLHashTable *ht, const void *key)
{
    PLHashNumber keyHash;
    PLHashEntry *he, **hep;

    keyHash = (*ht->keyHash)(key);
    hep = PL_HashTableRawLookupConst(ht, keyHash, key);
    if ((he = *hep) != 0) {
        return he->value;
    }
    return 0;
}






PR_IMPLEMENT(int)
PL_HashTableEnumerateEntries(PLHashTable *ht, PLHashEnumerator f, void *arg)
{
    PLHashEntry *he, **hep;
    PRUint32 i, nbuckets;
    int rv, n = 0;
    PLHashEntry *todo = 0;

    nbuckets = NBUCKETS(ht);
    for (i = 0; i < nbuckets; i++) {
        hep = &ht->buckets[i];
        while ((he = *hep) != 0) {
            rv = (*f)(he, n, arg);
            n++;
            if (rv & (HT_ENUMERATE_REMOVE | HT_ENUMERATE_UNHASH)) {
                *hep = he->next;
                if (rv & HT_ENUMERATE_REMOVE) {
                    he->next = todo;
                    todo = he;
                }
            } else {
                hep = &he->next;
            }
            if (rv & HT_ENUMERATE_STOP) {
                goto out;
            }
        }
    }

out:
    hep = &todo;
    while ((he = *hep) != 0) {
        PL_HashTableRawRemove(ht, hep, he);
    }
    return n;
}

#ifdef HASHMETER
#include <math.h>
#include <stdio.h>

PR_IMPLEMENT(void)
PL_HashTableDumpMeter(PLHashTable *ht, PLHashEnumerator dump, FILE *fp)
{
    double mean, variance;
    PRUint32 nchains, nbuckets;
    PRUint32 i, n, maxChain, maxChainLen;
    PLHashEntry *he;

    variance = 0;
    nchains = 0;
    maxChainLen = 0;
    nbuckets = NBUCKETS(ht);
    for (i = 0; i < nbuckets; i++) {
        he = ht->buckets[i];
        if (!he)
            continue;
        nchains++;
        for (n = 0; he; he = he->next)
            n++;
        variance += n * n;
        if (n > maxChainLen) {
            maxChainLen = n;
            maxChain = i;
        }
    }
    mean = (double)ht->nentries / nchains;
    variance = fabs(variance / nchains - mean * mean);

    fprintf(fp, "\nHash table statistics:\n");
    fprintf(fp, "     number of lookups: %u\n", ht->nlookups);
    fprintf(fp, "     number of entries: %u\n", ht->nentries);
    fprintf(fp, "       number of grows: %u\n", ht->ngrows);
    fprintf(fp, "     number of shrinks: %u\n", ht->nshrinks);
    fprintf(fp, "   mean steps per hash: %g\n", (double)ht->nsteps
                                                / ht->nlookups);
    fprintf(fp, "mean hash chain length: %g\n", mean);
    fprintf(fp, "    standard deviation: %g\n", sqrt(variance));
    fprintf(fp, " max hash chain length: %u\n", maxChainLen);
    fprintf(fp, "        max hash chain: [%u]\n", maxChain);

    for (he = ht->buckets[maxChain], i = 0; he; he = he->next, i++)
        if ((*dump)(he, i, fp) != HT_ENUMERATE_NEXT)
            break;
}
#endif 

PR_IMPLEMENT(int)
PL_HashTableDump(PLHashTable *ht, PLHashEnumerator dump, FILE *fp)
{
    int count;

    count = PL_HashTableEnumerateEntries(ht, dump, fp);
#ifdef HASHMETER
    PL_HashTableDumpMeter(ht, dump, fp);
#endif
    return count;
}

PR_IMPLEMENT(PLHashNumber)
PL_HashString(const void *key)
{
    PLHashNumber h;
    const PRUint8 *s;

    h = 0;
    for (s = (const PRUint8*)key; *s; s++)
        h = PR_ROTATE_LEFT32(h, 4) ^ *s;
    return h;
}

PR_IMPLEMENT(int)
PL_CompareStrings(const void *v1, const void *v2)
{
    return strcmp((const char*)v1, (const char*)v2) == 0;
}

PR_IMPLEMENT(int)
PL_CompareValues(const void *v1, const void *v2)
{
    return v1 == v2;
}
