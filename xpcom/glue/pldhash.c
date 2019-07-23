











































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prbit.h"
#include "pldhash.h"
#include "nsDebug.h"     

#ifdef PL_DHASHMETER
# if defined MOZILLA_CLIENT && defined DEBUG_XXXbrendan
#  include "nsTraceMalloc.h"
# endif
# define METER(x)       x
#else
# define METER(x)
#endif









#ifdef DEBUG

#define RECURSION_LEVEL(table_) (*(PRUint32*)(table_->entryStore + \
                                            PL_DHASH_TABLE_SIZE(table_) * \
                                            table_->entrySize))

#define ENTRY_STORE_EXTRA                   sizeof(PRUint32)
#define INCREMENT_RECURSION_LEVEL(table_)   (++RECURSION_LEVEL(table_))
#define DECREMENT_RECURSION_LEVEL(table_)   (--RECURSION_LEVEL(table_))

#else

#define ENTRY_STORE_EXTRA 0
#define INCREMENT_RECURSION_LEVEL(table_)   ((void)1)
#define DECREMENT_RECURSION_LEVEL(table_)   ((void)0)

#endif 

void *
PL_DHashAllocTable(PLDHashTable *table, PRUint32 nbytes)
{
    return malloc(nbytes);
}

void
PL_DHashFreeTable(PLDHashTable *table, void *ptr)
{
    free(ptr);
}

PLDHashNumber
PL_DHashStringKey(PLDHashTable *table, const void *key)
{
    PLDHashNumber h;
    const unsigned char *s;

    h = 0;
    for (s = key; *s != '\0'; s++)
        h = (h >> (PL_DHASH_BITS - 4)) ^ (h << 4) ^ *s;
    return h;
}

const void *
PL_DHashGetKeyStub(PLDHashTable *table, PLDHashEntryHdr *entry)
{
    PLDHashEntryStub *stub = (PLDHashEntryStub *)entry;

    return stub->key;
}

PLDHashNumber
PL_DHashVoidPtrKeyStub(PLDHashTable *table, const void *key)
{
    return (PLDHashNumber)(unsigned long)key >> 2;
}

PRBool
PL_DHashMatchEntryStub(PLDHashTable *table,
                       const PLDHashEntryHdr *entry,
                       const void *key)
{
    const PLDHashEntryStub *stub = (const PLDHashEntryStub *)entry;

    return stub->key == key;
}

PRBool
PL_DHashMatchStringKey(PLDHashTable *table,
                       const PLDHashEntryHdr *entry,
                       const void *key)
{
    const PLDHashEntryStub *stub = (const PLDHashEntryStub *)entry;

    
    return stub->key == key ||
           (stub->key && key && strcmp(stub->key, key) == 0);
}

void
PL_DHashMoveEntryStub(PLDHashTable *table,
                      const PLDHashEntryHdr *from,
                      PLDHashEntryHdr *to)
{
    memcpy(to, from, table->entrySize);
}

void
PL_DHashClearEntryStub(PLDHashTable *table, PLDHashEntryHdr *entry)
{
    memset(entry, 0, table->entrySize);
}

void
PL_DHashFreeStringKey(PLDHashTable *table, PLDHashEntryHdr *entry)
{
    const PLDHashEntryStub *stub = (const PLDHashEntryStub *)entry;

    free((void *) stub->key);
    memset(entry, 0, table->entrySize);
}

void
PL_DHashFinalizeStub(PLDHashTable *table)
{
}

static const PLDHashTableOps stub_ops = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    PL_DHashGetKeyStub,
    PL_DHashVoidPtrKeyStub,
    PL_DHashMatchEntryStub,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    NULL
};

const PLDHashTableOps *
PL_DHashGetStubOps(void)
{
    return &stub_ops;
}

PLDHashTable *
PL_NewDHashTable(const PLDHashTableOps *ops, void *data, PRUint32 entrySize,
                 PRUint32 capacity)
{
    PLDHashTable *table;

    table = (PLDHashTable *) malloc(sizeof *table);
    if (!table)
        return NULL;
    if (!PL_DHashTableInit(table, ops, data, entrySize, capacity)) {
        free(table);
        return NULL;
    }
    return table;
}

void
PL_DHashTableDestroy(PLDHashTable *table)
{
    PL_DHashTableFinish(table);
    free(table);
}

PRBool
PL_DHashTableInit(PLDHashTable *table, const PLDHashTableOps *ops, void *data,
                  PRUint32 entrySize, PRUint32 capacity)
{
    int log2;
    PRUint32 nbytes;

#ifdef DEBUG
    if (entrySize > 10 * sizeof(void *)) {
        printf_stderr(
                "pldhash: for the table at address %p, the given entrySize"
                " of %lu %s favors chaining over double hashing.\n",
                (void *)table,
                (unsigned long) entrySize,
                (entrySize > 16 * sizeof(void*)) ? "definitely" : "probably");
    }
#endif

    table->ops = ops;
    table->data = data;
    if (capacity < PL_DHASH_MIN_SIZE)
        capacity = PL_DHASH_MIN_SIZE;

    PR_CEILING_LOG2(log2, capacity);

    capacity = PR_BIT(log2);
    if (capacity >= PL_DHASH_SIZE_LIMIT)
        return PR_FALSE;
    table->hashShift = PL_DHASH_BITS - log2;
    table->maxAlphaFrac = (uint8)(0x100 * PL_DHASH_DEFAULT_MAX_ALPHA);
    table->minAlphaFrac = (uint8)(0x100 * PL_DHASH_DEFAULT_MIN_ALPHA);
    table->entrySize = entrySize;
    table->entryCount = table->removedCount = 0;
    table->generation = 0;
    nbytes = capacity * entrySize;

    table->entryStore = ops->allocTable(table, nbytes + ENTRY_STORE_EXTRA);
    if (!table->entryStore)
        return PR_FALSE;
    memset(table->entryStore, 0, nbytes);
    METER(memset(&table->stats, 0, sizeof table->stats));

#ifdef DEBUG
    RECURSION_LEVEL(table) = 0;
#endif

    return PR_TRUE;
}




#define MAX_LOAD(table, size)   (((table)->maxAlphaFrac * (size)) >> 8)
#define MIN_LOAD(table, size)   (((table)->minAlphaFrac * (size)) >> 8)

void
PL_DHashTableSetAlphaBounds(PLDHashTable *table,
                            float maxAlpha,
                            float minAlpha)
{
    PRUint32 size;

    



    NS_ASSERTION(0.5 <= maxAlpha && maxAlpha < 1 && 0 <= minAlpha,
                 "0.5 <= maxAlpha && maxAlpha < 1 && 0 <= minAlpha");
    if (maxAlpha < 0.5 || 1 <= maxAlpha || minAlpha < 0)
        return;

    




    NS_ASSERTION(PL_DHASH_MIN_SIZE - (maxAlpha * PL_DHASH_MIN_SIZE) >= 1,
                 "PL_DHASH_MIN_SIZE - (maxAlpha * PL_DHASH_MIN_SIZE) >= 1");
    if (PL_DHASH_MIN_SIZE - (maxAlpha * PL_DHASH_MIN_SIZE) < 1) {
        maxAlpha = (float)
                   (PL_DHASH_MIN_SIZE - PR_MAX(PL_DHASH_MIN_SIZE / 256, 1))
                   / PL_DHASH_MIN_SIZE;
    }

    




    NS_ASSERTION(minAlpha < maxAlpha / 2,
                 "minAlpha < maxAlpha / 2");
    if (minAlpha >= maxAlpha / 2) {
        size = PL_DHASH_TABLE_SIZE(table);
        minAlpha = (size * maxAlpha - PR_MAX(size / 256, 1)) / (2 * size);
    }

    table->maxAlphaFrac = (uint8)(maxAlpha * 256);
    table->minAlphaFrac = (uint8)(minAlpha * 256);
}





#define HASH1(hash0, shift)         ((hash0) >> (shift))
#define HASH2(hash0,log2,shift)     ((((hash0) << (log2)) >> (shift)) | 1)













#define COLLISION_FLAG              ((PLDHashNumber) 1)
#define MARK_ENTRY_FREE(entry)      ((entry)->keyHash = 0)
#define MARK_ENTRY_REMOVED(entry)   ((entry)->keyHash = 1)
#define ENTRY_IS_REMOVED(entry)     ((entry)->keyHash == 1)
#define ENTRY_IS_LIVE(entry)        PL_DHASH_ENTRY_IS_LIVE(entry)
#define ENSURE_LIVE_KEYHASH(hash0)  if (hash0 < 2) hash0 -= 2; else (void)0


#define MATCH_ENTRY_KEYHASH(entry,hash0) \
    (((entry)->keyHash & ~COLLISION_FLAG) == (hash0))


#define ADDRESS_ENTRY(table, index) \
    ((PLDHashEntryHdr *)((table)->entryStore + (index) * (table)->entrySize))

void
PL_DHashTableFinish(PLDHashTable *table)
{
    char *entryAddr, *entryLimit;
    PRUint32 entrySize;
    PLDHashEntryHdr *entry;

#ifdef DEBUG_XXXbrendan
    static FILE *dumpfp = NULL;
    if (!dumpfp) dumpfp = fopen("/tmp/pldhash.bigdump", "w");
    if (dumpfp) {
#ifdef MOZILLA_CLIENT
        NS_TraceStack(1, dumpfp);
#endif
        PL_DHashTableDumpMeter(table, NULL, dumpfp);
        fputc('\n', dumpfp);
    }
#endif

    INCREMENT_RECURSION_LEVEL(table);

    
    table->ops->finalize(table);

    
    entryAddr = table->entryStore;
    entrySize = table->entrySize;
    entryLimit = entryAddr + PL_DHASH_TABLE_SIZE(table) * entrySize;
    while (entryAddr < entryLimit) {
        entry = (PLDHashEntryHdr *)entryAddr;
        if (ENTRY_IS_LIVE(entry)) {
            METER(table->stats.removeEnums++);
            table->ops->clearEntry(table, entry);
        }
        entryAddr += entrySize;
    }

    DECREMENT_RECURSION_LEVEL(table);
    NS_ASSERTION(RECURSION_LEVEL(table) == 0,
                 "RECURSION_LEVEL(table) == 0");

    
    table->ops->freeTable(table, table->entryStore);
}

static PLDHashEntryHdr * PL_DHASH_FASTCALL
SearchTable(PLDHashTable *table, const void *key, PLDHashNumber keyHash,
            PLDHashOperator op)
{
    PLDHashNumber hash1, hash2;
    int hashShift, sizeLog2;
    PLDHashEntryHdr *entry, *firstRemoved;
    PLDHashMatchEntry matchEntry;
    PRUint32 sizeMask;

    METER(table->stats.searches++);
    NS_ASSERTION(!(keyHash & COLLISION_FLAG),
                 "!(keyHash & COLLISION_FLAG)");

    
    hashShift = table->hashShift;
    hash1 = HASH1(keyHash, hashShift);
    entry = ADDRESS_ENTRY(table, hash1);

    
    if (PL_DHASH_ENTRY_IS_FREE(entry)) {
        METER(table->stats.misses++);
        return entry;
    }

    
    matchEntry = table->ops->matchEntry;
    if (MATCH_ENTRY_KEYHASH(entry, keyHash) && matchEntry(table, entry, key)) {
        METER(table->stats.hits++);
        return entry;
    }

    
    sizeLog2 = PL_DHASH_BITS - table->hashShift;
    hash2 = HASH2(keyHash, sizeLog2, hashShift);
    sizeMask = PR_BITMASK(sizeLog2);

    
    if (ENTRY_IS_REMOVED(entry)) {
        firstRemoved = entry;
    } else {
        firstRemoved = NULL;
        if (op == PL_DHASH_ADD)
            entry->keyHash |= COLLISION_FLAG;
    }

    for (;;) {
        METER(table->stats.steps++);
        hash1 -= hash2;
        hash1 &= sizeMask;

        entry = ADDRESS_ENTRY(table, hash1);
        if (PL_DHASH_ENTRY_IS_FREE(entry)) {
            METER(table->stats.misses++);
            return (firstRemoved && op == PL_DHASH_ADD) ? firstRemoved : entry;
        }

        if (MATCH_ENTRY_KEYHASH(entry, keyHash) &&
            matchEntry(table, entry, key)) {
            METER(table->stats.hits++);
            return entry;
        }

        if (ENTRY_IS_REMOVED(entry)) {
            if (!firstRemoved)
                firstRemoved = entry;
        } else {
            if (op == PL_DHASH_ADD)
                entry->keyHash |= COLLISION_FLAG;
        }
    }

    
    return NULL;
}











static PLDHashEntryHdr * PL_DHASH_FASTCALL
FindFreeEntry(PLDHashTable *table, PLDHashNumber keyHash)
{
    PLDHashNumber hash1, hash2;
    int hashShift, sizeLog2;
    PLDHashEntryHdr *entry;
    PRUint32 sizeMask;

    METER(table->stats.searches++);
    NS_ASSERTION(!(keyHash & COLLISION_FLAG),
                 "!(keyHash & COLLISION_FLAG)");

    
    hashShift = table->hashShift;
    hash1 = HASH1(keyHash, hashShift);
    entry = ADDRESS_ENTRY(table, hash1);

    
    if (PL_DHASH_ENTRY_IS_FREE(entry)) {
        METER(table->stats.misses++);
        return entry;
    }

    
    sizeLog2 = PL_DHASH_BITS - table->hashShift;
    hash2 = HASH2(keyHash, sizeLog2, hashShift);
    sizeMask = PR_BITMASK(sizeLog2);

    NS_ASSERTION(!ENTRY_IS_REMOVED(entry),
                 "!ENTRY_IS_REMOVED(entry)");
    entry->keyHash |= COLLISION_FLAG;

    for (;;) {
        METER(table->stats.steps++);
        hash1 -= hash2;
        hash1 &= sizeMask;

        entry = ADDRESS_ENTRY(table, hash1);
        if (PL_DHASH_ENTRY_IS_FREE(entry)) {
            METER(table->stats.misses++);
            return entry;
        }

        NS_ASSERTION(!ENTRY_IS_REMOVED(entry),
                     "!ENTRY_IS_REMOVED(entry)");
        entry->keyHash |= COLLISION_FLAG;
    }

    
    return NULL;
}

static PRBool
ChangeTable(PLDHashTable *table, int deltaLog2)
{
    int oldLog2, newLog2;
    PRUint32 oldCapacity, newCapacity;
    char *newEntryStore, *oldEntryStore, *oldEntryAddr;
    PRUint32 entrySize, i, nbytes;
    PLDHashEntryHdr *oldEntry, *newEntry;
    PLDHashMoveEntry moveEntry;
#ifdef DEBUG
    PRUint32 recursionLevel;
#endif

    
    oldLog2 = PL_DHASH_BITS - table->hashShift;
    newLog2 = oldLog2 + deltaLog2;
    oldCapacity = PR_BIT(oldLog2);
    newCapacity = PR_BIT(newLog2);
    if (newCapacity >= PL_DHASH_SIZE_LIMIT)
        return PR_FALSE;
    entrySize = table->entrySize;
    nbytes = newCapacity * entrySize;

    newEntryStore = table->ops->allocTable(table, nbytes + ENTRY_STORE_EXTRA);
    if (!newEntryStore)
        return PR_FALSE;

    
#ifdef DEBUG
    recursionLevel = RECURSION_LEVEL(table);
#endif
    table->hashShift = PL_DHASH_BITS - newLog2;
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
        oldEntry = (PLDHashEntryHdr *)oldEntryAddr;
        if (ENTRY_IS_LIVE(oldEntry)) {
            oldEntry->keyHash &= ~COLLISION_FLAG;
            newEntry = FindFreeEntry(table, oldEntry->keyHash);
            NS_ASSERTION(PL_DHASH_ENTRY_IS_FREE(newEntry),
                         "PL_DHASH_ENTRY_IS_FREE(newEntry)");
            moveEntry(table, oldEntry, newEntry);
            newEntry->keyHash = oldEntry->keyHash;
        }
        oldEntryAddr += entrySize;
    }

    table->ops->freeTable(table, oldEntryStore);
    return PR_TRUE;
}

PLDHashEntryHdr * PL_DHASH_FASTCALL
PL_DHashTableOperate(PLDHashTable *table, const void *key, PLDHashOperator op)
{
    PLDHashNumber keyHash;
    PLDHashEntryHdr *entry;
    PRUint32 size;
    int deltaLog2;

    NS_ASSERTION(op == PL_DHASH_LOOKUP || RECURSION_LEVEL(table) == 0,
                 "op == PL_DHASH_LOOKUP || RECURSION_LEVEL(table) == 0");
    INCREMENT_RECURSION_LEVEL(table);

    keyHash = table->ops->hashKey(table, key);
    keyHash *= PL_DHASH_GOLDEN_RATIO;

    
    ENSURE_LIVE_KEYHASH(keyHash);
    keyHash &= ~COLLISION_FLAG;

    switch (op) {
      case PL_DHASH_LOOKUP:
        METER(table->stats.lookups++);
        entry = SearchTable(table, key, keyHash, op);
        break;

      case PL_DHASH_ADD:
        




        size = PL_DHASH_TABLE_SIZE(table);
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

      case PL_DHASH_REMOVE:
        entry = SearchTable(table, key, keyHash, op);
        if (ENTRY_IS_LIVE(entry)) {
            
            METER(table->stats.removeHits++);
            PL_DHashTableRawRemove(table, entry);

            
            size = PL_DHASH_TABLE_SIZE(table);
            if (size > PL_DHASH_MIN_SIZE &&
                table->entryCount <= MIN_LOAD(table, size)) {
                METER(table->stats.shrinks++);
                (void) ChangeTable(table, -1);
            }
        }
        METER(else table->stats.removeMisses++);
        entry = NULL;
        break;

      default:
        NS_NOTREACHED("0");
        entry = NULL;
    }

    DECREMENT_RECURSION_LEVEL(table);

    return entry;
}

void
PL_DHashTableRawRemove(PLDHashTable *table, PLDHashEntryHdr *entry)
{
    PLDHashNumber keyHash;      

    NS_ASSERTION(PL_DHASH_ENTRY_IS_LIVE(entry),
                 "PL_DHASH_ENTRY_IS_LIVE(entry)");
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

PRUint32
PL_DHashTableEnumerate(PLDHashTable *table, PLDHashEnumerator etor, void *arg)
{
    char *entryAddr, *entryLimit;
    PRUint32 i, capacity, entrySize, ceiling;
    PRBool didRemove;
    PLDHashEntryHdr *entry;
    PLDHashOperator op;

    INCREMENT_RECURSION_LEVEL(table);

    entryAddr = table->entryStore;
    entrySize = table->entrySize;
    capacity = PL_DHASH_TABLE_SIZE(table);
    entryLimit = entryAddr + capacity * entrySize;
    i = 0;
    didRemove = PR_FALSE;
    while (entryAddr < entryLimit) {
        entry = (PLDHashEntryHdr *)entryAddr;
        if (ENTRY_IS_LIVE(entry)) {
            op = etor(table, entry, i++, arg);
            if (op & PL_DHASH_REMOVE) {
                METER(table->stats.removeEnums++);
                PL_DHashTableRawRemove(table, entry);
                didRemove = PR_TRUE;
            }
            if (op & PL_DHASH_STOP)
                break;
        }
        entryAddr += entrySize;
    }

    NS_ASSERTION(!didRemove || RECURSION_LEVEL(table) == 1,
                 "!didRemove || RECURSION_LEVEL(table) == 1");

    






    if (didRemove &&
        (table->removedCount >= capacity >> 2 ||
         (capacity > PL_DHASH_MIN_SIZE &&
          table->entryCount <= MIN_LOAD(table, capacity)))) {
        METER(table->stats.enumShrinks++);
        capacity = table->entryCount;
        capacity += capacity >> 1;
        if (capacity < PL_DHASH_MIN_SIZE)
            capacity = PL_DHASH_MIN_SIZE;

        PR_CEILING_LOG2(ceiling, capacity);
        ceiling -= PL_DHASH_BITS - table->hashShift;

        (void) ChangeTable(table, ceiling);
    }

    DECREMENT_RECURSION_LEVEL(table);

    return i;
}

#ifdef PL_DHASHMETER
#include <math.h>

void
PL_DHashTableDumpMeter(PLDHashTable *table, PLDHashEnumerator dump, FILE *fp)
{
    char *entryAddr;
    PRUint32 entrySize, entryCount;
    int hashShift, sizeLog2;
    PRUint32 i, tableSize, sizeMask, chainLen, maxChainLen, chainCount;
    PLDHashNumber hash1, hash2, saveHash1, maxChainHash1, maxChainHash2;
    double sqsum, mean, variance, sigma;
    PLDHashEntryHdr *entry, *probe;

    entryAddr = table->entryStore;
    entrySize = table->entrySize;
    hashShift = table->hashShift;
    sizeLog2 = PL_DHASH_BITS - hashShift;
    tableSize = PL_DHASH_TABLE_SIZE(table);
    sizeMask = PR_BITMASK(sizeLog2);
    chainCount = maxChainLen = 0;
    hash2 = 0;
    sqsum = 0;

    for (i = 0; i < tableSize; i++) {
        entry = (PLDHashEntryHdr *)entryAddr;
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
            if (dump(table, entry, i++, fp) != PL_DHASH_NEXT)
                break;
            hash1 -= hash2;
            hash1 &= sizeMask;
            entry = ADDRESS_ENTRY(table, hash1);
        } while (PL_DHASH_ENTRY_IS_BUSY(entry));
    }
}
#endif 
