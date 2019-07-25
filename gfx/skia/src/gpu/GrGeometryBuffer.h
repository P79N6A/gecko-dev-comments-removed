








#ifndef GrGeometryBuffer_DEFINED
#define GrGeometryBuffer_DEFINED

#include "GrResource.h"

class GrGpu;




class GrGeometryBuffer : public GrResource {
public:

    




    bool dynamic() const { return fDynamic; }

    








    virtual void* lock() = 0;

    





    virtual void* lockPtr() const = 0;

    




    virtual void unlock() = 0;

    




    virtual bool isLocked() const = 0;

    








    virtual bool updateData(const void* src, size_t srcSizeInBytes) = 0;

    
    virtual size_t sizeInBytes() const { return fSizeInBytes; }

protected:
    GrGeometryBuffer(GrGpu* gpu, size_t sizeInBytes, bool dynamic)
        : INHERITED(gpu)
        , fSizeInBytes(sizeInBytes)
        , fDynamic(dynamic) {}

private:
    size_t   fSizeInBytes;
    bool     fDynamic;

    typedef GrResource INHERITED;
};

#endif
