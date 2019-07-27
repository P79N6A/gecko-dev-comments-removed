






#include "SkDiscardableMemory.h"
#include "SkDiscardableMemoryPool.h"
#include "SkImageGenerator.h"
#include "SkLazyPtr.h"
#include "SkTInternalLList.h"
#include "SkThread.h"





namespace {

class PoolDiscardableMemory;





class DiscardableMemoryPool : public SkDiscardableMemoryPool {
public:
    


    DiscardableMemoryPool(size_t budget, SkBaseMutex* mutex = NULL);
    virtual ~DiscardableMemoryPool();

    virtual SkDiscardableMemory* create(size_t bytes) SK_OVERRIDE;

    virtual size_t getRAMUsed() SK_OVERRIDE;
    virtual void setRAMBudget(size_t budget) SK_OVERRIDE;
    virtual size_t getRAMBudget() SK_OVERRIDE { return fBudget; }

    
    virtual void dumpPool() SK_OVERRIDE;

    #if SK_LAZY_CACHE_STATS  
    virtual int getCacheHits() SK_OVERRIDE { return fCacheHits; }
    virtual int getCacheMisses() SK_OVERRIDE { return fCacheMisses; }
    virtual void resetCacheHitsAndMisses() SK_OVERRIDE {
        fCacheHits = fCacheMisses = 0;
    }
    int          fCacheHits;
    int          fCacheMisses;
    #endif  

private:
    SkBaseMutex* fMutex;
    size_t       fBudget;
    size_t       fUsed;
    SkTInternalLList<PoolDiscardableMemory> fList;

    
    void dumpDownTo(size_t budget);
    
    void free(PoolDiscardableMemory* dm);
    
    bool lock(PoolDiscardableMemory* dm);
    
    void unlock(PoolDiscardableMemory* dm);

    friend class PoolDiscardableMemory;

    typedef SkDiscardableMemory::Factory INHERITED;
};





class PoolDiscardableMemory : public SkDiscardableMemory {
public:
    PoolDiscardableMemory(DiscardableMemoryPool* pool,
                            void* pointer, size_t bytes);
    virtual ~PoolDiscardableMemory();
    virtual bool lock() SK_OVERRIDE;
    virtual void* data() SK_OVERRIDE;
    virtual void unlock() SK_OVERRIDE;
    friend class DiscardableMemoryPool;
private:
    SK_DECLARE_INTERNAL_LLIST_INTERFACE(PoolDiscardableMemory);
    DiscardableMemoryPool* const fPool;
    bool                         fLocked;
    void*                        fPointer;
    const size_t                 fBytes;
};

PoolDiscardableMemory::PoolDiscardableMemory(DiscardableMemoryPool* pool,
                                             void* pointer,
                                             size_t bytes)
    : fPool(pool)
    , fLocked(true)
    , fPointer(pointer)
    , fBytes(bytes) {
    SkASSERT(fPool != NULL);
    SkASSERT(fPointer != NULL);
    SkASSERT(fBytes > 0);
    fPool->ref();
}

PoolDiscardableMemory::~PoolDiscardableMemory() {
    SkASSERT(!fLocked); 
    fPool->free(this);
    fPool->unref();
}

bool PoolDiscardableMemory::lock() {
    SkASSERT(!fLocked); 
    return fPool->lock(this);
}

void* PoolDiscardableMemory::data() {
    SkASSERT(fLocked); 
    return fPointer;
}

void PoolDiscardableMemory::unlock() {
    SkASSERT(fLocked); 
    fPool->unlock(this);
}



DiscardableMemoryPool::DiscardableMemoryPool(size_t budget,
                                             SkBaseMutex* mutex)
    : fMutex(mutex)
    , fBudget(budget)
    , fUsed(0) {
    #if SK_LAZY_CACHE_STATS
    fCacheHits = 0;
    fCacheMisses = 0;
    #endif  
}
DiscardableMemoryPool::~DiscardableMemoryPool() {
    
    
    
    SkASSERT(fList.isEmpty());
}

void DiscardableMemoryPool::dumpDownTo(size_t budget) {
    if (fMutex != NULL) {
        fMutex->assertHeld();
    }
    if (fUsed <= budget) {
        return;
    }
    typedef SkTInternalLList<PoolDiscardableMemory>::Iter Iter;
    Iter iter;
    PoolDiscardableMemory* cur = iter.init(fList, Iter::kTail_IterStart);
    while ((fUsed > budget) && (NULL != cur)) {
        if (!cur->fLocked) {
            PoolDiscardableMemory* dm = cur;
            SkASSERT(dm->fPointer != NULL);
            sk_free(dm->fPointer);
            dm->fPointer = NULL;
            SkASSERT(fUsed >= dm->fBytes);
            fUsed -= dm->fBytes;
            cur = iter.prev();
            
            
            fList.remove(dm);
        } else {
            cur = iter.prev();
        }
    }
}

SkDiscardableMemory* DiscardableMemoryPool::create(size_t bytes) {
    void* addr = sk_malloc_flags(bytes, 0);
    if (NULL == addr) {
        return NULL;
    }
    PoolDiscardableMemory* dm = SkNEW_ARGS(PoolDiscardableMemory,
                                             (this, addr, bytes));
    SkAutoMutexAcquire autoMutexAcquire(fMutex);
    fList.addToHead(dm);
    fUsed += bytes;
    this->dumpDownTo(fBudget);
    return dm;
}

void DiscardableMemoryPool::free(PoolDiscardableMemory* dm) {
    
    if (dm->fPointer != NULL) {
        SkAutoMutexAcquire autoMutexAcquire(fMutex);
        sk_free(dm->fPointer);
        dm->fPointer = NULL;
        SkASSERT(fUsed >= dm->fBytes);
        fUsed -= dm->fBytes;
        fList.remove(dm);
    } else {
        SkASSERT(!fList.isInList(dm));
    }
}

bool DiscardableMemoryPool::lock(PoolDiscardableMemory* dm) {
    SkASSERT(dm != NULL);
    if (NULL == dm->fPointer) {
        #if SK_LAZY_CACHE_STATS
        SkAutoMutexAcquire autoMutexAcquire(fMutex);
        ++fCacheMisses;
        #endif  
        return false;
    }
    SkAutoMutexAcquire autoMutexAcquire(fMutex);
    if (NULL == dm->fPointer) {
        
        #if SK_LAZY_CACHE_STATS
        ++fCacheMisses;
        #endif  
        return false;
    }
    dm->fLocked = true;
    fList.remove(dm);
    fList.addToHead(dm);
    #if SK_LAZY_CACHE_STATS
    ++fCacheHits;
    #endif  
    return true;
}

void DiscardableMemoryPool::unlock(PoolDiscardableMemory* dm) {
    SkASSERT(dm != NULL);
    SkAutoMutexAcquire autoMutexAcquire(fMutex);
    dm->fLocked = false;
    this->dumpDownTo(fBudget);
}

size_t DiscardableMemoryPool::getRAMUsed() {
    return fUsed;
}
void DiscardableMemoryPool::setRAMBudget(size_t budget) {
    SkAutoMutexAcquire autoMutexAcquire(fMutex);
    fBudget = budget;
    this->dumpDownTo(fBudget);
}
void DiscardableMemoryPool::dumpPool() {
    SkAutoMutexAcquire autoMutexAcquire(fMutex);
    this->dumpDownTo(0);
}


SK_DECLARE_STATIC_MUTEX(gMutex);
SkDiscardableMemoryPool* create_global_pool() {
    return SkDiscardableMemoryPool::Create(SK_DEFAULT_GLOBAL_DISCARDABLE_MEMORY_POOL_SIZE,
                                           &gMutex);
}

}  

SkDiscardableMemoryPool* SkDiscardableMemoryPool::Create(size_t size, SkBaseMutex* mutex) {
    return SkNEW_ARGS(DiscardableMemoryPool, (size, mutex));
}

SkDiscardableMemoryPool* SkGetGlobalDiscardableMemoryPool() {
    SK_DECLARE_STATIC_LAZY_PTR(SkDiscardableMemoryPool, global, create_global_pool);
    return global.get();
}


void SkPurgeGlobalDiscardableMemoryPool() {
    SkGetGlobalDiscardableMemoryPool()->dumpPool();
}

