









#include "SkGrTexturePixelRef.h"
#include "GrContext.h"
#include "GrTexture.h"
#include "SkGr.h"
#include "SkRect.h"



SK_DECLARE_STATIC_MUTEX(gROLockPixelsPixelRefMutex);

SkROLockPixelsPixelRef::SkROLockPixelsPixelRef() : INHERITED(&gROLockPixelsPixelRefMutex) {
}

SkROLockPixelsPixelRef::~SkROLockPixelsPixelRef() {
}

void* SkROLockPixelsPixelRef::onLockPixels(SkColorTable** ctable) {
    if (ctable) {
        *ctable = NULL;
    }
    fBitmap.reset();

    if (!this->onReadPixels(&fBitmap, NULL)) {
        SkDebugf("SkROLockPixelsPixelRef::onLockPixels failed!\n");
        return NULL;
    }
    fBitmap.lockPixels();
    return fBitmap.getPixels();
}

void SkROLockPixelsPixelRef::onUnlockPixels() {
    fBitmap.unlockPixels();
}

bool SkROLockPixelsPixelRef::onLockPixelsAreWritable() const {
    return false;
}



static SkGrTexturePixelRef* copyToTexturePixelRef(GrTexture* texture,
                                                  SkBitmap::Config dstConfig) {
    if (NULL == texture) {
        return NULL;
    }
    GrContext* context = texture->getContext();
    if (NULL == context) {
        return NULL;
    }
    GrTextureDesc desc;

    desc.fWidth  = texture->width();
    desc.fHeight = texture->height();
    desc.fFlags = kRenderTarget_GrTextureFlagBit | kNoStencil_GrTextureFlagBit;
    desc.fConfig = SkGr::BitmapConfig2PixelConfig(dstConfig, false);
    desc.fSampleCnt = 0;

    GrTexture* dst = context->createUncachedTexture(desc, NULL, 0);
    if (NULL == dst) {
        return NULL;
    }

    context->copyTexture(texture, dst->asRenderTarget());
    SkGrTexturePixelRef* pixelRef = new SkGrTexturePixelRef(dst);
    GrSafeUnref(dst);
    return pixelRef;
}



SkGrTexturePixelRef::SkGrTexturePixelRef(GrTexture* tex) {
    fTexture = tex;
    GrSafeRef(tex);
}

SkGrTexturePixelRef::~SkGrTexturePixelRef() {
    GrSafeUnref(fTexture);
}

SkGpuTexture* SkGrTexturePixelRef::getTexture() {
    return (SkGpuTexture*)fTexture;
}

SkPixelRef* SkGrTexturePixelRef::deepCopy(SkBitmap::Config dstConfig) {
    return copyToTexturePixelRef(fTexture, dstConfig);
}

bool SkGrTexturePixelRef::onReadPixels(SkBitmap* dst, const SkIRect* subset) {
    if (NULL != fTexture && fTexture->isValid()) {
        int left, top, width, height;
        if (NULL != subset) {
            left = subset->fLeft;
            width = subset->width();
            top = subset->fTop;
            height = subset->height();
        } else {
            left = 0;
            width = fTexture->width();
            top = 0;
            height = fTexture->height();
        }
        dst->setConfig(SkBitmap::kARGB_8888_Config, width, height);
        dst->allocPixels();
        SkAutoLockPixels al(*dst);
        void* buffer = dst->getPixels();
        return fTexture->readPixels(left, top, width, height,
                                    kSkia8888_PM_GrPixelConfig,
                                    buffer, dst->rowBytes());
    } else {
        return false;
    }
}



SkGrRenderTargetPixelRef::SkGrRenderTargetPixelRef(GrRenderTarget* rt) {
    fRenderTarget = rt;
    GrSafeRef(fRenderTarget);
}

SkGrRenderTargetPixelRef::~SkGrRenderTargetPixelRef() {
    GrSafeUnref(fRenderTarget);
}

SkGpuTexture* SkGrRenderTargetPixelRef::getTexture() { 
    if (NULL != fRenderTarget) {
        return (SkGpuTexture*) fRenderTarget->asTexture();
    }
    return NULL;
}

SkPixelRef* SkGrRenderTargetPixelRef::deepCopy(SkBitmap::Config dstConfig) {
    if (NULL == fRenderTarget) {
        return NULL;
    }
    
    
    
    
    
    
    return copyToTexturePixelRef(fRenderTarget->asTexture(), dstConfig);
}

bool SkGrRenderTargetPixelRef::onReadPixels(SkBitmap* dst, const SkIRect* subset) {
    if (NULL != fRenderTarget && fRenderTarget->isValid()) {
        int left, top, width, height;
        if (NULL != subset) {
            left = subset->fLeft;
            width = subset->width();
            top = subset->fTop;
            height = subset->height();
        } else {
            left = 0;
            width = fRenderTarget->width();
            top = 0;
            height = fRenderTarget->height();
        }
        dst->setConfig(SkBitmap::kARGB_8888_Config, width, height);
        dst->allocPixels();
        SkAutoLockPixels al(*dst);
        void* buffer = dst->getPixels();
        return fRenderTarget->readPixels(left, top, width, height,
                                         kSkia8888_PM_GrPixelConfig,
                                         buffer, dst->rowBytes());
    } else {
        return false;
    }
}

