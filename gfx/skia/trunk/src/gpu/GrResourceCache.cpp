









#include "GrResourceCache.h"
#include "GrResource.h"

DECLARE_SKMESSAGEBUS_MESSAGE(GrResourceInvalidatedMessage);

GrResourceKey::ResourceType GrResourceKey::GenerateResourceType() {
    static int32_t gNextType = 0;

    int32_t type = sk_atomic_inc(&gNextType);
    if (type >= (1 << 8 * sizeof(ResourceType))) {
        GrCrash("Too many Resource Types");
    }

    return static_cast<ResourceType>(type);
}



GrResourceEntry::GrResourceEntry(const GrResourceKey& key, GrResource* resource)
        : fKey(key), fResource(resource) {
    
    SkASSERT(resource);
    resource->ref();
}

GrResourceEntry::~GrResourceEntry() {
    fResource->setCacheEntry(NULL);
    fResource->unref();
}

#ifdef SK_DEBUG
void GrResourceEntry::validate() const {
    SkASSERT(fResource);
    SkASSERT(fResource->getCacheEntry() == this);
    fResource->validate();
}
#endif



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

    fPurging                      = false;

    fOverbudgetCB                 = NULL;
    fOverbudgetData               = NULL;
}

GrResourceCache::~GrResourceCache() {
    GrAutoResourceCacheValidate atcv(this);

    EntryList::Iter iter;

    
    while (GrResourceEntry* entry = fList.head()) {
        GrAutoResourceCacheValidate atcv(this);

        
        fCache.remove(entry->fKey, entry);

        
        this->internalDetach(entry);

        delete entry;
    }
}

void GrResourceCache::getLimits(int* maxResources, size_t* maxResourceBytes) const{
    if (NULL != maxResources) {
        *maxResources = fMaxCount;
    }
    if (NULL != maxResourceBytes) {
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
                                     BudgetBehaviors behavior) {
    fList.remove(entry);

    
    if (kIgnore_BudgetBehavior == behavior) {
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
        SkASSERT(kAccountFor_BudgetBehavior == behavior);

        fEntryCount -= 1;
        fEntryBytes -= entry->resource()->sizeInBytes();
    }
}

void GrResourceCache::attachToHead(GrResourceEntry* entry,
                                   BudgetBehaviors behavior) {
    fList.addToHead(entry);

    
    if (kIgnore_BudgetBehavior == behavior) {
        fClientDetachedCount -= 1;
        fClientDetachedBytes -= entry->resource()->sizeInBytes();
    } else {
        SkASSERT(kAccountFor_BudgetBehavior == behavior);

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




class GrTFindUnreffedFunctor {
public:
    bool operator()(const GrResourceEntry* entry) const {
        return entry->resource()->unique();
    }
};

GrResource* GrResourceCache::find(const GrResourceKey& key, uint32_t ownershipFlags) {
    GrAutoResourceCacheValidate atcv(this);

    GrResourceEntry* entry = NULL;

    if (ownershipFlags & kNoOtherOwners_OwnershipFlag) {
        GrTFindUnreffedFunctor functor;

        entry = fCache.find<GrTFindUnreffedFunctor>(key, functor);
    } else {
        entry = fCache.find(key);
    }

    if (NULL == entry) {
        return NULL;
    }

    if (ownershipFlags & kHide_OwnershipFlag) {
        this->makeExclusive(entry);
    } else {
        
        this->internalDetach(entry);
        this->attachToHead(entry);
    }

    return entry->fResource;
}

void GrResourceCache::addResource(const GrResourceKey& key,
                                  GrResource* resource,
                                  uint32_t ownershipFlags) {
    SkASSERT(NULL == resource->getCacheEntry());
    
    
    
    
    SkASSERT(!fPurging);
    GrAutoResourceCacheValidate atcv(this);

    GrResourceEntry* entry = SkNEW_ARGS(GrResourceEntry, (key, resource));
    resource->setCacheEntry(entry);

    this->attachToHead(entry);
    fCache.insert(key, entry);

    if (ownershipFlags & kHide_OwnershipFlag) {
        this->makeExclusive(entry);
    }

}

void GrResourceCache::makeExclusive(GrResourceEntry* entry) {
    GrAutoResourceCacheValidate atcv(this);

    
    
    this->internalDetach(entry, kIgnore_BudgetBehavior);
    fCache.remove(entry->key(), entry);

#ifdef SK_DEBUG
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

#ifdef SK_DEBUG
    fExclusiveList.remove(entry);
#endif

    if (entry->resource()->isValid()) {
        
        
        
        attachToHead(entry, kIgnore_BudgetBehavior);
        fCache.insert(entry->key(), entry);
    } else {
        this->removeInvalidResource(entry);
    }
}















void GrResourceCache::purgeAsNeeded(int extraCount, size_t extraBytes) {
    if (fPurging) {
        return;
    }

    fPurging = true;

    this->purgeInvalidated();

    this->internalPurge(extraCount, extraBytes);
    if (((fEntryCount+extraCount) > fMaxCount ||
        (fEntryBytes+extraBytes) > fMaxBytes) &&
        NULL != fOverbudgetCB) {
        
        
        if ((*fOverbudgetCB)(fOverbudgetData)) {
            this->internalPurge(extraCount, extraBytes);
        }
    }

    fPurging = false;
}

void GrResourceCache::purgeInvalidated() {
    SkTDArray<GrResourceInvalidatedMessage> invalidated;
    fInvalidationInbox.poll(&invalidated);

    for (int i = 0; i < invalidated.count(); i++) {
        
        
        
        
        
        
        
        while (GrResourceEntry* entry = fCache.find(invalidated[i].key, GrTFindUnreffedFunctor())) {
            this->deleteResource(entry);
        }
    }
}

void GrResourceCache::deleteResource(GrResourceEntry* entry) {
    SkASSERT(1 == entry->fResource->getRefCnt());

    
    fCache.remove(entry->key(), entry);

    
    this->internalDetach(entry);
    delete entry;
}

void GrResourceCache::internalPurge(int extraCount, size_t extraBytes) {
    SkASSERT(fPurging);

    bool withinBudget = false;
    bool changed = false;

    
    
    do {
        EntryList::Iter iter;

        changed = false;

        
        
        
        
        GrResourceEntry* entry = iter.init(fList, EntryList::Iter::kTail_IterStart);

        while (NULL != entry) {
            GrAutoResourceCacheValidate atcv(this);

            if ((fEntryCount+extraCount) <= fMaxCount &&
                (fEntryBytes+extraBytes) <= fMaxBytes) {
                withinBudget = true;
                break;
            }

            GrResourceEntry* prev = iter.prev();
            if (entry->fResource->unique()) {
                changed = true;
                this->deleteResource(entry);
            }
            entry = prev;
        }
    } while (!withinBudget && changed);
}

void GrResourceCache::purgeAllUnlocked() {
    GrAutoResourceCacheValidate atcv(this);

    
    
    

    size_t savedMaxBytes = fMaxBytes;
    int savedMaxCount = fMaxCount;
    fMaxBytes = (size_t) -1;
    fMaxCount = 0;
    this->purgeAsNeeded();

#ifdef SK_DEBUG
    SkASSERT(fExclusiveList.countEntries() == fClientDetachedCount);
    SkASSERT(countBytes(fExclusiveList) == fClientDetachedBytes);
    if (!fCache.count()) {
        
        
        
        SkASSERT(fEntryCount == fClientDetachedCount);
        SkASSERT(fEntryBytes == fClientDetachedBytes);
        SkASSERT(fList.isEmpty());
    }
#endif

    fMaxBytes = savedMaxBytes;
    fMaxCount = savedMaxCount;
}



#ifdef SK_DEBUG
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
    SkASSERT(both_zero_or_nonzero(fEntryCount, fEntryBytes));
    SkASSERT(both_zero_or_nonzero(fClientDetachedCount, fClientDetachedBytes));
    SkASSERT(fClientDetachedBytes <= fEntryBytes);
    SkASSERT(fClientDetachedCount <= fEntryCount);
    SkASSERT((fEntryCount - fClientDetachedCount) == fCache.count());

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
        SkASSERT(fCache.find(entry->key()));
        count += 1;
    }
    SkASSERT(count == fEntryCount - fClientDetachedCount);

    size_t bytes = countBytes(fList);
    SkASSERT(bytes == fEntryBytes  - fClientDetachedBytes);

    bytes = countBytes(fExclusiveList);
    SkASSERT(bytes == fClientDetachedBytes);

    SkASSERT(fList.countEntries() == fEntryCount - fClientDetachedCount);

    SkASSERT(fExclusiveList.countEntries() == fClientDetachedCount);
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


