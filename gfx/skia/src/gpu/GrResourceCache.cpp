









#include "GrResourceCache.h"
#include "GrResource.h"

GrResourceEntry::GrResourceEntry(const GrResourceKey& key, GrResource* resource)
        : fKey(key), fResource(resource) {
    
    GrAssert(resource);
    resource->ref();
}

GrResourceEntry::~GrResourceEntry() {
    fResource->setCacheEntry(NULL);
    fResource->unref();
}

#if GR_DEBUG
void GrResourceEntry::validate() const {
    GrAssert(fResource);
    GrAssert(fResource->getCacheEntry() == this);
    fResource->validate();
}
#endif



class GrResourceCache::Key {
    typedef GrResourceEntry T;

    const GrResourceKey& fKey;
public:
    Key(const GrResourceKey& key) : fKey(key) {}

    uint32_t getHash() const { return fKey.hashIndex(); }

    static bool LT(const T& entry, const Key& key) {
        return entry.key() < key.fKey;
    }
    static bool EQ(const T& entry, const Key& key) {
        return entry.key() == key.fKey;
    }
#if GR_DEBUG
    static uint32_t GetHash(const T& entry) {
        return entry.key().hashIndex();
    }
    static bool LT(const T& a, const T& b) {
        return a.key() < b.key();
    }
    static bool EQ(const T& a, const T& b) {
        return a.key() == b.key();
    }
#endif
};



GrResourceCache::GrResourceCache(int maxCount, size_t maxBytes) :
        fMaxCount(maxCount),
        fMaxBytes(maxBytes) {
#if GR_CACHE_STATS
    fHighWaterEntryCount          = 0;
    fHighWaterEntryBytes          = 0;
    fHighWaterClientDetachedCount = 0;
    fHighWaterClientDetachedBytes = 0;
#endif

    fEntryCount                   = 0;
    fEntryBytes                   = 0;
    fClientDetachedCount          = 0;
    fClientDetachedBytes          = 0;

    fPurging = false;
}

GrResourceCache::~GrResourceCache() {
    GrAutoResourceCacheValidate atcv(this);

    EntryList::Iter iter;

    
    while (GrResourceEntry* entry = fList.head()) {
        GrAutoResourceCacheValidate atcv(this);

        
        fCache.remove(entry->fKey, entry);

        
        this->internalDetach(entry, false);

        delete entry;
    }
}

void GrResourceCache::getLimits(int* maxResources, size_t* maxResourceBytes) const{
    if (maxResources) {
        *maxResources = fMaxCount;
    }
    if (maxResourceBytes) {
        *maxResourceBytes = fMaxBytes;
    }
}

void GrResourceCache::setLimits(int maxResources, size_t maxResourceBytes) {
    bool smaller = (maxResources < fMaxCount) || (maxResourceBytes < fMaxBytes);

    fMaxCount = maxResources;
    fMaxBytes = maxResourceBytes;

    if (smaller) {
        this->purgeAsNeeded();
    }
}

void GrResourceCache::internalDetach(GrResourceEntry* entry,
                                    bool clientDetach) {
    fList.remove(entry);

    
    if (clientDetach) {
        fClientDetachedCount += 1;
        fClientDetachedBytes += entry->resource()->sizeInBytes();

#if GR_CACHE_STATS
        if (fHighWaterClientDetachedCount < fClientDetachedCount) {
            fHighWaterClientDetachedCount = fClientDetachedCount;
        }
        if (fHighWaterClientDetachedBytes < fClientDetachedBytes) {
            fHighWaterClientDetachedBytes = fClientDetachedBytes;
        }
#endif

    } else {
        fEntryCount -= 1;
        fEntryBytes -= entry->resource()->sizeInBytes();
    }
}

void GrResourceCache::attachToHead(GrResourceEntry* entry,
                                   bool clientReattach) {
    fList.addToHead(entry);

    
    if (clientReattach) {
        fClientDetachedCount -= 1;
        fClientDetachedBytes -= entry->resource()->sizeInBytes();
    } else {
        fEntryCount += 1;
        fEntryBytes += entry->resource()->sizeInBytes();

#if GR_CACHE_STATS
        if (fHighWaterEntryCount < fEntryCount) {
            fHighWaterEntryCount = fEntryCount;
        }
        if (fHighWaterEntryBytes < fEntryBytes) {
            fHighWaterEntryBytes = fEntryBytes;
        }
#endif
    }
}

GrResource* GrResourceCache::find(const GrResourceKey& key) {
    GrAutoResourceCacheValidate atcv(this);

    GrResourceEntry* entry = fCache.find(key);
    if (NULL == entry) {
        return NULL;
    }

    this->internalDetach(entry, false);
    this->attachToHead(entry, false);

    return entry->fResource;
}

bool GrResourceCache::hasKey(const GrResourceKey& key) const {
    return NULL != fCache.find(key);
}

void GrResourceCache::create(const GrResourceKey& key, GrResource* resource) {
    GrAssert(NULL == resource->getCacheEntry());
    
    
    
    
    GrAssert(!fPurging);
    GrAutoResourceCacheValidate atcv(this);

    GrResourceEntry* entry = SkNEW_ARGS(GrResourceEntry, (key, resource));
    resource->setCacheEntry(entry);

    this->attachToHead(entry, false);
    fCache.insert(key, entry);

#if GR_DUMP_TEXTURE_UPLOAD
    GrPrintf("--- add resource to cache %p, count=%d bytes= %d %d\n",
             entry, fEntryCount, resource->sizeInBytes(), fEntryBytes);
#endif
}

void GrResourceCache::makeExclusive(GrResourceEntry* entry) {
    GrAutoResourceCacheValidate atcv(this);

    this->internalDetach(entry, true);
    fCache.remove(entry->key(), entry);

#if GR_DEBUG
    fExclusiveList.addToHead(entry);
#endif
}

void GrResourceCache::removeInvalidResource(GrResourceEntry* entry) {
    
    
    
    
    
    fClientDetachedCount -= 1;
    fEntryCount -= 1;
    size_t size = entry->resource()->sizeInBytes();
    fClientDetachedBytes -= size;
    fEntryBytes -= size;
}

void GrResourceCache::makeNonExclusive(GrResourceEntry* entry) {
    GrAutoResourceCacheValidate atcv(this);

#if GR_DEBUG
    fExclusiveList.remove(entry);
#endif

    if (entry->resource()->isValid()) {
        attachToHead(entry, true);
        fCache.insert(entry->key(), entry);
    } else {
        this->removeInvalidResource(entry);
    }
}











void GrResourceCache::purgeAsNeeded() {
    if (!fPurging) {
        fPurging = true;
        bool withinBudget = false;
        bool changed = false;

        
        
        do {
            EntryList::Iter iter;

            changed = false;

            
            
            
            
            GrResourceEntry* entry = iter.init(fList, EntryList::Iter::kTail_IterStart);

            while (NULL != entry) {
                GrAutoResourceCacheValidate atcv(this);

                if (fEntryCount <= fMaxCount && fEntryBytes <= fMaxBytes) {
                    withinBudget = true;
                    break;
                }

                GrResourceEntry* prev = iter.prev();
                if (1 == entry->fResource->getRefCnt()) {
                    changed = true;

                    
                    fCache.remove(entry->key(), entry);

                    
                    this->internalDetach(entry, false);

        #if GR_DUMP_TEXTURE_UPLOAD
                    GrPrintf("--- ~resource from cache %p [%d %d]\n",
                             entry->resource(),
                             entry->resource()->width(),
                             entry->resource()->height());
        #endif

                    delete entry;
                }
                entry = prev;
            }
        } while (!withinBudget && changed);
        fPurging = false;
    }
}

void GrResourceCache::purgeAllUnlocked() {
    GrAutoResourceCacheValidate atcv(this);

    
    
    

    int savedMaxBytes = fMaxBytes;
    int savedMaxCount = fMaxCount;
    fMaxBytes = (size_t) -1;
    fMaxCount = 0;
    this->purgeAsNeeded();

#if GR_DEBUG
    GrAssert(fExclusiveList.countEntries() == fClientDetachedCount);
    GrAssert(countBytes(fExclusiveList) == fClientDetachedBytes);
    if (!fCache.count()) {
        
        
        
        GrAssert(fEntryCount == fClientDetachedCount);
        GrAssert(fEntryBytes == fClientDetachedBytes);
        GrAssert(fList.isEmpty());
    }
#endif

    fMaxBytes = savedMaxBytes;
    fMaxCount = savedMaxCount;
}



#if GR_DEBUG
size_t GrResourceCache::countBytes(const EntryList& list) {
    size_t bytes = 0;

    EntryList::Iter iter;

    const GrResourceEntry* entry = iter.init(const_cast<EntryList&>(list),
                                             EntryList::Iter::kTail_IterStart);

    for ( ; NULL != entry; entry = iter.prev()) {
        bytes += entry->resource()->sizeInBytes();
    }
    return bytes;
}

static bool both_zero_or_nonzero(int count, size_t bytes) {
    return (count == 0 && bytes == 0) || (count > 0 && bytes > 0);
}

void GrResourceCache::validate() const {
    fList.validate();
    fExclusiveList.validate();
    GrAssert(both_zero_or_nonzero(fEntryCount, fEntryBytes));
    GrAssert(both_zero_or_nonzero(fClientDetachedCount, fClientDetachedBytes));
    GrAssert(fClientDetachedBytes <= fEntryBytes);
    GrAssert(fClientDetachedCount <= fEntryCount);
    GrAssert((fEntryCount - fClientDetachedCount) == fCache.count());

    fCache.validate();


    EntryList::Iter iter;

    
    const GrResourceEntry* entry = iter.init(const_cast<EntryList&>(fExclusiveList),
                                             EntryList::Iter::kHead_IterStart);

    for ( ; NULL != entry; entry = iter.next()) {
        entry->validate();
    }

    
    entry = iter.init(const_cast<EntryList&>(fList), EntryList::Iter::kHead_IterStart);

    int count = 0;
    for ( ; NULL != entry; entry = iter.next()) {
        entry->validate();
        GrAssert(fCache.find(entry->key()));
        count += 1;
    }
    GrAssert(count == fEntryCount - fClientDetachedCount);

    size_t bytes = countBytes(fList);
    GrAssert(bytes == fEntryBytes  - fClientDetachedBytes);

    bytes = countBytes(fExclusiveList);
    GrAssert(bytes == fClientDetachedBytes);

    GrAssert(fList.countEntries() == fEntryCount - fClientDetachedCount);

    GrAssert(fExclusiveList.countEntries() == fClientDetachedCount);
}
#endif 

#if GR_CACHE_STATS

void GrResourceCache::printStats() {
    int locked = 0;

    EntryList::Iter iter;

    GrResourceEntry* entry = iter.init(fList, EntryList::Iter::kTail_IterStart);

    for ( ; NULL != entry; entry = iter.prev()) {
        if (entry->fResource->getRefCnt() > 1) {
            ++locked;
        }
    }

    SkDebugf("Budget: %d items %d bytes\n", fMaxCount, fMaxBytes);
    SkDebugf("\t\tEntry Count: current %d (%d locked) high %d\n",
                fEntryCount, locked, fHighWaterEntryCount);
    SkDebugf("\t\tEntry Bytes: current %d high %d\n",
                fEntryBytes, fHighWaterEntryBytes);
    SkDebugf("\t\tDetached Entry Count: current %d high %d\n",
                fClientDetachedCount, fHighWaterClientDetachedCount);
    SkDebugf("\t\tDetached Bytes: current %d high %d\n",
                fClientDetachedBytes, fHighWaterClientDetachedBytes);
}

#endif


