







#include "GrGLPathRange.h"
#include "GrGLPath.h"
#include "GrGpuGL.h"

#define GPUGL static_cast<GrGpuGL*>(this->getGpu())

#define GL_CALL(X) GR_GL_CALL(GPUGL->glInterface(), X)
#define GL_CALL_RET(R, X) GR_GL_CALL_RET(GPUGL->glInterface(), R, X)

GrGLPathRange::GrGLPathRange(GrGpu* gpu, size_t size, const SkStrokeRec& stroke)
    : INHERITED(gpu, size, stroke),
      fNumDefinedPaths(0) {
    GL_CALL_RET(fBasePathID, GenPaths(fSize));
}

GrGLPathRange::~GrGLPathRange() {
    this->release();
}

void GrGLPathRange::initAt(size_t index, const SkPath& skPath) {
    GrGpuGL* gpu = static_cast<GrGpuGL*>(this->getGpu());
    if (NULL == gpu) {
        return;
    }

#ifdef SK_DEBUG
    
    GrGLboolean hasPathAtIndex;
    GL_CALL_RET(hasPathAtIndex, IsPath(fBasePathID + index));
    SkASSERT(GR_GL_FALSE == hasPathAtIndex);
#endif

    GrGLPath::InitPathObject(gpu->glInterface(), fBasePathID + index, skPath, fStroke);

    ++fNumDefinedPaths;
    this->didChangeGpuMemorySize();
}

void GrGLPathRange::onRelease() {
    SkASSERT(NULL != this->getGpu());

    if (0 != fBasePathID && !this->isWrapped()) {
        GL_CALL(DeletePaths(fBasePathID, fSize));
        fBasePathID = 0;
    }

    INHERITED::onRelease();
}

void GrGLPathRange::onAbandon() {
    fBasePathID = 0;

    INHERITED::onAbandon();
}
