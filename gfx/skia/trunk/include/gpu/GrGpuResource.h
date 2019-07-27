






#ifndef GrGpuResource_DEFINED
#define GrGpuResource_DEFINED

#include "SkInstCnt.h"
#include "SkTInternalLList.h"

class GrResourceCacheEntry;
class GrGpu;
class GrContext;




class GrGpuResource : public SkNoncopyable {
public:
    SK_DECLARE_INST_COUNT_ROOT(GrGpuResource)

    
    
    
    
    void ref() const { ++fRefCnt; }
    void unref() const { --fRefCnt; if (0 == fRefCnt) { this->internal_dispose(); } }
    virtual void internal_dispose() const { SkDELETE(this); }
    bool unique() const { return 1 == fRefCnt; }
#ifdef SK_DEBUG
    void validate() const {
        SkASSERT(fRefCnt > 0);
    }
#endif

    



    void release();

    



    void abandon();

    









    bool wasDestroyed() const { return NULL == fGpu; }

    





    const GrContext* getContext() const;
    GrContext* getContext();

    






    virtual size_t gpuMemorySize() const = 0;

    void setCacheEntry(GrResourceCacheEntry* cacheEntry) { fCacheEntry = cacheEntry; }
    GrResourceCacheEntry* getCacheEntry() { return fCacheEntry; }

    




    uint32_t getUniqueID() const { return fUniqueID; }

protected:
    GrGpuResource(GrGpu*, bool isWrapped);
    virtual ~GrGpuResource();

    bool isInCache() const { return NULL != fCacheEntry; }

    GrGpu* getGpu() const { return fGpu; }

    
    
    virtual void onRelease() {};
    virtual void onAbandon() {};

    bool isWrapped() const { return kWrapped_FlagBit & fFlags; }

    





    void didChangeGpuMemorySize() const;

private:
#ifdef SK_DEBUG
    friend class GrGpu; 
#endif

    static uint32_t CreateUniqueID();

    
    SK_DECLARE_INTERNAL_LLIST_INTERFACE(GrGpuResource);

    GrGpu*              fGpu;               
                                            
                                            
    enum Flags {
        




        kWrapped_FlagBit         = 0x1,
    };

    uint32_t                fFlags;

    mutable int32_t         fRefCnt;
    GrResourceCacheEntry*   fCacheEntry;  
    const uint32_t          fUniqueID;

    typedef SkNoncopyable INHERITED;
};

#endif
