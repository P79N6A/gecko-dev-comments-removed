









#ifndef GrIndexBuffer_DEFINED
#define GrIndexBuffer_DEFINED

#include "GrGeometryBuffer.h"

class GrIndexBuffer : public GrGeometryBuffer {
public:
    




    int maxQuads() const {
        return static_cast<int>(this->gpuMemorySize() / (sizeof(uint16_t) * 6));
    }
protected:
    GrIndexBuffer(GrGpu* gpu, bool isWrapped, size_t gpuMemorySize, bool dynamic, bool cpuBacked)
        : INHERITED(gpu, isWrapped, gpuMemorySize, dynamic, cpuBacked) {}
private:
    typedef GrGeometryBuffer INHERITED;
};

#endif
