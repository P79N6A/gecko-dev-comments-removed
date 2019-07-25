








#include "GrTexture.h"

#include "GrContext.h"
#include "GrGpu.h"
#include "GrRenderTarget.h"

bool GrTexture::readPixels(int left, int top, int width, int height,
                           GrPixelConfig config, void* buffer) {
    
    GrContext* context = this->getGpu()->getContext();
    GrAssert(NULL != context);
    return context->readTexturePixels(this,
                                        left, top, 
                                        width, height,
                                        config, buffer);
}

void GrTexture::releaseRenderTarget() {
    if (NULL != fRenderTarget) {
        GrAssert(fRenderTarget->asTexture() == this);
        fRenderTarget->onTextureReleaseRenderTarget();
        fRenderTarget->unref();
        fRenderTarget = NULL;
    }
}

void GrTexture::onAbandon() {
    if (NULL != fRenderTarget) {
        fRenderTarget->abandon();
    }
}

