









#ifndef GrVertexBuffer_DEFINED
#define GrVertexBuffer_DEFINED

#include "GrGeometryBuffer.h"

class GrVertexBuffer : public GrGeometryBuffer {
protected:
    GrVertexBuffer(GrGpu* gpu, bool isWrapped, size_t sizeInBytes, bool dynamic, bool cpuBacked)
        : INHERITED(gpu, isWrapped, sizeInBytes, dynamic, cpuBacked) {}
private:
    typedef GrGeometryBuffer INHERITED;
};

#endif
