








#ifndef GrResource_DEFINED
#define GrResource_DEFINED

#include "GrRefCnt.h"

#include "SkTDLinkedList.h"

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

protected:
    explicit GrResource(GrGpu* gpu);
    virtual ~GrResource();

    GrGpu* getGpu() const { return fGpu; }

    
    
    virtual void onRelease() {};
    virtual void onAbandon() {};

    bool isInCache() const { return NULL != fCacheEntry; }

private:

#if GR_DEBUG
    friend class GrGpu; 
#endif

    GrGpu*      fGpu;       
                            
                            
                            

    
    SK_DEFINE_DLINKEDLIST_INTERFACE(GrResource);

    GrResourceEntry* fCacheEntry;  

    typedef GrRefCnt INHERITED;
};

#endif
