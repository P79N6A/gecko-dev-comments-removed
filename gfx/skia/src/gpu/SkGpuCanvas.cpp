









#include "GrContext.h"

#include "SkGpuCanvas.h"
#include "SkGpuDevice.h"



SkGpuCanvas::SkGpuCanvas(GrContext* context, GrRenderTarget* renderTarget) {
    SkASSERT(context);
    fContext = context;
    fContext->ref();

    this->setDevice(new SkGpuDevice(context, renderTarget))->unref();
}

SkGpuCanvas::~SkGpuCanvas() {
    
    this->restoreToCount(1);
    fContext->flush(false);
    fContext->unref();
}



bool SkGpuCanvas::getViewport(SkIPoint* size) const {
    if (size) {
        SkDevice* device = this->getDevice();
        if (device) {
            size->set(device->width(), device->height());
        } else {
            size->set(0, 0);
        }
    }
    return true;
}

