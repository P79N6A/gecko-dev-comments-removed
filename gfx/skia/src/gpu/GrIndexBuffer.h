









#ifndef GrIndexBuffer_DEFINED
#define GrIndexBuffer_DEFINED

#include "GrGeometryBuffer.h"

class GrIndexBuffer : public GrGeometryBuffer {
public:
        




        int maxQuads() const {
            return this->sizeInBytes() / (sizeof(uint16_t) * 6);
        }
protected:
    GrIndexBuffer(GrGpu* gpu, size_t sizeInBytes, bool dynamic)
        : INHERITED(gpu, sizeInBytes, dynamic) {}
private:
    typedef GrGeometryBuffer INHERITED;
};

#endif
