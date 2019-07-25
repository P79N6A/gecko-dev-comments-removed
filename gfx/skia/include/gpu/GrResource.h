








#ifndef GrResource_DEFINED
#define GrResource_DEFINED

#include "GrRefCnt.h"

class GrGpu;

class GrResource : public GrRefCnt {
public:
    explicit GrResource(GrGpu* gpu);

    virtual ~GrResource() {
        
        GrAssert(!isValid());
    }

    



    void release();

    



    void abandon();

    









    bool isValid() const { return NULL != fGpu; }

    





     virtual size_t sizeInBytes() const = 0;

protected:

    virtual void onRelease() = 0;
    virtual void onAbandon() = 0;

    GrGpu* getGpu() const { return fGpu; }

private:
    GrResource(); 

    GrGpu* fGpu; 

    friend class GrGpu; 

    GrResource* fNext;      
    GrResource* fPrevious;

    typedef GrRefCnt INHERITED;
};

#endif
