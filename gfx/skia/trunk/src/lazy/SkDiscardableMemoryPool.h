






#ifndef SkDiscardableMemoryPool_DEFINED
#define SkDiscardableMemoryPool_DEFINED

#include "SkDiscardableMemory.h"
#include "SkTInternalLList.h"
#include "SkThread.h"

class SkPoolDiscardableMemory;

#ifdef SK_DEBUG
    #define LAZY_CACHE_STATS 1
#elif !defined(LAZY_CACHE_STATS)
    #define LAZY_CACHE_STATS 0
#endif





class SkDiscardableMemoryPool : public SkDiscardableMemory::Factory {
public:
    


    SkDiscardableMemoryPool(size_t budget, SkBaseMutex* mutex = NULL);
    virtual ~SkDiscardableMemoryPool();

    virtual SkDiscardableMemory* create(size_t bytes) SK_OVERRIDE;

    size_t getRAMUsed();
    void setRAMBudget(size_t budget);

    
    void dumpPool();

    #if LAZY_CACHE_STATS
    int          fCacheHits;
    int          fCacheMisses;
    #endif  

private:
    SkBaseMutex* fMutex;
    size_t       fBudget;
    size_t       fUsed;
    SkTInternalLList<SkPoolDiscardableMemory> fList;

    
    void dumpDownTo(size_t budget);
    
    void free(SkPoolDiscardableMemory* dm);
    
    bool lock(SkPoolDiscardableMemory* dm);
    
    void unlock(SkPoolDiscardableMemory* dm);

    friend class SkPoolDiscardableMemory;

    typedef SkDiscardableMemory::Factory INHERITED;
};





SkDiscardableMemoryPool* SkGetGlobalDiscardableMemoryPool();

#if !defined(SK_DEFAULT_GLOBAL_DISCARDABLE_MEMORY_POOL_SIZE)
#define SK_DEFAULT_GLOBAL_DISCARDABLE_MEMORY_POOL_SIZE (128 * 1024 * 1024)
#endif

#endif  
