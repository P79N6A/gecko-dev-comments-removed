









#include "SkGrPixelRef.h"
#include "GrContext.h"
#include "GrTexture.h"
#include "SkGr.h"
#include "SkRect.h"



SK_DECLARE_STATIC_MUTEX(gROLockPixelsPixelRefMutex);

SkROLockPixelsPixelRef::SkROLockPixelsPixelRef(const SkImageInfo& info)
    : INHERITED(info, &gROLockPixelsPixelRefMutex) {}

SkROLockPixelsPixelRef::~SkROLockPixelsPixelRef() {}

bool SkROLockPixelsPixelRef::onNewLockPixels(LockRec* rec) {
    fBitmap.reset();

    if (!this->onReadPixels(&fBitmap, NULL)) {
        SkDebugf("SkROLockPixelsPixelRef::onLockPixels failed!\n");
        return false;
    }
    fBitmap.lockPixels();
    if (NULL == fBitmap.getPixels()) {
        return false;
    }

    rec->fPixels = fBitmap.getPixels();
    rec->fColorTable = NULL;
    rec->fRowBytes = fBitmap.rowBytes();
    return true;
}

void SkROLockPixelsPixelRef::onUnlockPixels() {
    fBitmap.unlockPixels();
}

bool SkROLockPixelsPixelRef::onLockPixelsAreWritable() const {
    return false;
}



static SkGrPixelRef* copyToTexturePixelRef(GrTexture* texture, SkColorType dstCT,
                                           const SkIRect* subset) {
    if (NULL == texture || kUnknown_SkColorType == dstCT) {
        return NULL;
    }
    GrContext* context = texture->getContext();
    if (NULL == context) {
        return NULL;
    }
    GrTextureDesc desc;

    SkIPoint pointStorage;
    SkIPoint* topLeft;
    if (subset != NULL) {
        SkASSERT(SkIRect::MakeWH(texture->width(), texture->height()).contains(*subset));
        
        desc.fWidth = subset->width();
        desc.fHeight = subset->height();
        pointStorage.set(subset->x(), subset->y());
        topLeft = &pointStorage;
    } else {
        desc.fWidth  = texture->width();
        desc.fHeight = texture->height();
        topLeft = NULL;
    }
    desc.fFlags = kRenderTarget_GrTextureFlagBit | kNoStencil_GrTextureFlagBit;
    desc.fConfig = SkImageInfo2GrPixelConfig(dstCT, kPremul_SkAlphaType);

    GrTexture* dst = context->createUncachedTexture(desc, NULL, 0);
    if (NULL == dst) {
        return NULL;
    }

    context->copyTexture(texture, dst->asRenderTarget(), topLeft);

    
#if 0
    
    
    
    
    
    dst->releaseRenderTarget();
#endif

    SkImageInfo info = SkImageInfo::Make(desc.fWidth, desc.fHeight, dstCT, kPremul_SkAlphaType);
    SkGrPixelRef* pixelRef = SkNEW_ARGS(SkGrPixelRef, (info, dst));
    SkSafeUnref(dst);
    return pixelRef;
}



SkGrPixelRef::SkGrPixelRef(const SkImageInfo& info, GrSurface* surface,
                           bool transferCacheLock) : INHERITED(info) {
    
#if 0
    
    
    
    fSurface = surface->asTexture();
#else
    fSurface = NULL;
#endif
    if (NULL == fSurface) {
        fSurface = surface;
    }
    fUnlock = transferCacheLock;
    SkSafeRef(surface);

    if (fSurface) {
        SkASSERT(info.fWidth <= fSurface->width());
        SkASSERT(info.fHeight <= fSurface->height());
    }
}

SkGrPixelRef::~SkGrPixelRef() {
    if (fUnlock) {
        GrContext* context = fSurface->getContext();
        GrTexture* texture = fSurface->asTexture();
        if (NULL != context && NULL != texture) {
            context->unlockScratchTexture(texture);
        }
    }
    SkSafeUnref(fSurface);
}

GrTexture* SkGrPixelRef::getTexture() {
    if (NULL != fSurface) {
        return fSurface->asTexture();
    }
    return NULL;
}

SkPixelRef* SkGrPixelRef::deepCopy(SkColorType dstCT, const SkIRect* subset) {
    if (NULL == fSurface) {
        return NULL;
    }
    
    
    
    
    
    
    
    return copyToTexturePixelRef(fSurface->asTexture(), dstCT, subset);
}

bool SkGrPixelRef::onReadPixels(SkBitmap* dst, const SkIRect* subset) {
    if (NULL == fSurface || fSurface->wasDestroyed()) {
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
        width = this->info().fWidth;
        top = 0;
        height = this->info().fHeight;
    }
    if (!dst->allocPixels(SkImageInfo::MakeN32Premul(width, height))) {
        SkDebugf("SkGrPixelRef::onReadPixels failed to alloc bitmap for result!\n");
        return false;
    }
    SkAutoLockPixels al(*dst);
    void* buffer = dst->getPixels();
    return fSurface->readPixels(left, top, width, height,
                                kSkia8888_GrPixelConfig,
                                buffer, dst->rowBytes());
}
