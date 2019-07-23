





































 









#include <stdlib.h>
#include <string.h>

#include "jsj_hash.h"
#include "jstypes.h"
#include "jsutil.h"
#include "jsbit.h"



#define NBUCKETS(ht)    (1 << (JSJ_HASH_BITS - (ht)->shift))


#define MINBUCKETSLOG2  4
#define MINBUCKETS      (1 << MINBUCKETSLOG2)


#define OVERLOADED(n)   ((n) - ((n) >> 3))


#define UNDERLOADED(n)  (((n) > MINBUCKETS) ? ((n) >> 2) : 0)




static void *
DefaultAllocTable(void *pool, size_t size)
{
    return malloc(size);
}

static void
DefaultFreeTable(void *pool, void *item)
{
    free(item);
}

static JSJHashEntry *
DefaultAllocEntry(void *pool, const void *key)
{
    return malloc(sizeof(JSJHashEntry));
}

static void
DefaultFreeEntry(void *pool, JSJHashEntry *he, JSUintn flag)
{
    if (flag == HT_FREE_ENTRY)
        free(he);
}

static JSJHashAllocOps defaultHashAllocOps = {
    DefaultAllocTable, DefaultFreeTable,
    DefaultAllocEntry, DefaultFreeEntry
};

JS_EXPORT_API(JSJHashTable *)
JSJ_NewHashTable(JSUint32 n, JSJHashFunction keyHash,
                JSJHashComparator keyCompare, JSJHashComparator valueCompare,
                JSJHashAllocOps *allocOps, void *allocPriv)
{
    JSJHashTable *ht;
    JSUint32 nb;

    if (n <= MINBUCKETS) {
        n = MINBUCKETSLOG2;
    } else {
        n = JS_CeilingLog2(n);
        if ((JSInt32)n < 0)
            return 0;
    }

    if (!allocOps) allocOps = &defaultHashAllocOps;

    ht = (*allocOps->allocTable)(allocPriv, sizeof *ht);
    if (!ht)
    return 0;
    memset(ht, 0, sizeof *ht);
    ht->shift = JSJ_HASH_BITS - n;
    n = 1 << n;
#if (defined(XP_WIN) || defined(XP_OS2)) && !defined(_WIN32)
    if (n > 16000) {
        (*allocOps->freeTable)(allocPriv, ht);
        return 0;
    }
#endif  
    nb = n * sizeof(JSJHashEntry *);
    ht->buckets = (*allocOps->allocTable)(allocPriv, nb);
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

JS_EXPORT_API(void)
JSJ_HashTableDestroy(JSJHashTable *ht)
{
    JSUint32 i, n;
    JSJHashEntry *he, *next;
    JSJHashAllocOps *allocOps = ht->allocOps;
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




#define GOLDEN_RATIO    0x9E3779B9U

JS_EXPORT_API(JSJHashEntry **)
JSJ_HashTableRawLookup(JSJHashTable *ht, JSJHashNumber keyHash, const void *key, void *arg)
{
    JSJHashEntry *he, **hep, **hep0;
    JSJHashNumber h;

#ifdef HASHMETER
    ht->nlookups++;
#endif
    h = keyHash * GOLDEN_RATIO;
    h >>= ht->shift;
    hep = hep0 = &ht->buckets[h];
    while ((he = *hep) != 0) {
        if (he->keyHash == keyHash && (*ht->keyCompare)(key, he->key, arg)) {
            
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

JS_EXPORT_API(JSJHashEntry *)
JSJ_HashTableRawAdd(JSJHashTable *ht, JSJHashEntry **hep,
                    JSJHashNumber keyHash, const void *key, void *value,
                    void *arg)
{
    JSUint32 i, n;
    JSJHashEntry *he, *next, **oldbuckets;
    JSUint32 nb;

    
    n = NBUCKETS(ht);
    if (ht->nentries >= OVERLOADED(n)) {
#ifdef HASHMETER
        ht->ngrows++;
#endif
        ht->shift--;
        oldbuckets = ht->buckets;
#if (defined(XP_WIN) || defined(XP_OS2)) && !defined(_WIN32)
        if (2 * n > 16000)
            return 0;
#endif  
        nb = 2 * n * sizeof(JSJHashEntry *);
        ht->buckets = (*ht->allocOps->allocTable)(ht->allocPriv, nb);
        if (!ht->buckets) {
            ht->buckets = oldbuckets;
            return 0;
        }
        memset(ht->buckets, 0, nb);

        for (i = 0; i < n; i++) {
            for (he = oldbuckets[i]; he; he = next) {
                next = he->next;
                hep = JSJ_HashTableRawLookup(ht, he->keyHash, he->key, arg);
                JS_ASSERT(*hep == 0);
                he->next = 0;
                *hep = he;
            }
        }
#ifdef DEBUG
        memset(oldbuckets, 0xDB, n * sizeof oldbuckets[0]);
#endif
        (*ht->allocOps->freeTable)(ht->allocPriv, oldbuckets);
        hep = JSJ_HashTableRawLookup(ht, keyHash, key, arg);
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

JS_EXPORT_API(JSJHashEntry *)
JSJ_HashTableAdd(JSJHashTable *ht, const void *key, void *value, void *arg)
{
    JSJHashNumber keyHash;
    JSJHashEntry *he, **hep;

    keyHash = (*ht->keyHash)(key, arg);
    hep = JSJ_HashTableRawLookup(ht, keyHash, key, arg);
    if ((he = *hep) != 0) {
        
        if ((*ht->valueCompare)(he->value, value, arg)) {
            
            return he;
        }
        if (he->value)
            (*ht->allocOps->freeEntry)(ht->allocPriv, he, HT_FREE_VALUE);
        he->value = value;
        return he;
    }
    return JSJ_HashTableRawAdd(ht, hep, keyHash, key, value, arg);
}

JS_EXPORT_API(void)
JSJ_HashTableRawRemove(JSJHashTable *ht, JSJHashEntry **hep, JSJHashEntry *he, void *arg)
{
    JSUint32 i, n;
    JSJHashEntry *next, **oldbuckets;
    JSUint32 nb;

    *hep = he->next;
    (*ht->allocOps->freeEntry)(ht->allocPriv, he, HT_FREE_ENTRY);

    
    n = NBUCKETS(ht);
    if (--ht->nentries < UNDERLOADED(n)) {
#ifdef HASHMETER
        ht->nshrinks++;
#endif
        ht->shift++;
        oldbuckets = ht->buckets;
        nb = n * sizeof(JSJHashEntry*) / 2;
        ht->buckets = (*ht->allocOps->allocTable)(ht->allocPriv, nb);
        if (!ht->buckets) {
            ht->buckets = oldbuckets;
            return;
        }
        memset(ht->buckets, 0, nb);

        for (i = 0; i < n; i++) {
            for (he = oldbuckets[i]; he; he = next) {
                next = he->next;
                hep = JSJ_HashTableRawLookup(ht, he->keyHash, he->key, arg);
                JS_ASSERT(*hep == 0);
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

JS_EXPORT_API(JSBool)
JSJ_HashTableRemove(JSJHashTable *ht, const void *key, void *arg)
{
    JSJHashNumber keyHash;
    JSJHashEntry *he, **hep;

    keyHash = (*ht->keyHash)(key, arg);
    hep = JSJ_HashTableRawLookup(ht, keyHash, key, arg);
    if ((he = *hep) == 0)
        return JS_FALSE;

    
    JSJ_HashTableRawRemove(ht, hep, he, arg);
    return JS_TRUE;
}

JS_EXPORT_API(void *)
JSJ_HashTableLookup(JSJHashTable *ht, const void *key, void *arg)
{
    JSJHashNumber keyHash;
    JSJHashEntry *he, **hep;

    keyHash = (*ht->keyHash)(key, arg);
    hep = JSJ_HashTableRawLookup(ht, keyHash, key, arg);
    if ((he = *hep) != 0) {
        return he->value;
    }
    return 0;
}






JS_EXPORT_API(int)
JSJ_HashTableEnumerateEntries(JSJHashTable *ht, JSJHashEnumerator f, void *arg)
{
    JSJHashEntry *he, **hep;
    JSUint32 i, nbuckets;
    int rv, n = 0;
    JSJHashEntry *todo = 0;

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
        JSJ_HashTableRawRemove(ht, hep, he, arg);
    }
    return n;
}

#ifdef HASHMETER
#include <math.h>
#include <stdio.h>

JS_EXPORT_API(void)
JSJ_HashTableDumpMeter(JSJHashTable *ht, JSJHashEnumerator dump, FILE *fp)
{
    double mean, variance;
    JSUint32 nchains, nbuckets;
    JSUint32 i, n, maxChain, maxChainLen;
    JSJHashEntry *he;

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

JS_EXPORT_API(int)
JSJ_HashTableDump(JSJHashTable *ht, JSJHashEnumerator dump, FILE *fp)
{
    int count;

    count = JSJ_HashTableEnumerateEntries(ht, dump, fp);
#ifdef HASHMETER
    JSJ_HashTableDumpMeter(ht, dump, fp);
#endif
    return count;
}

JS_EXPORT_API(JSJHashNumber)
JSJ_HashString(const void *key)
{
    JSJHashNumber h;
    const unsigned char *s;

    h = 0;
    for (s = key; *s; s++)
        h = (h >> 28) ^ (h << 4) ^ *s;
    return h;
}

JS_EXPORT_API(int)
JSJ_CompareStrings(const void *v1, const void *v2)
{
    return strcmp(v1, v2) == 0;
}

JS_EXPORT_API(int)
JSJ_CompareValues(const void *v1, const void *v2)
{
    return v1 == v2;
}
