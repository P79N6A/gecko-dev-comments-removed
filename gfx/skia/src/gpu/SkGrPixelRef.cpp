









#include "SkGrPixelRef.h"
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



static SkGrPixelRef* copyToTexturePixelRef(GrTexture* texture,
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
    desc.fConfig = SkBitmapConfig2GrPixelConfig(dstConfig);

    GrTexture* dst = context->createUncachedTexture(desc, NULL, 0);
    if (NULL == dst) {
        return NULL;
    }

    context->copyTexture(texture, dst->asRenderTarget());

    
#if 0
    
    
    
    
    
    dst->releaseRenderTarget();
#endif

    SkGrPixelRef* pixelRef = SkNEW_ARGS(SkGrPixelRef, (dst));
    GrSafeUnref(dst);
    return pixelRef;
}



SkGrPixelRef::SkGrPixelRef(GrSurface* surface, bool transferCacheLock) {
    
#if 0
    
    
    
    fSurface = surface->asTexture();
#else
    fSurface = NULL;
#endif
    if (NULL == fSurface) {
        fSurface = surface;
    }
    fUnlock = transferCacheLock;
    GrSafeRef(surface);
}

SkGrPixelRef::~SkGrPixelRef() {
    if (fUnlock) {
        GrContext* context = fSurface->getContext();
        GrTexture* texture = fSurface->asTexture();
        if (NULL != context && NULL != texture) {
            context->unlockScratchTexture(texture);
        }
    }
    GrSafeUnref(fSurface);
}

SkGpuTexture* SkGrPixelRef::getTexture() {
    if (NULL != fSurface) {
        return (SkGpuTexture*) fSurface->asTexture();
    }
    return NULL;
}

SkPixelRef* SkGrPixelRef::deepCopy(SkBitmap::Config dstConfig) {
    if (NULL == fSurface) {
        return NULL;
    }

    
    
    
    
    
    
    return copyToTexturePixelRef(fSurface->asTexture(), dstConfig);
}

bool SkGrPixelRef::onReadPixels(SkBitmap* dst, const SkIRect* subset) {
    if (NULL == fSurface || !fSurface->isValid()) {
        return false;
    }

    int left, top, width, height;
    if (NULL != subset) {
        left = subset->fLeft;
        width = subset->width();
        top = subset->fTop;
        height = subset->height();
    } else {
        left = 0;
        width = fSurface->width();
        top = 0;
        height = fSurface->height();
    }
    dst->setConfig(SkBitmap::kARGB_8888_Config, width, height);
    dst->allocPixels();
    SkAutoLockPixels al(*dst);
    void* buffer = dst->getPixels();
    return fSurface->readPixels(left, top, width, height,
                                kSkia8888_PM_GrPixelConfig,
                                buffer, dst->rowBytes());
}

