








#include "GrResource.h"
#include "GrGpu.h"

GrResource::GrResource(GrGpu* gpu) {
    fGpu        = gpu;
    fNext       = NULL;
    fPrevious   = NULL;
    fGpu->insertResource(this);
}

void GrResource::release() {
    if (NULL != fGpu) {
        this->onRelease();
        fGpu->removeResource(this);
        fGpu = NULL;
    }
}

void GrResource::abandon() {
    if (NULL != fGpu) {
        this->onAbandon();
        fGpu->removeResource(this);
        fGpu = NULL;
    }
}
