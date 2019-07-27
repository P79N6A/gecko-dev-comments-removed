






#ifndef SkDiscardableMemoryPool_DEFINED
#define SkDiscardableMemoryPool_DEFINED

#include "SkDiscardableMemory.h"

#ifndef SK_LAZY_CACHE_STATS
    #ifdef SK_DEBUG
        #define SK_LAZY_CACHE_STATS 1
    #else
        #define SK_LAZY_CACHE_STATS 0
    #endif
#endif







class SkDiscardableMemoryPool : public SkDiscardableMemory::Factory {
public:
    virtual ~SkDiscardableMemoryPool() { }

    virtual size_t getRAMUsed() = 0;
    virtual void setRAMBudget(size_t budget) = 0;
    virtual size_t getRAMBudget() = 0;

    
    virtual void dumpPool() = 0;

    #if SK_LAZY_CACHE_STATS
    




    virtual int getCacheHits() = 0;
    virtual int getCacheMisses() = 0;
    virtual void resetCacheHitsAndMisses() = 0;
    #endif

    




    static SkDiscardableMemoryPool* Create(
            size_t size, SkBaseMutex* mutex = NULL);
};





SkDiscardableMemoryPool* SkGetGlobalDiscardableMemoryPool();

#if !defined(SK_DEFAULT_GLOBAL_DISCARDABLE_MEMORY_POOL_SIZE)
#define SK_DEFAULT_GLOBAL_DISCARDABLE_MEMORY_POOL_SIZE (128 * 1024 * 1024)
#endif

#endif  
