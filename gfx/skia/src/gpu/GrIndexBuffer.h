









#ifndef GrIndexBuffer_DEFINED
#define GrIndexBuffer_DEFINED

#include "GrGeometryBuffer.h"

class GrIndexBuffer : public GrGeometryBuffer {
public:
    




    int maxQuads() const {
        return this->sizeInBytes() / (sizeof(uint16_t) * 6);
    }
protected:
    GrIndexBuffer(GrGpu* gpu, bool isWrapped, size_t sizeInBytes, bool dynamic, bool cpuBacked)
        : INHERITED(gpu, isWrapped, sizeInBytes, dynamic, cpuBacked) {}
private:
    typedef GrGeometryBuffer INHERITED;
};

#endif
