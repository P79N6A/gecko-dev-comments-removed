








#ifndef GrResource_DEFINED
#define GrResource_DEFINED

#include "GrRefCnt.h"

#include "SkTInternalLList.h"

class GrGpu;
class GrContext;
class GrResourceEntry;




class GrResource : public GrRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrResource)

    



    void release();

    



    void abandon();

    









    bool isValid() const { return NULL != fGpu; }

    





    virtual size_t sizeInBytes() const = 0;

    





    const GrContext* getContext() const;
    GrContext* getContext();

    void setCacheEntry(GrResourceEntry* cacheEntry) { fCacheEntry = cacheEntry; }
    GrResourceEntry* getCacheEntry() { return fCacheEntry; }

    void incDeferredRefCount() const { GrAssert(fDeferredRefCount >= 0); ++fDeferredRefCount; }
    void decDeferredRefCount() const { GrAssert(fDeferredRefCount > 0); --fDeferredRefCount; }

protected:
    




    GrResource(GrGpu* gpu, bool isWrapped);
    virtual ~GrResource();

    GrGpu* getGpu() const { return fGpu; }

    
    
    virtual void onRelease() {};
    virtual void onAbandon() {};

    bool isInCache() const { return NULL != fCacheEntry; }
    bool isWrapped() const { return kWrapped_Flag & fFlags; }

private:
#if GR_DEBUG
    friend class GrGpu; 
#endif

    
    SK_DECLARE_INTERNAL_LLIST_INTERFACE(GrResource);

    GrGpu*              fGpu;               
                                            
                                            
                                            
    GrResourceEntry*    fCacheEntry;        
    mutable int         fDeferredRefCount;  

    enum Flags {
        kWrapped_Flag = 0x1,
    };
    uint32_t         fFlags;

    typedef GrRefCnt INHERITED;
};

#endif
