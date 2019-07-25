








#ifndef GrResource_DEFINED
#define GrResource_DEFINED

#include "GrRefCnt.h"

class GrGpu;
class GrContext;




class GrResource : public GrRefCnt {
public:
    



    void release();

    



    void abandon();

    









    bool isValid() const { return NULL != fGpu; }

    





     virtual size_t sizeInBytes() const = 0;

     





     const GrContext* getContext() const;
     GrContext* getContext();

protected:
    explicit GrResource(GrGpu* gpu);
    virtual ~GrResource();

    GrGpu* getGpu() const { return fGpu; }

    virtual void onRelease() = 0;
    virtual void onAbandon() = 0;

private:

    friend class GrGpu; 

    GrGpu*      fGpu;       
                            
                            
                            
    GrResource* fNext;      
    GrResource* fPrevious;

    typedef GrRefCnt INHERITED;
};

#endif
