










































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsbit.h"
#include "jsdhash.h"
#include "jsutil.h"     

#ifdef JS_DHASHMETER
# if defined MOZILLA_CLIENT && defined DEBUG_XXXbrendan
#  include "nsTraceMalloc.h"
# endif
# define METER(x)       x
#else
# define METER(x)
#endif









#ifdef DEBUG

#define RECURSION_LEVEL(table_) (*(uint32*)(table_->entryStore + \
                                            JS_DHASH_TABLE_SIZE(table_) * \
                                            table_->entrySize))

#define ENTRY_STORE_EXTRA                   sizeof(uint32)
#define INCREMENT_RECURSION_LEVEL(table_)   (++RECURSION_LEVEL(table_))
#define DECREMENT_RECURSION_LEVEL(table_)   (--RECURSION_LEVEL(table_))

#else

#define ENTRY_STORE_EXTRA 0
#define INCREMENT_RECURSION_LEVEL(table_)   ((void)1)
#define DECREMENT_RECURSION_LEVEL(table_)   ((void)0)

#endif 

JS_PUBLIC_API(void *)
JS_DHashAllocTable(JSDHashTable *table, uint32 nbytes)
{
    return malloc(nbytes);
}

JS_PUBLIC_API(void)
JS_DHashFreeTable(JSDHashTable *table, void *ptr)
{
    free(ptr);
}

JS_PUBLIC_API(JSDHashNumber)
JS_DHashStringKey(JSDHashTable *table, const void *key)
{
    JSDHashNumber h;
    const unsigned char *s;

    h = 0;
    for (s = (const unsigned char *) key; *s != '\0'; s++)
        h = (h >> (JS_DHASH_BITS - 4)) ^ (h << 4) ^ *s;
    return h;
}

JS_PUBLIC_API(JSDHashNumber)
JS_DHashVoidPtrKeyStub(JSDHashTable *table, const void *key)
{
    return (JSDHashNumber)(unsigned long)key >> 2;
}

JS_PUBLIC_API(JSBool)
JS_DHashMatchEntryStub(JSDHashTable *table,
                       const JSDHashEntryHdr *entry,
                       const void *key)
{
    const JSDHashEntryStub *stub = (const JSDHashEntryStub *)entry;

    return stub->key == key;
}

JS_PUBLIC_API(JSBool)
JS_DHashMatchStringKey(JSDHashTable *table,
                       const JSDHashEntryHdr *entry,
                       const void *key)
{
    const JSDHashEntryStub *stub = (const JSDHashEntryStub *)entry;

    
    return stub->key == key ||
           (stub->key && key &&
            strcmp((const char *) stub->key, (const char *) key) == 0);
}

JS_PUBLIC_API(void)
JS_DHashMoveEntryStub(JSDHashTable *table,
                      const JSDHashEntryHdr *from,
                      JSDHashEntryHdr *to)
{
    memcpy(to, from, table->entrySize);
}

JS_PUBLIC_API(void)
JS_DHashClearEntryStub(JSDHashTable *table, JSDHashEntryHdr *entry)
{
    memset(entry, 0, table->entrySize);
}

JS_PUBLIC_API(void)
JS_DHashFreeStringKey(JSDHashTable *table, JSDHashEntryHdr *entry)
{
    const JSDHashEntryStub *stub = (const JSDHashEntryStub *)entry;

    free((void *) stub->key);
    memset(entry, 0, table->entrySize);
}

JS_PUBLIC_API(void)
JS_DHashFinalizeStub(JSDHashTable *table)
{
}

static const JSDHashTableOps stub_ops = {
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    JS_DHashVoidPtrKeyStub,
    JS_DHashMatchEntryStub,
    JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub,
    JS_DHashFinalizeStub,
    NULL
};

JS_PUBLIC_API(const JSDHashTableOps *)
JS_DHashGetStubOps(void)
{
    return &stub_ops;
}

JS_PUBLIC_API(JSDHashTable *)
JS_NewDHashTable(const JSDHashTableOps *ops, void *data, uint32 entrySize,
                 uint32 capacity)
{
    JSDHashTable *table;

    table = (JSDHashTable *) malloc(sizeof *table);
    if (!table)
        return NULL;
    if (!JS_DHashTableInit(table, ops, data, entrySize, capacity)) {
        free(table);
        return NULL;
    }
    return table;
}

JS_PUBLIC_API(void)
JS_DHashTableDestroy(JSDHashTable *table)
{
    JS_DHashTableFinish(table);
    free(table);
}

JS_PUBLIC_API(JSBool)
JS_DHashTableInit(JSDHashTable *table, const JSDHashTableOps *ops, void *data,
                  uint32 entrySize, uint32 capacity)
{
    int log2;
    uint32 nbytes;

#ifdef DEBUG
    if (entrySize > 10 * sizeof(void *)) {
        fprintf(stderr,
                "jsdhash: for the table at address %p, the given entrySize"
                " of %lu %s favors chaining over double hashing.\n",
                (void *) table,
                (unsigned long) entrySize,
                (entrySize > 16 * sizeof(void*)) ? "definitely" : "probably");
    }
#endif

    table->ops = ops;
    table->data = data;
    if (capacity < JS_DHASH_MIN_SIZE)
        capacity = JS_DHASH_MIN_SIZE;

    JS_CEILING_LOG2(log2, capacity);

    capacity = JS_BIT(log2);
    if (capacity >= JS_DHASH_SIZE_LIMIT)
        return JS_FALSE;
    table->hashShift = JS_DHASH_BITS - log2;
    table->maxAlphaFrac = (uint8)(0x100 * JS_DHASH_DEFAULT_MAX_ALPHA);
    table->minAlphaFrac = (uint8)(0x100 * JS_DHASH_DEFAULT_MIN_ALPHA);
    table->entrySize = entrySize;
    table->entryCount = table->removedCount = 0;
    table->generation = 0;
    nbytes = capacity * entrySize;

    table->entryStore = (char *) ops->allocTable(table,
                                                 nbytes + ENTRY_STORE_EXTRA);
    if (!table->entryStore)
        return JS_FALSE;
    memset(table->entryStore, 0, nbytes);
    METER(memset(&table->stats, 0, sizeof table->stats));

#ifdef DEBUG
    RECURSION_LEVEL(table) = 0;
#endif

    return JS_TRUE;
}




#define MAX_LOAD(table, size)   (((table)->maxAlphaFrac * (size)) >> 8)
#define MIN_LOAD(table, size)   (((table)->minAlphaFrac * (size)) >> 8)

JS_PUBLIC_API(void)
JS_DHashTableSetAlphaBounds(JSDHashTable *table,
                            float maxAlpha,
                            float minAlpha)
{
    uint32 size;

    



    JS_ASSERT(0.5 <= maxAlpha && maxAlpha < 1 && 0 <= minAlpha);
    if (maxAlpha < 0.5 || 1 <= maxAlpha || minAlpha < 0)
        return;

    




    JS_ASSERT(JS_DHASH_MIN_SIZE - (maxAlpha * JS_DHASH_MIN_SIZE) >= 1);
    if (JS_DHASH_MIN_SIZE - (maxAlpha * JS_DHASH_MIN_SIZE) < 1) {
        maxAlpha = (float)
                   (JS_DHASH_MIN_SIZE - JS_MAX(JS_DHASH_MIN_SIZE / 256, 1))
                   / JS_DHASH_MIN_SIZE;
    }

    




    JS_ASSERT(minAlpha < maxAlpha / 2);
    if (minAlpha >= maxAlpha / 2) {
        size = JS_DHASH_TABLE_SIZE(table);
        minAlpha = (size * maxAlpha - JS_MAX(size / 256, 1)) / (2 * size);
    }

    table->maxAlphaFrac = (uint8)(maxAlpha * 256);
    table->minAlphaFrac = (uint8)(minAlpha * 256);
}





#define HASH1(hash0, shift)         ((hash0) >> (shift))
#define HASH2(hash0,log2,shift)     ((((hash0) << (log2)) >> (shift)) | 1)













#define COLLISION_FLAG              ((JSDHashNumber) 1)
#define MARK_ENTRY_FREE(entry)      ((entry)->keyHash = 0)
#define MARK_ENTRY_REMOVED(entry)   ((entry)->keyHash = 1)
#define ENTRY_IS_REMOVED(entry)     ((entry)->keyHash == 1)
#define ENTRY_IS_LIVE(entry)        JS_DHASH_ENTRY_IS_LIVE(entry)
#define ENSURE_LIVE_KEYHASH(hash0)  if (hash0 < 2) hash0 -= 2; else (void)0


#define MATCH_ENTRY_KEYHASH(entry,hash0) \
    (((entry)->keyHash & ~COLLISION_FLAG) == (hash0))


#define ADDRESS_ENTRY(table, index) \
    ((JSDHashEntryHdr *)((table)->entryStore + (index) * (table)->entrySize))

JS_PUBLIC_API(void)
JS_DHashTableFinish(JSDHashTable *table)
{
    char *entryAddr, *entryLimit;
    uint32 entrySize;
    JSDHashEntryHdr *entry;

#ifdef DEBUG_XXXbrendan
    static FILE *dumpfp = NULL;
    if (!dumpfp) dumpfp = fopen("/tmp/jsdhash.bigdump", "w");
    if (dumpfp) {
#ifdef MOZILLA_CLIENT
        NS_TraceStack(1, dumpfp);
#endif
        JS_DHashTableDumpMeter(table, NULL, dumpfp);
        fputc('\n', dumpfp);
    }
#endif

    INCREMENT_RECURSION_LEVEL(table);

    
    table->ops->finalize(table);

    
    entryAddr = table->entryStore;
    entrySize = table->entrySize;
    entryLimit = entryAddr + JS_DHASH_TABLE_SIZE(table) * entrySize;
    while (entryAddr < entryLimit) {
        entry = (JSDHashEntryHdr *)entryAddr;
        if (ENTRY_IS_LIVE(entry)) {
            METER(table->stats.removeEnums++);
            table->ops->clearEntry(table, entry);
        }
        entryAddr += entrySize;
    }

    DECREMENT_RECURSION_LEVEL(table);
    JS_ASSERT(RECURSION_LEVEL(table) == 0);

    
    table->ops->freeTable(table, table->entryStore);
}

static JSDHashEntryHdr * JS_DHASH_FASTCALL
SearchTable(JSDHashTable *table, const void *key, JSDHashNumber keyHash,
            JSDHashOperator op)
{
    JSDHashNumber hash1, hash2;
    int hashShift, sizeLog2;
    JSDHashEntryHdr *entry, *firstRemoved;
    JSDHashMatchEntry matchEntry;
    uint32 sizeMask;

    METER(table->stats.searches++);
    JS_ASSERT(!(keyHash & COLLISION_FLAG));

    
    hashShift = table->hashShift;
    hash1 = HASH1(keyHash, hashShift);
    entry = ADDRESS_ENTRY(table, hash1);

    
    if (JS_DHASH_ENTRY_IS_FREE(entry)) {
        METER(table->stats.misses++);
        return entry;
    }

    
    matchEntry = table->ops->matchEntry;
    if (MATCH_ENTRY_KEYHASH(entry, keyHash) && matchEntry(table, entry, key)) {
        METER(table->stats.hits++);
        return entry;
    }

    
    sizeLog2 = JS_DHASH_BITS - table->hashShift;
    hash2 = HASH2(keyHash, sizeLog2, hashShift);
    sizeMask = JS_BITMASK(sizeLog2);

    
    firstRemoved = NULL;

    for (;;) {
        if (JS_UNLIKELY(ENTRY_IS_REMOVED(entry))) {
            if (!firstRemoved)
                firstRemoved = entry;
        } else {
            if (op == JS_DHASH_ADD)
                entry->keyHash |= COLLISION_FLAG;
        }

        METER(table->stats.steps++);
        hash1 -= hash2;
        hash1 &= sizeMask;

        entry = ADDRESS_ENTRY(table, hash1);
        if (JS_DHASH_ENTRY_IS_FREE(entry)) {
            METER(table->stats.misses++);
            return (firstRemoved && op == JS_DHASH_ADD) ? firstRemoved : entry;
        }

        if (MATCH_ENTRY_KEYHASH(entry, keyHash) &&
            matchEntry(table, entry, key)) {
            METER(table->stats.hits++);
            return entry;
        }
    }

    
    return NULL;
}











static JSDHashEntryHdr * JS_DHASH_FASTCALL
FindFreeEntry(JSDHashTable *table, JSDHashNumber keyHash)
{
    JSDHashNumber hash1, hash2;
    int hashShift, sizeLog2;
    JSDHashEntryHdr *entry;
    uint32 sizeMask;

    METER(table->stats.searches++);
    JS_ASSERT(!(keyHash & COLLISION_FLAG));

    
    hashShift = table->hashShift;
    hash1 = HASH1(keyHash, hashShift);
    entry = ADDRESS_ENTRY(table, hash1);

    
    if (JS_DHASH_ENTRY_IS_FREE(entry)) {
        METER(table->stats.misses++);
        return entry;
    }

    
    sizeLog2 = JS_DHASH_BITS - table->hashShift;
    hash2 = HASH2(keyHash, sizeLog2, hashShift);
    sizeMask = JS_BITMASK(sizeLog2);

    for (;;) {
        JS_ASSERT(!ENTRY_IS_REMOVED(entry));
        entry->keyHash |= COLLISION_FLAG;

        METER(table->stats.steps++);
        hash1 -= hash2;
        hash1 &= sizeMask;

        entry = ADDRESS_ENTRY(table, hash1);
        if (JS_DHASH_ENTRY_IS_FREE(entry)) {
            METER(table->stats.misses++);
            return entry;
        }
    }

    
    return NULL;
}

static JSBool
ChangeTable(JSDHashTable *table, int deltaLog2)
{
    int oldLog2, newLog2;
    uint32 oldCapacity, newCapacity;
    char *newEntryStore, *oldEntryStore, *oldEntryAddr;
    uint32 entrySize, i, nbytes;
    JSDHashEntryHdr *oldEntry, *newEntry;
    JSDHashMoveEntry moveEntry;
#ifdef DEBUG
    uint32 recursionLevel;
#endif

    
    oldLog2 = JS_DHASH_BITS - table->hashShift;
    newLog2 = oldLog2 + deltaLog2;
    oldCapacity = JS_BIT(oldLog2);
    newCapacity = JS_BIT(newLog2);
    if (newCapacity >= JS_DHASH_SIZE_LIMIT)
        return JS_FALSE;
    entrySize = table->entrySize;
    nbytes = newCapacity * entrySize;

    newEntryStore = (char *) table->ops->allocTable(table,
                                                    nbytes + ENTRY_STORE_EXTRA);
    if (!newEntryStore)
        return JS_FALSE;

    
#ifdef DEBUG
    recursionLevel = RECURSION_LEVEL(table);
#endif
    table->hashShift = JS_DHASH_BITS - newLog2;
    table->removedCount = 0;
    table->generation++;

    
    memset(newEntryStore, 0, nbytes);
    oldEntryAddr = oldEntryStore = table->entryStore;
    table->entryStore = newEntryStore;
    moveEntry = table->ops->moveEntry;
#ifdef DEBUG
    RECURSION_LEVEL(table) = recursionLevel;
#endif

    
    for (i = 0; i < oldCapacity; i++) {
        oldEntry = (JSDHashEntryHdr *)oldEntryAddr;
        if (ENTRY_IS_LIVE(oldEntry)) {
            oldEntry->keyHash &= ~COLLISION_FLAG;
            newEntry = FindFreeEntry(table, oldEntry->keyHash);
            JS_ASSERT(JS_DHASH_ENTRY_IS_FREE(newEntry));
            moveEntry(table, oldEntry, newEntry);
            newEntry->keyHash = oldEntry->keyHash;
        }
        oldEntryAddr += entrySize;
    }

    table->ops->freeTable(table, oldEntryStore);
    return JS_TRUE;
}

JS_PUBLIC_API(JSDHashEntryHdr *) JS_DHASH_FASTCALL
JS_DHashTableOperate(JSDHashTable *table, const void *key, JSDHashOperator op)
{
    JSDHashNumber keyHash;
    JSDHashEntryHdr *entry;
    uint32 size;
    int deltaLog2;

    JS_ASSERT(op == JS_DHASH_LOOKUP || RECURSION_LEVEL(table) == 0);
    INCREMENT_RECURSION_LEVEL(table);

    keyHash = table->ops->hashKey(table, key);
    keyHash *= JS_DHASH_GOLDEN_RATIO;

    
    ENSURE_LIVE_KEYHASH(keyHash);
    keyHash &= ~COLLISION_FLAG;

    switch (op) {
      case JS_DHASH_LOOKUP:
        METER(table->stats.lookups++);
        entry = SearchTable(table, key, keyHash, op);
        break;

      case JS_DHASH_ADD:
        




        size = JS_DHASH_TABLE_SIZE(table);
        if (table->entryCount + table->removedCount >= MAX_LOAD(table, size)) {
            
            if (table->removedCount >= size >> 2) {
                METER(table->stats.compresses++);
                deltaLog2 = 0;
            } else {
                METER(table->stats.grows++);
                deltaLog2 = 1;
            }

            



            if (!ChangeTable(table, deltaLog2) &&
                table->entryCount + table->removedCount == size - 1) {
                METER(table->stats.addFailures++);
                entry = NULL;
                break;
            }
        }

        



        entry = SearchTable(table, key, keyHash, op);
        if (!ENTRY_IS_LIVE(entry)) {
            
            METER(table->stats.addMisses++);
            if (ENTRY_IS_REMOVED(entry)) {
                METER(table->stats.addOverRemoved++);
                table->removedCount--;
                keyHash |= COLLISION_FLAG;
            }
            if (table->ops->initEntry &&
                !table->ops->initEntry(table, entry, key)) {
                
                memset(entry + 1, 0, table->entrySize - sizeof *entry);
                entry = NULL;
                break;
            }
            entry->keyHash = keyHash;
            table->entryCount++;
        }
        METER(else table->stats.addHits++);
        break;

      case JS_DHASH_REMOVE:
        entry = SearchTable(table, key, keyHash, op);
        if (ENTRY_IS_LIVE(entry)) {
            
            METER(table->stats.removeHits++);
            JS_DHashTableRawRemove(table, entry);

            
            size = JS_DHASH_TABLE_SIZE(table);
            if (size > JS_DHASH_MIN_SIZE &&
                table->entryCount <= MIN_LOAD(table, size)) {
                METER(table->stats.shrinks++);
                (void) ChangeTable(table, -1);
            }
        }
        METER(else table->stats.removeMisses++);
        entry = NULL;
        break;

      default:
        JS_ASSERT(0);
        entry = NULL;
    }

    DECREMENT_RECURSION_LEVEL(table);

    return entry;
}

JS_PUBLIC_API(void)
JS_DHashTableRawRemove(JSDHashTable *table, JSDHashEntryHdr *entry)
{
    JSDHashNumber keyHash;      

    JS_ASSERT(JS_DHASH_ENTRY_IS_LIVE(entry));
    keyHash = entry->keyHash;
    table->ops->clearEntry(table, entry);
    if (keyHash & COLLISION_FLAG) {
        MARK_ENTRY_REMOVED(entry);
        table->removedCount++;
    } else {
        METER(table->stats.removeFrees++);
        MARK_ENTRY_FREE(entry);
    }
    table->entryCount--;
}

JS_PUBLIC_API(uint32)
JS_DHashTableEnumerate(JSDHashTable *table, JSDHashEnumerator etor, void *arg)
{
    char *entryAddr, *entryLimit;
    uint32 i, capacity, entrySize, ceiling;
    JSBool didRemove;
    JSDHashEntryHdr *entry;
    JSDHashOperator op;

    INCREMENT_RECURSION_LEVEL(table);

    entryAddr = table->entryStore;
    entrySize = table->entrySize;
    capacity = JS_DHASH_TABLE_SIZE(table);
    entryLimit = entryAddr + capacity * entrySize;
    i = 0;
    didRemove = JS_FALSE;
    while (entryAddr < entryLimit) {
        entry = (JSDHashEntryHdr *)entryAddr;
        if (ENTRY_IS_LIVE(entry)) {
            op = etor(table, entry, i++, arg);
            if (op & JS_DHASH_REMOVE) {
                METER(table->stats.removeEnums++);
                JS_DHashTableRawRemove(table, entry);
                didRemove = JS_TRUE;
            }
            if (op & JS_DHASH_STOP)
                break;
        }
        entryAddr += entrySize;
    }

    JS_ASSERT(!didRemove || RECURSION_LEVEL(table) == 1);

    






    if (didRemove &&
        (table->removedCount >= capacity >> 2 ||
         (capacity > JS_DHASH_MIN_SIZE &&
          table->entryCount <= MIN_LOAD(table, capacity)))) {
        METER(table->stats.enumShrinks++);
        capacity = table->entryCount;
        capacity += capacity >> 1;
        if (capacity < JS_DHASH_MIN_SIZE)
            capacity = JS_DHASH_MIN_SIZE;

        JS_CEILING_LOG2(ceiling, capacity);
        ceiling -= JS_DHASH_BITS - table->hashShift;

        (void) ChangeTable(table, ceiling);
    }

    DECREMENT_RECURSION_LEVEL(table);

    return i;
}

#ifdef JS_DHASHMETER
#include <math.h>

JS_PUBLIC_API(void)
JS_DHashTableDumpMeter(JSDHashTable *table, JSDHashEnumerator dump, FILE *fp)
{
    char *entryAddr;
    uint32 entrySize, entryCount;
    int hashShift, sizeLog2;
    uint32 i, tableSize, sizeMask, chainLen, maxChainLen, chainCount;
    JSDHashNumber hash1, hash2, saveHash1, maxChainHash1, maxChainHash2;
    double sqsum, mean, variance, sigma;
    JSDHashEntryHdr *entry, *probe;

    entryAddr = table->entryStore;
    entrySize = table->entrySize;
    hashShift = table->hashShift;
    sizeLog2 = JS_DHASH_BITS - hashShift;
    tableSize = JS_DHASH_TABLE_SIZE(table);
    sizeMask = JS_BITMASK(sizeLog2);
    chainCount = maxChainLen = 0;
    hash2 = 0;
    sqsum = 0;

    for (i = 0; i < tableSize; i++) {
        entry = (JSDHashEntryHdr *)entryAddr;
        entryAddr += entrySize;
        if (!ENTRY_IS_LIVE(entry))
            continue;
        hash1 = HASH1(entry->keyHash & ~COLLISION_FLAG, hashShift);
        saveHash1 = hash1;
        probe = ADDRESS_ENTRY(table, hash1);
        chainLen = 1;
        if (probe == entry) {
            
            chainCount++;
        } else {
            hash2 = HASH2(entry->keyHash & ~COLLISION_FLAG, sizeLog2,
                          hashShift);
            do {
                chainLen++;
                hash1 -= hash2;
                hash1 &= sizeMask;
                probe = ADDRESS_ENTRY(table, hash1);
            } while (probe != entry);
        }
        sqsum += chainLen * chainLen;
        if (chainLen > maxChainLen) {
            maxChainLen = chainLen;
            maxChainHash1 = saveHash1;
            maxChainHash2 = hash2;
        }
    }

    entryCount = table->entryCount;
    if (entryCount && chainCount) {
        mean = (double)entryCount / chainCount;
        variance = chainCount * sqsum - entryCount * entryCount;
        if (variance < 0 || chainCount == 1)
            variance = 0;
        else
            variance /= chainCount * (chainCount - 1);
        sigma = sqrt(variance);
    } else {
        mean = sigma = 0;
    }

    fprintf(fp, "Double hashing statistics:\n");
    fprintf(fp, "    table size (in entries): %u\n", tableSize);
    fprintf(fp, "          number of entries: %u\n", table->entryCount);
    fprintf(fp, "  number of removed entries: %u\n", table->removedCount);
    fprintf(fp, "         number of searches: %u\n", table->stats.searches);
    fprintf(fp, "             number of hits: %u\n", table->stats.hits);
    fprintf(fp, "           number of misses: %u\n", table->stats.misses);
    fprintf(fp, "      mean steps per search: %g\n", table->stats.searches ?
                                                     (double)table->stats.steps
                                                     / table->stats.searches :
                                                     0.);
    fprintf(fp, "     mean hash chain length: %g\n", mean);
    fprintf(fp, "         standard deviation: %g\n", sigma);
    fprintf(fp, "  maximum hash chain length: %u\n", maxChainLen);
    fprintf(fp, "          number of lookups: %u\n", table->stats.lookups);
    fprintf(fp, " adds that made a new entry: %u\n", table->stats.addMisses);
    fprintf(fp, "adds that recycled removeds: %u\n", table->stats.addOverRemoved);
    fprintf(fp, "   adds that found an entry: %u\n", table->stats.addHits);
    fprintf(fp, "               add failures: %u\n", table->stats.addFailures);
    fprintf(fp, "             useful removes: %u\n", table->stats.removeHits);
    fprintf(fp, "            useless removes: %u\n", table->stats.removeMisses);
    fprintf(fp, "removes that freed an entry: %u\n", table->stats.removeFrees);
    fprintf(fp, "  removes while enumerating: %u\n", table->stats.removeEnums);
    fprintf(fp, "            number of grows: %u\n", table->stats.grows);
    fprintf(fp, "          number of shrinks: %u\n", table->stats.shrinks);
    fprintf(fp, "       number of compresses: %u\n", table->stats.compresses);
    fprintf(fp, "number of enumerate shrinks: %u\n", table->stats.enumShrinks);

    if (dump && maxChainLen && hash2) {
        fputs("Maximum hash chain:\n", fp);
        hash1 = maxChainHash1;
        hash2 = maxChainHash2;
        entry = ADDRESS_ENTRY(table, hash1);
        i = 0;
        do {
            if (dump(table, entry, i++, fp) != JS_DHASH_NEXT)
                break;
            hash1 -= hash2;
            hash1 &= sizeMask;
            entry = ADDRESS_ENTRY(table, hash1);
        } while (JS_DHASH_ENTRY_IS_BUSY(entry));
    }
}
#endif 
