







#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pldhash.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/MathAlgorithms.h"
#include "nsDebug.h"     
#include "nsAlgorithm.h"
#include "mozilla/Likely.h"
#include "mozilla/MemoryReporting.h"

#ifdef PL_DHASHMETER
# define METER(x)       x
#else
# define METER(x)
#endif






#ifdef DEBUG









#define IMMUTABLE_RECURSION_LEVEL ((uint16_t)-1)

#define RECURSION_LEVEL_SAFE_TO_FINISH(table_)                                \
    (table_->recursionLevel == 0 ||                                           \
     table_->recursionLevel == IMMUTABLE_RECURSION_LEVEL)

#define INCREMENT_RECURSION_LEVEL(table_)                                     \
    do {                                                                      \
        if (table_->recursionLevel != IMMUTABLE_RECURSION_LEVEL)              \
            ++table_->recursionLevel;                                         \
    } while(0)
#define DECREMENT_RECURSION_LEVEL(table_)                                     \
    do {                                                                      \
        if (table->recursionLevel != IMMUTABLE_RECURSION_LEVEL) {             \
            MOZ_ASSERT(table->recursionLevel > 0);                            \
            --table->recursionLevel;                                          \
        }                                                                     \
    } while(0)

#else

#define INCREMENT_RECURSION_LEVEL(table_)   do { } while(0)
#define DECREMENT_RECURSION_LEVEL(table_)   do { } while(0)

#endif 

using namespace mozilla;

void *
PL_DHashAllocTable(PLDHashTable *table, uint32_t nbytes)
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
    return HashString(static_cast<const char*>(key));
}

PLDHashNumber
PL_DHashVoidPtrKeyStub(PLDHashTable *table, const void *key)
{
    return (PLDHashNumber)(ptrdiff_t)key >> 2;
}

bool
PL_DHashMatchEntryStub(PLDHashTable *table,
                       const PLDHashEntryHdr *entry,
                       const void *key)
{
    const PLDHashEntryStub *stub = (const PLDHashEntryStub *)entry;

    return stub->key == key;
}

bool
PL_DHashMatchStringKey(PLDHashTable *table,
                       const PLDHashEntryHdr *entry,
                       const void *key)
{
    const PLDHashEntryStub *stub = (const PLDHashEntryStub *)entry;

    
    return stub->key == key ||
           (stub->key && key &&
            strcmp((const char *) stub->key, (const char *) key) == 0);
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
    PL_DHashVoidPtrKeyStub,
    PL_DHashMatchEntryStub,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    nullptr
};

const PLDHashTableOps *
PL_DHashGetStubOps(void)
{
    return &stub_ops;
}

static bool
SizeOfEntryStore(uint32_t capacity, uint32_t entrySize, uint32_t *nbytes)
{
    uint64_t nbytes64 = uint64_t(capacity) * uint64_t(entrySize);
    *nbytes = capacity * entrySize;
    return uint64_t(*nbytes) == nbytes64;   
}

PLDHashTable *
PL_NewDHashTable(const PLDHashTableOps *ops, void *data, uint32_t entrySize,
                 uint32_t capacity)
{
    PLDHashTable *table = (PLDHashTable *) malloc(sizeof *table);
    if (!table)
        return nullptr;
    if (!PL_DHashTableInit(table, ops, data, entrySize, capacity)) {
        free(table);
        return nullptr;
    }
    return table;
}

void
PL_DHashTableDestroy(PLDHashTable *table)
{
    PL_DHashTableFinish(table);
    free(table);
}

bool
PL_DHashTableInit(PLDHashTable *table, const PLDHashTableOps *ops, void *data,
                  uint32_t entrySize, uint32_t capacity)
{
#ifdef DEBUG
    if (entrySize > 16 * sizeof(void *)) {
        printf_stderr(
                "pldhash: for the table at address %p, the given entrySize"
                " of %lu definitely favors chaining over double hashing.\n",
                (void *) table,
                (unsigned long) entrySize);
    }
#endif

    table->ops = ops;
    table->data = data;
    if (capacity < PL_DHASH_MIN_SIZE)
        capacity = PL_DHASH_MIN_SIZE;

    int log2 = CeilingLog2(capacity);

    capacity = 1u << log2;
    if (capacity > PL_DHASH_MAX_SIZE)
        return false;
    table->hashShift = PL_DHASH_BITS - log2;
    table->entrySize = entrySize;
    table->entryCount = table->removedCount = 0;
    table->generation = 0;
    uint32_t nbytes;
    if (!SizeOfEntryStore(capacity, entrySize, &nbytes))
        return false;   

    table->entryStore = (char *) ops->allocTable(table, nbytes);
    if (!table->entryStore)
        return false;
    memset(table->entryStore, 0, nbytes);
    METER(memset(&table->stats, 0, sizeof table->stats));

#ifdef DEBUG
    table->recursionLevel = 0;
#endif

    return true;
}








static inline uint32_t MaxLoad(uint32_t size) {
    return size - (size >> 2);  
}
static inline uint32_t MaxLoadOnGrowthFailure(uint32_t size) {
    return size - (size >> 5);  
}
static inline uint32_t MinLoad(uint32_t size) {
    return size >> 2;           
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
    INCREMENT_RECURSION_LEVEL(table);

    
    table->ops->finalize(table);

    
    char *entryAddr = table->entryStore;
    uint32_t entrySize = table->entrySize;
    char *entryLimit = entryAddr + PL_DHASH_TABLE_SIZE(table) * entrySize;
    while (entryAddr < entryLimit) {
        PLDHashEntryHdr *entry = (PLDHashEntryHdr *)entryAddr;
        if (ENTRY_IS_LIVE(entry)) {
            METER(table->stats.removeEnums++);
            table->ops->clearEntry(table, entry);
        }
        entryAddr += entrySize;
    }

    DECREMENT_RECURSION_LEVEL(table);
    MOZ_ASSERT(RECURSION_LEVEL_SAFE_TO_FINISH(table));

    
    table->ops->freeTable(table, table->entryStore);
}

static PLDHashEntryHdr * PL_DHASH_FASTCALL
SearchTable(PLDHashTable *table, const void *key, PLDHashNumber keyHash,
            PLDHashOperator op)
{
    METER(table->stats.searches++);
    NS_ASSERTION(!(keyHash & COLLISION_FLAG),
                 "!(keyHash & COLLISION_FLAG)");

    
    int hashShift = table->hashShift;
    PLDHashNumber hash1 = HASH1(keyHash, hashShift);
    PLDHashEntryHdr *entry = ADDRESS_ENTRY(table, hash1);

    
    if (PL_DHASH_ENTRY_IS_FREE(entry)) {
        METER(table->stats.misses++);
        return entry;
    }

    
    PLDHashMatchEntry matchEntry = table->ops->matchEntry;
    if (MATCH_ENTRY_KEYHASH(entry, keyHash) && matchEntry(table, entry, key)) {
        METER(table->stats.hits++);
        return entry;
    }

    
    int sizeLog2 = PL_DHASH_BITS - table->hashShift;
    PLDHashNumber hash2 = HASH2(keyHash, sizeLog2, hashShift);
    uint32_t sizeMask = (1u << sizeLog2) - 1;

    
    PLDHashEntryHdr *firstRemoved = nullptr;

    for (;;) {
        if (MOZ_UNLIKELY(ENTRY_IS_REMOVED(entry))) {
            if (!firstRemoved)
                firstRemoved = entry;
        } else {
            if (op == PL_DHASH_ADD)
                entry->keyHash |= COLLISION_FLAG;
        }

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
    }

    
    return nullptr;
}











static PLDHashEntryHdr * PL_DHASH_FASTCALL
FindFreeEntry(PLDHashTable *table, PLDHashNumber keyHash)
{
    METER(table->stats.searches++);
    NS_ASSERTION(!(keyHash & COLLISION_FLAG),
                 "!(keyHash & COLLISION_FLAG)");

    
    int hashShift = table->hashShift;
    PLDHashNumber hash1 = HASH1(keyHash, hashShift);
    PLDHashEntryHdr *entry = ADDRESS_ENTRY(table, hash1);

    
    if (PL_DHASH_ENTRY_IS_FREE(entry)) {
        METER(table->stats.misses++);
        return entry;
    }

    
    int sizeLog2 = PL_DHASH_BITS - table->hashShift;
    PLDHashNumber hash2 = HASH2(keyHash, sizeLog2, hashShift);
    uint32_t sizeMask = (1u << sizeLog2) - 1;

    for (;;) {
        NS_ASSERTION(!ENTRY_IS_REMOVED(entry),
                     "!ENTRY_IS_REMOVED(entry)");
        entry->keyHash |= COLLISION_FLAG;

        METER(table->stats.steps++);
        hash1 -= hash2;
        hash1 &= sizeMask;

        entry = ADDRESS_ENTRY(table, hash1);
        if (PL_DHASH_ENTRY_IS_FREE(entry)) {
            METER(table->stats.misses++);
            return entry;
        }
    }

    
    return nullptr;
}

static bool
ChangeTable(PLDHashTable *table, int deltaLog2)
{
    
    int oldLog2 = PL_DHASH_BITS - table->hashShift;
    int newLog2 = oldLog2 + deltaLog2;
    uint32_t newCapacity = 1u << newLog2;
    if (newCapacity > PL_DHASH_MAX_SIZE)
        return false;

    uint32_t entrySize = table->entrySize;
    uint32_t nbytes;
    if (!SizeOfEntryStore(newCapacity, entrySize, &nbytes))
        return false;   

    char *newEntryStore = (char *) table->ops->allocTable(table, nbytes);
    if (!newEntryStore)
        return false;

    
#ifdef DEBUG
    uint32_t recursionLevel = table->recursionLevel;
#endif
    table->hashShift = PL_DHASH_BITS - newLog2;
    table->removedCount = 0;
    table->generation++;

    
    memset(newEntryStore, 0, nbytes);
    char *oldEntryStore, *oldEntryAddr;
    oldEntryAddr = oldEntryStore = table->entryStore;
    table->entryStore = newEntryStore;
    PLDHashMoveEntry moveEntry = table->ops->moveEntry;
#ifdef DEBUG
    table->recursionLevel = recursionLevel;
#endif

    
    uint32_t oldCapacity = 1u << oldLog2;
    for (uint32_t i = 0; i < oldCapacity; i++) {
        PLDHashEntryHdr *oldEntry = (PLDHashEntryHdr *)oldEntryAddr;
        if (ENTRY_IS_LIVE(oldEntry)) {
            oldEntry->keyHash &= ~COLLISION_FLAG;
            PLDHashEntryHdr *newEntry = FindFreeEntry(table, oldEntry->keyHash);
            NS_ASSERTION(PL_DHASH_ENTRY_IS_FREE(newEntry),
                         "PL_DHASH_ENTRY_IS_FREE(newEntry)");
            moveEntry(table, oldEntry, newEntry);
            newEntry->keyHash = oldEntry->keyHash;
        }
        oldEntryAddr += entrySize;
    }

    table->ops->freeTable(table, oldEntryStore);
    return true;
}

PLDHashEntryHdr * PL_DHASH_FASTCALL
PL_DHashTableOperate(PLDHashTable *table, const void *key, PLDHashOperator op)
{
    PLDHashEntryHdr *entry;

    MOZ_ASSERT(op == PL_DHASH_LOOKUP || table->recursionLevel == 0);
    INCREMENT_RECURSION_LEVEL(table);

    PLDHashNumber keyHash = table->ops->hashKey(table, key);
    keyHash *= PL_DHASH_GOLDEN_RATIO;

    
    ENSURE_LIVE_KEYHASH(keyHash);
    keyHash &= ~COLLISION_FLAG;

    switch (op) {
      case PL_DHASH_LOOKUP:
        METER(table->stats.lookups++);
        entry = SearchTable(table, key, keyHash, op);
        break;

      case PL_DHASH_ADD: {
        




        uint32_t size = PL_DHASH_TABLE_SIZE(table);
        if (table->entryCount + table->removedCount >= MaxLoad(size)) {
            
            int deltaLog2;
            if (table->removedCount >= size >> 2) {
                METER(table->stats.compresses++);
                deltaLog2 = 0;
            } else {
                METER(table->stats.grows++);
                deltaLog2 = 1;
            }

            




            if (!ChangeTable(table, deltaLog2) &&
                table->entryCount + table->removedCount >=
                    MaxLoadOnGrowthFailure(size))
            {
                METER(table->stats.addFailures++);
                entry = nullptr;
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
                entry = nullptr;
                break;
            }
            entry->keyHash = keyHash;
            table->entryCount++;
        }
        METER(else table->stats.addHits++);
        break;
      }

      case PL_DHASH_REMOVE:
        entry = SearchTable(table, key, keyHash, op);
        if (ENTRY_IS_LIVE(entry)) {
            
            METER(table->stats.removeHits++);
            PL_DHashTableRawRemove(table, entry);

            
            uint32_t size = PL_DHASH_TABLE_SIZE(table);
            if (size > PL_DHASH_MIN_SIZE &&
                table->entryCount <= MinLoad(size)) {
                METER(table->stats.shrinks++);
                (void) ChangeTable(table, -1);
            }
        }
        METER(else table->stats.removeMisses++);
        entry = nullptr;
        break;

      default:
        NS_NOTREACHED("0");
        entry = nullptr;
    }

    DECREMENT_RECURSION_LEVEL(table);

    return entry;
}

void
PL_DHashTableRawRemove(PLDHashTable *table, PLDHashEntryHdr *entry)
{
    MOZ_ASSERT(table->recursionLevel != IMMUTABLE_RECURSION_LEVEL);

    NS_ASSERTION(PL_DHASH_ENTRY_IS_LIVE(entry),
                 "PL_DHASH_ENTRY_IS_LIVE(entry)");

    
    PLDHashNumber keyHash = entry->keyHash;
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

uint32_t
PL_DHashTableEnumerate(PLDHashTable *table, PLDHashEnumerator etor, void *arg)
{
    INCREMENT_RECURSION_LEVEL(table);

    char *entryAddr = table->entryStore;
    uint32_t entrySize = table->entrySize;
    uint32_t capacity = PL_DHASH_TABLE_SIZE(table);
    char *entryLimit = entryAddr + capacity * entrySize;
    uint32_t i = 0;
    bool didRemove = false;
    while (entryAddr < entryLimit) {
        PLDHashEntryHdr *entry = (PLDHashEntryHdr *)entryAddr;
        if (ENTRY_IS_LIVE(entry)) {
            PLDHashOperator op = etor(table, entry, i++, arg);
            if (op & PL_DHASH_REMOVE) {
                METER(table->stats.removeEnums++);
                PL_DHashTableRawRemove(table, entry);
                didRemove = true;
            }
            if (op & PL_DHASH_STOP)
                break;
        }
        entryAddr += entrySize;
    }

    MOZ_ASSERT(!didRemove || table->recursionLevel == 1);

    






    if (didRemove &&
        (table->removedCount >= capacity >> 2 ||
         (capacity > PL_DHASH_MIN_SIZE &&
          table->entryCount <= MinLoad(capacity)))) {
        METER(table->stats.enumShrinks++);
        capacity = table->entryCount;
        capacity += capacity >> 1;
        if (capacity < PL_DHASH_MIN_SIZE)
            capacity = PL_DHASH_MIN_SIZE;

        uint32_t ceiling = CeilingLog2(capacity);
        ceiling -= PL_DHASH_BITS - table->hashShift;

        (void) ChangeTable(table, ceiling);
    }

    DECREMENT_RECURSION_LEVEL(table);

    return i;
}

struct SizeOfEntryExcludingThisArg
{
    size_t total;
    PLDHashSizeOfEntryExcludingThisFun sizeOfEntryExcludingThis;
    MallocSizeOf mallocSizeOf;
    void *arg;      
};

static PLDHashOperator
SizeOfEntryExcludingThisEnumerator(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                   uint32_t number, void *arg)
{
    SizeOfEntryExcludingThisArg *e = (SizeOfEntryExcludingThisArg *)arg;
    e->total += e->sizeOfEntryExcludingThis(hdr, e->mallocSizeOf, e->arg);
    return PL_DHASH_NEXT;
}

size_t
PL_DHashTableSizeOfExcludingThis(const PLDHashTable *table,
                                 PLDHashSizeOfEntryExcludingThisFun sizeOfEntryExcludingThis,
                                 MallocSizeOf mallocSizeOf,
                                 void *arg )
{
    size_t n = 0;
    n += mallocSizeOf(table->entryStore);
    if (sizeOfEntryExcludingThis) {
        SizeOfEntryExcludingThisArg arg2 = { 0, sizeOfEntryExcludingThis, mallocSizeOf, arg };
        PL_DHashTableEnumerate(const_cast<PLDHashTable *>(table),
                               SizeOfEntryExcludingThisEnumerator, &arg2);
        n += arg2.total;
    }
    return n;
}

size_t
PL_DHashTableSizeOfIncludingThis(const PLDHashTable *table,
                                 PLDHashSizeOfEntryExcludingThisFun sizeOfEntryExcludingThis,
                                 MallocSizeOf mallocSizeOf,
                                 void *arg )
{
    return mallocSizeOf(table) +
           PL_DHashTableSizeOfExcludingThis(table, sizeOfEntryExcludingThis,
                                            mallocSizeOf, arg);
}

#ifdef DEBUG
void
PL_DHashMarkTableImmutable(PLDHashTable *table)
{
    table->recursionLevel = IMMUTABLE_RECURSION_LEVEL;
}
#endif

#ifdef PL_DHASHMETER
#include <math.h>

void
PL_DHashTableDumpMeter(PLDHashTable *table, PLDHashEnumerator dump, FILE *fp)
{
    PLDHashNumber hash1, hash2, maxChainHash1, maxChainHash2;
    double sqsum, mean, variance, sigma;
    PLDHashEntryHdr *entry;

    char *entryAddr = table->entryStore;
    uint32_t entrySize = table->entrySize;
    int hashShift = table->hashShift;
    int sizeLog2 = PL_DHASH_BITS - hashShift;
    uint32_t tableSize = PL_DHASH_TABLE_SIZE(table);
    uint32_t sizeMask = (1u << sizeLog2) - 1;
    uint32_t chainCount = 0, maxChainLen = 0;
    hash2 = 0;
    sqsum = 0;

    for (uint32_t i = 0; i < tableSize; i++) {
        entry = (PLDHashEntryHdr *)entryAddr;
        entryAddr += entrySize;
        if (!ENTRY_IS_LIVE(entry))
            continue;
        hash1 = HASH1(entry->keyHash & ~COLLISION_FLAG, hashShift);
        PLDHashNumber saveHash1 = hash1;
        PLDHashEntryHdr *probe = ADDRESS_ENTRY(table, hash1);
        uint32_t chainLen = 1;
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

    uint32_t entryCount = table->entryCount;
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
        uint32_t i = 0;
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
