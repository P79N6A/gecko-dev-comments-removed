








#ifndef GrGeometryBuffer_DEFINED
#define GrGeometryBuffer_DEFINED

#include "GrGpuResource.h"

class GrGpu;




class GrGeometryBuffer : public GrGpuResource {
public:
    SK_DECLARE_INST_COUNT(GrGeometryBuffer);

    




    bool dynamic() const { return fDynamic; }

    



    bool isCPUBacked() const { return fCPUBacked; }

    















     void* map() { return (fMapPtr = this->onMap()); }

    




     void unmap() {
         SkASSERT(NULL != fMapPtr);
         this->onUnmap();
         fMapPtr = NULL;
     }

    





     void* mapPtr() const { return fMapPtr; }

    




     bool isMapped() const { return NULL != fMapPtr; }

    













    bool updateData(const void* src, size_t srcSizeInBytes) {
        SkASSERT(!this->isMapped());
        SkASSERT(srcSizeInBytes <= fGpuMemorySize);
        return this->onUpdateData(src, srcSizeInBytes);
    }

    
    virtual size_t gpuMemorySize() const { return fGpuMemorySize; }

protected:
    GrGeometryBuffer(GrGpu* gpu, bool isWrapped, size_t gpuMemorySize, bool dynamic, bool cpuBacked)
        : INHERITED(gpu, isWrapped)
        , fMapPtr(NULL)
        , fGpuMemorySize(gpuMemorySize)
        , fDynamic(dynamic)
        , fCPUBacked(cpuBacked) {}

private:
    virtual void* onMap() = 0;
    virtual void onUnmap() = 0;
    virtual bool onUpdateData(const void* src, size_t srcSizeInBytes) = 0;

    void*    fMapPtr;
    size_t   fGpuMemorySize;
    bool     fDynamic;
    bool     fCPUBacked;

    typedef GrGpuResource INHERITED;
};

#endif
