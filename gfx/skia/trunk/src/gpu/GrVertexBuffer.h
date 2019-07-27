









#ifndef GrVertexBuffer_DEFINED
#define GrVertexBuffer_DEFINED

#include "GrGeometryBuffer.h"

class GrVertexBuffer : public GrGeometryBuffer {
protected:
    GrVertexBuffer(GrGpu* gpu, bool isWrapped, size_t gpuMemorySize, bool dynamic, bool cpuBacked)
        : INHERITED(gpu, isWrapped, gpuMemorySize, dynamic, cpuBacked) {}
private:
    typedef GrGeometryBuffer INHERITED;
};

#endif
