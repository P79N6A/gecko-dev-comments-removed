








#ifndef GrGeometryBuffer_DEFINED
#define GrGeometryBuffer_DEFINED

#include "GrResource.h"

class GrGpu;




class GrGeometryBuffer : public GrResource {
public:
    SK_DECLARE_INST_COUNT(GrGeometryBuffer);

    




    bool dynamic() const { return fDynamic; }

    



    bool isCPUBacked() const { return fCPUBacked; }

    












    virtual void* lock() = 0;

    





    virtual void* lockPtr() const = 0;

    




    virtual void unlock() = 0;

    




    virtual bool isLocked() const = 0;

    








    virtual bool updateData(const void* src, size_t srcSizeInBytes) = 0;

    
    virtual size_t sizeInBytes() const { return fSizeInBytes; }

protected:
    GrGeometryBuffer(GrGpu* gpu, bool isWrapped, size_t sizeInBytes, bool dynamic, bool cpuBacked)
        : INHERITED(gpu, isWrapped)
        , fSizeInBytes(sizeInBytes)
        , fDynamic(dynamic)
        , fCPUBacked(cpuBacked) {}

private:
    size_t   fSizeInBytes;
    bool     fDynamic;
    bool     fCPUBacked;

    typedef GrResource INHERITED;
};

#endif
