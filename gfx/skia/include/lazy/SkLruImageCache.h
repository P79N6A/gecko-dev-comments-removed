






#ifndef SkLruImageCache_DEFINED
#define SkLruImageCache_DEFINED

#include "SkImageCache.h"
#include "SkThread.h"
#include "SkTInternalLList.h"

class CachedPixels;




class SkLruImageCache : public SkImageCache {

public:
    SkLruImageCache(size_t budget);

    virtual ~SkLruImageCache();

#ifdef SK_DEBUG
    virtual MemoryStatus getMemoryStatus(intptr_t ID) const SK_OVERRIDE;
    virtual void purgeAllUnpinnedCaches() SK_OVERRIDE;
#endif

    







    size_t setImageCacheLimit(size_t newLimit);

    



    size_t getImageCacheUsed() const { return fRamUsed; }

    virtual void* allocAndPinCache(size_t bytes, intptr_t* ID) SK_OVERRIDE;
    virtual void* pinCache(intptr_t ID, SkImageCache::DataStatus*) SK_OVERRIDE;
    virtual void releaseCache(intptr_t ID) SK_OVERRIDE;
    virtual void throwAwayCache(intptr_t ID) SK_OVERRIDE;

private:
    
    SkTInternalLList<CachedPixels> fLRU;
    typedef SkTInternalLList<CachedPixels>::Iter Iter;

#ifdef SK_DEBUG
    
    mutable
#endif
    SkMutex fMutex;
    size_t  fRamBudget;
    size_t  fRamUsed;

    



    CachedPixels* findByID(intptr_t ID) const;

    



    void purgeIfNeeded();

    


    void purgeTilAtOrBelow(size_t limit);

    


    void removePixels(CachedPixels*);
};

#endif 
