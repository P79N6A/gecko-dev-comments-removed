









#ifndef GrVertexBuffer_DEFINED
#define GrVertexBuffer_DEFINED

#include "GrGeometryBuffer.h"

class GrVertexBuffer : public GrGeometryBuffer {
protected:
    GrVertexBuffer(GrGpu* gpu, size_t sizeInBytes, bool dynamic)
        : INHERITED(gpu, sizeInBytes, dynamic) {}
private:
    typedef GrGeometryBuffer INHERITED;
};

#endif
