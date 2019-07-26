






#ifndef GrResource_DEFINED
#define GrResource_DEFINED

#include "SkRefCnt.h"
#include "SkTInternalLList.h"

class GrGpu;
class GrContext;
class GrResourceEntry;




class GrResource : public SkRefCnt {
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

    void incDeferredRefCount() const {
        SkASSERT(fDeferredRefCount >= 0);
        ++fDeferredRefCount;
    }

    void decDeferredRefCount() const {
        SkASSERT(fDeferredRefCount > 0);
        --fDeferredRefCount;
        if (0 == fDeferredRefCount && this->needsDeferredUnref()) {
            SkASSERT(this->getRefCnt() > 1);
            this->unref();
        }
    }

    int getDeferredRefCount() const { return fDeferredRefCount; }

    void setNeedsDeferredUnref() { fFlags |= kDeferredUnref_FlagBit; }

protected:
    




    GrResource(GrGpu* gpu, bool isWrapped);
    virtual ~GrResource();

    GrGpu* getGpu() const { return fGpu; }

    
    
    virtual void onRelease() {};
    virtual void onAbandon() {};

    bool isInCache() const { return NULL != fCacheEntry; }
    bool isWrapped() const { return kWrapped_FlagBit & fFlags; }
    bool needsDeferredUnref() const { return SkToBool(kDeferredUnref_FlagBit & fFlags); }

private:
#ifdef SK_DEBUG
    friend class GrGpu; 
#endif

    
    SK_DECLARE_INTERNAL_LLIST_INTERFACE(GrResource);

    GrGpu*              fGpu;               
                                            
                                            
                                            
    GrResourceEntry*    fCacheEntry;        
    mutable int         fDeferredRefCount;  

    enum Flags {
        




        kWrapped_FlagBit         = 0x1,

        





        kDeferredUnref_FlagBit  = 0x2,
    };
    uint32_t         fFlags;

    typedef SkRefCnt INHERITED;
};

#endif
