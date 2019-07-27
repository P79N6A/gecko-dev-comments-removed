








#include "GrContext.h"

#include "effects/GrConfigConversionEffect.h"
#include "effects/GrDashingEffect.h"
#include "effects/GrSingleTextureEffect.h"

#include "GrAARectRenderer.h"
#include "GrBufferAllocPool.h"
#include "GrGpu.h"
#include "GrDistanceFieldTextContext.h"
#include "GrDrawTargetCaps.h"
#include "GrIndexBuffer.h"
#include "GrInOrderDrawBuffer.h"
#include "GrLayerCache.h"
#include "GrOvalRenderer.h"
#include "GrPathRenderer.h"
#include "GrPathUtils.h"
#include "GrResourceCache.h"
#include "GrSoftwarePathRenderer.h"
#include "GrStencilBuffer.h"
#include "GrStencilAndCoverTextContext.h"
#include "GrStrokeInfo.h"
#include "GrTextStrike.h"
#include "GrTraceMarker.h"
#include "GrTracing.h"
#include "SkDashPathPriv.h"
#include "SkGr.h"
#include "SkRTConf.h"
#include "SkRRect.h"
#include "SkStrokeRec.h"
#include "SkTLazy.h"
#include "SkTLS.h"
#include "SkTraceEvent.h"




SK_CONF_DECLARE(bool, c_Defer, "gpu.deferContext", true,
                "Defers rendering in GrContext via GrInOrderDrawBuffer.");

#define BUFFERED_DRAW (c_Defer ? kYes_BufferedDraw : kNo_BufferedDraw)

#ifdef SK_DEBUG
    
    #define GR_DEBUG_PARTIAL_COVERAGE_CHECK 0
#else
    #define GR_DEBUG_PARTIAL_COVERAGE_CHECK 0
#endif

static const size_t MAX_RESOURCE_CACHE_COUNT = GR_DEFAULT_RESOURCE_CACHE_COUNT_LIMIT;
static const size_t MAX_RESOURCE_CACHE_BYTES = GR_DEFAULT_RESOURCE_CACHE_MB_LIMIT * 1024 * 1024;

static const size_t DRAW_BUFFER_VBPOOL_BUFFER_SIZE = 1 << 15;
static const int DRAW_BUFFER_VBPOOL_PREALLOC_BUFFERS = 4;

static const size_t DRAW_BUFFER_IBPOOL_BUFFER_SIZE = 1 << 11;
static const int DRAW_BUFFER_IBPOOL_PREALLOC_BUFFERS = 4;

#define ASSERT_OWNED_RESOURCE(R) SkASSERT(!(R) || (R)->getContext() == this)


class GrContext::AutoRestoreEffects : public GrDrawState::AutoRestoreEffects {};

class GrContext::AutoCheckFlush {
public:
    AutoCheckFlush(GrContext* context) : fContext(context) { SkASSERT(NULL != context); }

    ~AutoCheckFlush() {
        if (fContext->fFlushToReduceCacheSize) {
            fContext->flush();
        }
    }

private:
    GrContext* fContext;
};

GrContext* GrContext::Create(GrBackend backend, GrBackendContext backendContext) {
    GrContext* context = SkNEW(GrContext);
    if (context->init(backend, backendContext)) {
        return context;
    } else {
        context->unref();
        return NULL;
    }
}

GrContext::GrContext() {
    fDrawState = NULL;
    fGpu = NULL;
    fClip = NULL;
    fPathRendererChain = NULL;
    fSoftwarePathRenderer = NULL;
    fResourceCache = NULL;
    fFontCache = NULL;
    fDrawBuffer = NULL;
    fDrawBufferVBAllocPool = NULL;
    fDrawBufferIBAllocPool = NULL;
    fFlushToReduceCacheSize = false;
    fAARectRenderer = NULL;
    fOvalRenderer = NULL;
    fViewMatrix.reset();
    fMaxTextureSizeOverride = 1 << 20;
    fGpuTracingEnabled = false;
}

bool GrContext::init(GrBackend backend, GrBackendContext backendContext) {
    SkASSERT(NULL == fGpu);

    fGpu = GrGpu::Create(backend, backendContext, this);
    if (NULL == fGpu) {
        return false;
    }

    fDrawState = SkNEW(GrDrawState);
    fGpu->setDrawState(fDrawState);

    fResourceCache = SkNEW_ARGS(GrResourceCache, (MAX_RESOURCE_CACHE_COUNT,
                                                  MAX_RESOURCE_CACHE_BYTES));
    fResourceCache->setOverbudgetCallback(OverbudgetCB, this);

    fFontCache = SkNEW_ARGS(GrFontCache, (fGpu));

    fLayerCache.reset(SkNEW_ARGS(GrLayerCache, (this)));

    fLastDrawWasBuffered = kNo_BufferedDraw;

    fAARectRenderer = SkNEW(GrAARectRenderer);
    fOvalRenderer = SkNEW(GrOvalRenderer);

    fDidTestPMConversions = false;

    this->setupDrawBuffer();

    return true;
}

GrContext::~GrContext() {
    if (NULL == fGpu) {
        return;
    }

    this->flush();

    for (int i = 0; i < fCleanUpData.count(); ++i) {
        (*fCleanUpData[i].fFunc)(this, fCleanUpData[i].fInfo);
    }

    
    
    fGpu->purgeResources();

    delete fResourceCache;
    fResourceCache = NULL;
    delete fFontCache;
    delete fDrawBuffer;
    delete fDrawBufferVBAllocPool;
    delete fDrawBufferIBAllocPool;

    fAARectRenderer->unref();
    fOvalRenderer->unref();

    fGpu->unref();
    SkSafeUnref(fPathRendererChain);
    SkSafeUnref(fSoftwarePathRenderer);
    fDrawState->unref();
}

void GrContext::contextLost() {
    this->contextDestroyed();
    this->setupDrawBuffer();
}

void GrContext::contextDestroyed() {
    
    
    fGpu->abandonResources();

    
    
    SkSafeSetNull(fPathRendererChain);
    SkSafeSetNull(fSoftwarePathRenderer);

    delete fDrawBuffer;
    fDrawBuffer = NULL;

    delete fDrawBufferVBAllocPool;
    fDrawBufferVBAllocPool = NULL;

    delete fDrawBufferIBAllocPool;
    fDrawBufferIBAllocPool = NULL;

    fAARectRenderer->reset();
    fOvalRenderer->reset();

    fResourceCache->purgeAllUnlocked();

    fFontCache->freeAll();
    fLayerCache->freeAll();
    fGpu->markContextDirty();
}

void GrContext::resetContext(uint32_t state) {
    fGpu->markContextDirty(state);
}

void GrContext::freeGpuResources() {
    this->flush();

    fGpu->purgeResources();

    fAARectRenderer->reset();
    fOvalRenderer->reset();

    fResourceCache->purgeAllUnlocked();
    fFontCache->freeAll();
    fLayerCache->freeAll();
    
    SkSafeSetNull(fPathRendererChain);
    SkSafeSetNull(fSoftwarePathRenderer);
}

void GrContext::getResourceCacheUsage(int* resourceCount, size_t* resourceBytes) const {
  if (NULL != resourceCount) {
    *resourceCount = fResourceCache->getCachedResourceCount();
  }
  if (NULL != resourceBytes) {
    *resourceBytes = fResourceCache->getCachedResourceBytes();
  }
}

GrTextContext* GrContext::createTextContext(GrRenderTarget* renderTarget,
                                            const SkDeviceProperties&
                                            leakyProperties,
                                            bool enableDistanceFieldFonts) {
    if (fGpu->caps()->pathRenderingSupport()) {
        if (renderTarget->getStencilBuffer() && renderTarget->isMultisampled()) {
            return SkNEW_ARGS(GrStencilAndCoverTextContext, (this, leakyProperties));
        }
    }
    return SkNEW_ARGS(GrDistanceFieldTextContext, (this, leakyProperties,
                                                   enableDistanceFieldFonts));
}



GrTexture* GrContext::findAndRefTexture(const GrTextureDesc& desc,
                                        const GrCacheID& cacheID,
                                        const GrTextureParams* params) {
    GrResourceKey resourceKey = GrTextureImpl::ComputeKey(fGpu, params, desc, cacheID);
    GrGpuResource* resource = fResourceCache->find(resourceKey);
    SkSafeRef(resource);
    return static_cast<GrTexture*>(resource);
}

bool GrContext::isTextureInCache(const GrTextureDesc& desc,
                                 const GrCacheID& cacheID,
                                 const GrTextureParams* params) const {
    GrResourceKey resourceKey = GrTextureImpl::ComputeKey(fGpu, params, desc, cacheID);
    return fResourceCache->hasKey(resourceKey);
}

void GrContext::addStencilBuffer(GrStencilBuffer* sb) {
    ASSERT_OWNED_RESOURCE(sb);

    GrResourceKey resourceKey = GrStencilBuffer::ComputeKey(sb->width(),
                                                            sb->height(),
                                                            sb->numSamples());
    fResourceCache->addResource(resourceKey, sb);
}

GrStencilBuffer* GrContext::findStencilBuffer(int width, int height,
                                              int sampleCnt) {
    GrResourceKey resourceKey = GrStencilBuffer::ComputeKey(width,
                                                            height,
                                                            sampleCnt);
    GrGpuResource* resource = fResourceCache->find(resourceKey);
    return static_cast<GrStencilBuffer*>(resource);
}

static void stretch_image(void* dst,
                          int dstW,
                          int dstH,
                          const void* src,
                          int srcW,
                          int srcH,
                          size_t bpp) {
    SkFixed dx = (srcW << 16) / dstW;
    SkFixed dy = (srcH << 16) / dstH;

    SkFixed y = dy >> 1;

    size_t dstXLimit = dstW*bpp;
    for (int j = 0; j < dstH; ++j) {
        SkFixed x = dx >> 1;
        const uint8_t* srcRow = reinterpret_cast<const uint8_t *>(src) + (y>>16)*srcW*bpp;
        uint8_t* dstRow = reinterpret_cast<uint8_t *>(dst) + j*dstW*bpp;
        for (size_t i = 0; i < dstXLimit; i += bpp) {
            memcpy(dstRow + i, srcRow + (x>>16)*bpp, bpp);
            x += dx;
        }
        y += dy;
    }
}

namespace {


extern const GrVertexAttrib gVertexAttribs[] = {
    {kVec2f_GrVertexAttribType, 0,               kPosition_GrVertexAttribBinding},
    {kVec2f_GrVertexAttribType, sizeof(SkPoint), kLocalCoord_GrVertexAttribBinding}
};

};



GrTexture* GrContext::createResizedTexture(const GrTextureDesc& desc,
                                           const GrCacheID& cacheID,
                                           const void* srcData,
                                           size_t rowBytes,
                                           bool filter) {
    SkAutoTUnref<GrTexture> clampedTexture(this->findAndRefTexture(desc, cacheID, NULL));
    if (NULL == clampedTexture) {
        clampedTexture.reset(this->createTexture(NULL, desc, cacheID, srcData, rowBytes));

        if (NULL == clampedTexture) {
            return NULL;
        }
    }

    GrTextureDesc rtDesc = desc;
    rtDesc.fFlags =  rtDesc.fFlags |
                     kRenderTarget_GrTextureFlagBit |
                     kNoStencil_GrTextureFlagBit;
    rtDesc.fWidth  = GrNextPow2(desc.fWidth);
    rtDesc.fHeight = GrNextPow2(desc.fHeight);

    GrTexture* texture = fGpu->createTexture(rtDesc, NULL, 0);

    if (NULL != texture) {
        GrDrawTarget::AutoStateRestore asr(fGpu, GrDrawTarget::kReset_ASRInit);
        GrDrawState* drawState = fGpu->drawState();
        drawState->setRenderTarget(texture->asRenderTarget());

        
        
        
        GrTextureParams params(SkShader::kClamp_TileMode, filter ? GrTextureParams::kBilerp_FilterMode :
                                                                   GrTextureParams::kNone_FilterMode);
        drawState->addColorTextureEffect(clampedTexture, SkMatrix::I(), params);

        drawState->setVertexAttribs<gVertexAttribs>(SK_ARRAY_COUNT(gVertexAttribs));

        GrDrawTarget::AutoReleaseGeometry arg(fGpu, 4, 0);

        if (arg.succeeded()) {
            SkPoint* verts = (SkPoint*) arg.vertices();
            verts[0].setIRectFan(0, 0, texture->width(), texture->height(), 2 * sizeof(SkPoint));
            verts[1].setIRectFan(0, 0, 1, 1, 2 * sizeof(SkPoint));
            fGpu->drawNonIndexed(kTriangleFan_GrPrimitiveType, 0, 4);
        }
    } else {
        
        
        
        

        rtDesc.fFlags = kNone_GrTextureFlags;
        
        rtDesc.fWidth  = GrNextPow2(desc.fWidth);
        rtDesc.fHeight = GrNextPow2(desc.fHeight);

        
        SkASSERT(!GrPixelConfigIsCompressed(desc.fConfig));

        size_t bpp = GrBytesPerPixel(desc.fConfig);
        GrAutoMalloc<128*128*4> stretchedPixels(bpp * rtDesc.fWidth * rtDesc.fHeight);
        stretch_image(stretchedPixels.get(), rtDesc.fWidth, rtDesc.fHeight,
                      srcData, desc.fWidth, desc.fHeight, bpp);

        size_t stretchedRowBytes = rtDesc.fWidth * bpp;

        texture = fGpu->createTexture(rtDesc, stretchedPixels.get(), stretchedRowBytes);
        SkASSERT(NULL != texture);
    }

    return texture;
}

GrTexture* GrContext::createTexture(const GrTextureParams* params,
                                    const GrTextureDesc& desc,
                                    const GrCacheID& cacheID,
                                    const void* srcData,
                                    size_t rowBytes,
                                    GrResourceKey* cacheKey) {
    GrResourceKey resourceKey = GrTextureImpl::ComputeKey(fGpu, params, desc, cacheID);

    GrTexture* texture;
    if (GrTextureImpl::NeedsResizing(resourceKey)) {
        
        SkASSERT(!GrPixelConfigIsCompressed(desc.fConfig));

        texture = this->createResizedTexture(desc, cacheID,
                                             srcData, rowBytes,
                                             GrTextureImpl::NeedsBilerp(resourceKey));
    } else {
        texture = fGpu->createTexture(desc, srcData, rowBytes);
    }

    if (NULL != texture) {
        
        
        fResourceCache->purgeAsNeeded(1, texture->gpuMemorySize());
        fResourceCache->addResource(resourceKey, texture);

        if (NULL != cacheKey) {
            *cacheKey = resourceKey;
        }
    }

    return texture;
}

static GrTexture* create_scratch_texture(GrGpu* gpu,
                                         GrResourceCache* resourceCache,
                                         const GrTextureDesc& desc) {
    GrTexture* texture = gpu->createTexture(desc, NULL, 0);
    if (NULL != texture) {
        GrResourceKey key = GrTextureImpl::ComputeScratchKey(texture->desc());
        
        
        resourceCache->purgeAsNeeded(1, texture->gpuMemorySize());
        
        resourceCache->addResource(key, texture, GrResourceCache::kHide_OwnershipFlag);
    }
    return texture;
}

GrTexture* GrContext::lockAndRefScratchTexture(const GrTextureDesc& inDesc, ScratchTexMatch match) {

    SkASSERT((inDesc.fFlags & kRenderTarget_GrTextureFlagBit) ||
             !(inDesc.fFlags & kNoStencil_GrTextureFlagBit));

    
    SkASSERT(this->isConfigRenderable(kAlpha_8_GrPixelConfig, inDesc.fSampleCnt > 0) ||
             !(inDesc.fFlags & kRenderTarget_GrTextureFlagBit) ||
             (inDesc.fConfig != kAlpha_8_GrPixelConfig));

    if (!fGpu->caps()->reuseScratchTextures() &&
        !(inDesc.fFlags & kRenderTarget_GrTextureFlagBit)) {
        
        return create_scratch_texture(fGpu, fResourceCache, inDesc);
    }

    GrTextureDesc desc = inDesc;

    if (kApprox_ScratchTexMatch == match) {
        
        static const int MIN_SIZE = 16;
        desc.fWidth  = SkTMax(MIN_SIZE, GrNextPow2(desc.fWidth));
        desc.fHeight = SkTMax(MIN_SIZE, GrNextPow2(desc.fHeight));
    }

    GrGpuResource* resource = NULL;
    int origWidth = desc.fWidth;
    int origHeight = desc.fHeight;

    do {
        GrResourceKey key = GrTextureImpl::ComputeScratchKey(desc);
        
        resource = fResourceCache->find(key, GrResourceCache::kHide_OwnershipFlag);
        if (NULL != resource) {
            resource->ref();
            break;
        }
        if (kExact_ScratchTexMatch == match) {
            break;
        }
        

        
        
        
        if (desc.fFlags & kNoStencil_GrTextureFlagBit) {
            desc.fFlags = desc.fFlags & ~kNoStencil_GrTextureFlagBit;
        } else {
            break;
        }

    } while (true);

    if (NULL == resource) {
        desc.fFlags = inDesc.fFlags;
        desc.fWidth = origWidth;
        desc.fHeight = origHeight;
        resource = create_scratch_texture(fGpu, fResourceCache, desc);
    }

    return static_cast<GrTexture*>(resource);
}

void GrContext::addExistingTextureToCache(GrTexture* texture) {

    if (NULL == texture) {
        return;
    }

    
    
    SkASSERT(NULL != texture->getCacheEntry());

    
    
    SkASSERT(texture->unique());

    if (fGpu->caps()->reuseScratchTextures() || NULL != texture->asRenderTarget()) {
        
        
        fResourceCache->makeNonExclusive(texture->getCacheEntry());
        this->purgeCache();
    } else {
        
        
        fResourceCache->makeNonExclusive(texture->getCacheEntry());
        fResourceCache->deleteResource(texture->getCacheEntry());
    }
}

void GrContext::unlockScratchTexture(GrTexture* texture) {
    ASSERT_OWNED_RESOURCE(texture);
    SkASSERT(NULL != texture->getCacheEntry());

    
    
    
    if (texture->getCacheEntry()->key().isScratch()) {
        if (fGpu->caps()->reuseScratchTextures() || NULL != texture->asRenderTarget()) {
            fResourceCache->makeNonExclusive(texture->getCacheEntry());
            this->purgeCache();
        } else if (texture->unique()) {
            
            
            
            fResourceCache->makeNonExclusive(texture->getCacheEntry());
            fResourceCache->deleteResource(texture->getCacheEntry());
        } else {
            
            
            
            
            
            texture->impl()->setFlag((GrTextureFlags) GrTextureImpl::kReturnToCache_FlagBit);
            
            texture->unref();
        }
    }
}

void GrContext::purgeCache() {
    if (NULL != fResourceCache) {
        fResourceCache->purgeAsNeeded();
    }
}

bool GrContext::OverbudgetCB(void* data) {
    SkASSERT(NULL != data);

    GrContext* context = reinterpret_cast<GrContext*>(data);

    
    context->fFlushToReduceCacheSize = true;

    return true;
}


GrTexture* GrContext::createUncachedTexture(const GrTextureDesc& descIn,
                                            void* srcData,
                                            size_t rowBytes) {
    GrTextureDesc descCopy = descIn;
    return fGpu->createTexture(descCopy, srcData, rowBytes);
}

void GrContext::getResourceCacheLimits(int* maxTextures, size_t* maxTextureBytes) const {
    fResourceCache->getLimits(maxTextures, maxTextureBytes);
}

void GrContext::setResourceCacheLimits(int maxTextures, size_t maxTextureBytes) {
    fResourceCache->setLimits(maxTextures, maxTextureBytes);
}

int GrContext::getMaxTextureSize() const {
    return SkTMin(fGpu->caps()->maxTextureSize(), fMaxTextureSizeOverride);
}

int GrContext::getMaxRenderTargetSize() const {
    return fGpu->caps()->maxRenderTargetSize();
}

int GrContext::getMaxSampleCount() const {
    return fGpu->caps()->maxSampleCount();
}



GrTexture* GrContext::wrapBackendTexture(const GrBackendTextureDesc& desc) {
    return fGpu->wrapBackendTexture(desc);
}

GrRenderTarget* GrContext::wrapBackendRenderTarget(const GrBackendRenderTargetDesc& desc) {
    return fGpu->wrapBackendRenderTarget(desc);
}



bool GrContext::supportsIndex8PixelConfig(const GrTextureParams* params,
                                          int width, int height) const {
    const GrDrawTargetCaps* caps = fGpu->caps();
    if (!caps->isConfigTexturable(kIndex_8_GrPixelConfig)) {
        return false;
    }

    bool isPow2 = SkIsPow2(width) && SkIsPow2(height);

    if (!isPow2) {
        bool tiled = NULL != params && params->isTiled();
        if (tiled && !caps->npotTextureTileSupport()) {
            return false;
        }
    }
    return true;
}




void GrContext::clear(const SkIRect* rect,
                      const GrColor color,
                      bool canIgnoreRect,
                      GrRenderTarget* target) {
    AutoRestoreEffects are;
    AutoCheckFlush acf(this);
    GR_CREATE_TRACE_MARKER_CONTEXT("GrContext::clear", this);
    this->prepareToDraw(NULL, BUFFERED_DRAW, &are, &acf)->clear(rect, color,
                                                                canIgnoreRect, target);
}

void GrContext::drawPaint(const GrPaint& origPaint) {
    
    
    SkRect r;
    r.setLTRB(0, 0,
              SkIntToScalar(getRenderTarget()->width()),
              SkIntToScalar(getRenderTarget()->height()));
    SkMatrix inverse;
    SkTCopyOnFirstWrite<GrPaint> paint(origPaint);
    AutoMatrix am;
    GR_CREATE_TRACE_MARKER_CONTEXT("GrContext::drawPaint", this);

    
    
    
    if (!this->getMatrix().hasPerspective()) {
        if (!fViewMatrix.invert(&inverse)) {
            GrPrintf("Could not invert matrix\n");
            return;
        }
        inverse.mapRect(&r);
    } else {
        if (!am.setIdentity(this, paint.writable())) {
            GrPrintf("Could not invert matrix\n");
            return;
        }
    }
    
    if (paint->isAntiAlias()) {
        paint.writable()->setAntiAlias(false);
    }
    this->drawRect(*paint, r);
}

#ifdef SK_DEVELOPER
void GrContext::dumpFontCache() const {
    fFontCache->dump();
}
#endif








static void setStrokeRectStrip(SkPoint verts[10], SkRect rect,
                               SkScalar width) {
    const SkScalar rad = SkScalarHalf(width);
    rect.sort();

    verts[0].set(rect.fLeft + rad, rect.fTop + rad);
    verts[1].set(rect.fLeft - rad, rect.fTop - rad);
    verts[2].set(rect.fRight - rad, rect.fTop + rad);
    verts[3].set(rect.fRight + rad, rect.fTop - rad);
    verts[4].set(rect.fRight - rad, rect.fBottom - rad);
    verts[5].set(rect.fRight + rad, rect.fBottom + rad);
    verts[6].set(rect.fLeft + rad, rect.fBottom - rad);
    verts[7].set(rect.fLeft - rad, rect.fBottom + rad);
    verts[8] = verts[0];
    verts[9] = verts[1];
}

static bool isIRect(const SkRect& r) {
    return SkScalarIsInt(r.fLeft)  && SkScalarIsInt(r.fTop) &&
           SkScalarIsInt(r.fRight) && SkScalarIsInt(r.fBottom);
}

static bool apply_aa_to_rect(GrDrawTarget* target,
                             const SkRect& rect,
                             SkScalar strokeWidth,
                             const SkMatrix& combinedMatrix,
                             SkRect* devBoundRect,
                             bool* useVertexCoverage) {
    
    
    

    
    
    
    
    *useVertexCoverage = false;
    if (!target->getDrawState().canTweakAlphaForCoverage()) {
        if (target->shouldDisableCoverageAAForBlend()) {
#ifdef SK_DEBUG
            
#endif
            return false;
        } else {
            *useVertexCoverage = true;
        }
    }
    const GrDrawState& drawState = target->getDrawState();
    if (drawState.getRenderTarget()->isMultisampled()) {
        return false;
    }

    if (0 == strokeWidth && target->willUseHWAALines()) {
        return false;
    }

#if defined(SHADER_AA_FILL_RECT) || !defined(IGNORE_ROT_AA_RECT_OPT)
    if (strokeWidth >= 0) {
#endif
        if (!combinedMatrix.preservesAxisAlignment()) {
            return false;
        }

#if defined(SHADER_AA_FILL_RECT) || !defined(IGNORE_ROT_AA_RECT_OPT)
    } else {
        if (!combinedMatrix.preservesRightAngles()) {
            return false;
        }
    }
#endif

    combinedMatrix.mapRect(devBoundRect, rect);

    if (strokeWidth < 0) {
        return !isIRect(*devBoundRect);
    } else {
        return true;
    }
}

static inline bool rect_contains_inclusive(const SkRect& rect, const SkPoint& point) {
    return point.fX >= rect.fLeft && point.fX <= rect.fRight &&
           point.fY >= rect.fTop && point.fY <= rect.fBottom;
}

void GrContext::drawRect(const GrPaint& paint,
                         const SkRect& rect,
                         const GrStrokeInfo* strokeInfo,
                         const SkMatrix* matrix) {
    if (NULL != strokeInfo && strokeInfo->isDashed()) {
        SkPath path;
        path.addRect(rect);
        this->drawPath(paint, path, *strokeInfo);
        return;
    }

    AutoRestoreEffects are;
    AutoCheckFlush acf(this);
    GrDrawTarget* target = this->prepareToDraw(&paint, BUFFERED_DRAW, &are, &acf);

    GR_CREATE_TRACE_MARKER("GrContext::drawRect", target);
    SkScalar width = NULL == strokeInfo ? -1 : strokeInfo->getStrokeRec().getWidth();
    SkMatrix combinedMatrix = target->drawState()->getViewMatrix();
    if (NULL != matrix) {
        combinedMatrix.preConcat(*matrix);
    }

    
    
    if (width < 0) {
        SkRect rtRect;
        target->getDrawState().getRenderTarget()->getBoundsRect(&rtRect);
        SkRect clipSpaceRTRect = rtRect;
        bool checkClip = false;
        if (NULL != this->getClip()) {
            checkClip = true;
            clipSpaceRTRect.offset(SkIntToScalar(this->getClip()->fOrigin.fX),
                                   SkIntToScalar(this->getClip()->fOrigin.fY));
        }
        
        if (!checkClip || target->getClip()->fClipStack->quickContains(clipSpaceRTRect)) {
            SkMatrix invM;
            if (!combinedMatrix.invert(&invM)) {
                return;
            }
            
            SkPoint srcSpaceRTQuad[4];
            invM.mapRectToQuad(srcSpaceRTQuad, rtRect);
            if (rect_contains_inclusive(rect, srcSpaceRTQuad[0]) &&
                rect_contains_inclusive(rect, srcSpaceRTQuad[1]) &&
                rect_contains_inclusive(rect, srcSpaceRTQuad[2]) &&
                rect_contains_inclusive(rect, srcSpaceRTQuad[3])) {
                
                GrColor clearColor;
                if (paint.isOpaqueAndConstantColor(&clearColor)) {
                    target->clear(NULL, clearColor, true);
                    return;
                }
            }
        }
    }

    SkRect devBoundRect;
    bool useVertexCoverage;
    bool needAA = paint.isAntiAlias() &&
                  !target->getDrawState().getRenderTarget()->isMultisampled();
    bool doAA = needAA && apply_aa_to_rect(target, rect, width, combinedMatrix, &devBoundRect,
                                           &useVertexCoverage);

    const SkStrokeRec& strokeRec = strokeInfo->getStrokeRec();

    if (doAA) {
        GrDrawState::AutoViewMatrixRestore avmr;
        if (!avmr.setIdentity(target->drawState())) {
            return;
        }
        if (width >= 0) {
            fAARectRenderer->strokeAARect(this->getGpu(), target, rect,
                                          combinedMatrix, devBoundRect,
                                          strokeRec, useVertexCoverage);
        } else {
            
            fAARectRenderer->fillAARect(this->getGpu(), target,
                                        rect, combinedMatrix, devBoundRect,
                                        useVertexCoverage);
        }
        return;
    }

    if (width >= 0) {
        
        
        

        static const int worstCaseVertCount = 10;
        target->drawState()->setDefaultVertexAttribs();
        GrDrawTarget::AutoReleaseGeometry geo(target, worstCaseVertCount, 0);

        if (!geo.succeeded()) {
            GrPrintf("Failed to get space for vertices!\n");
            return;
        }

        GrPrimitiveType primType;
        int vertCount;
        SkPoint* vertex = geo.positions();

        if (width > 0) {
            vertCount = 10;
            primType = kTriangleStrip_GrPrimitiveType;
            setStrokeRectStrip(vertex, rect, width);
        } else {
            
            vertCount = 5;
            primType = kLineStrip_GrPrimitiveType;
            vertex[0].set(rect.fLeft, rect.fTop);
            vertex[1].set(rect.fRight, rect.fTop);
            vertex[2].set(rect.fRight, rect.fBottom);
            vertex[3].set(rect.fLeft, rect.fBottom);
            vertex[4].set(rect.fLeft, rect.fTop);
        }

        GrDrawState::AutoViewMatrixRestore avmr;
        if (NULL != matrix) {
            GrDrawState* drawState = target->drawState();
            avmr.set(drawState, *matrix);
        }

        target->drawNonIndexed(primType, 0, vertCount);
    } else {
        
        target->drawSimpleRect(rect, matrix);
    }
}

void GrContext::drawRectToRect(const GrPaint& paint,
                               const SkRect& dstRect,
                               const SkRect& localRect,
                               const SkMatrix* dstMatrix,
                               const SkMatrix* localMatrix) {
    AutoRestoreEffects are;
    AutoCheckFlush acf(this);
    GrDrawTarget* target = this->prepareToDraw(&paint, BUFFERED_DRAW, &are, &acf);

    GR_CREATE_TRACE_MARKER("GrContext::drawRectToRect", target);

    target->drawRect(dstRect, dstMatrix, &localRect, localMatrix);
}

namespace {

extern const GrVertexAttrib gPosUVColorAttribs[] = {
    {kVec2f_GrVertexAttribType,  0, kPosition_GrVertexAttribBinding },
    {kVec2f_GrVertexAttribType,  sizeof(SkPoint), kLocalCoord_GrVertexAttribBinding },
    {kVec4ub_GrVertexAttribType, 2*sizeof(SkPoint), kColor_GrVertexAttribBinding}
};

extern const GrVertexAttrib gPosColorAttribs[] = {
    {kVec2f_GrVertexAttribType,  0, kPosition_GrVertexAttribBinding},
    {kVec4ub_GrVertexAttribType, sizeof(SkPoint), kColor_GrVertexAttribBinding},
};

static void set_vertex_attributes(GrDrawState* drawState,
                                  const SkPoint* texCoords,
                                  const GrColor* colors,
                                  int* colorOffset,
                                  int* texOffset) {
    *texOffset = -1;
    *colorOffset = -1;

    if (NULL != texCoords && NULL != colors) {
        *texOffset = sizeof(SkPoint);
        *colorOffset = 2*sizeof(SkPoint);
        drawState->setVertexAttribs<gPosUVColorAttribs>(3);
    } else if (NULL != texCoords) {
        *texOffset = sizeof(SkPoint);
        drawState->setVertexAttribs<gPosUVColorAttribs>(2);
    } else if (NULL != colors) {
        *colorOffset = sizeof(SkPoint);
        drawState->setVertexAttribs<gPosColorAttribs>(2);
    } else {
        drawState->setVertexAttribs<gPosColorAttribs>(1);
    }
}

};

void GrContext::drawVertices(const GrPaint& paint,
                             GrPrimitiveType primitiveType,
                             int vertexCount,
                             const SkPoint positions[],
                             const SkPoint texCoords[],
                             const GrColor colors[],
                             const uint16_t indices[],
                             int indexCount) {
    AutoRestoreEffects are;
    AutoCheckFlush acf(this);
    GrDrawTarget::AutoReleaseGeometry geo; 

    GrDrawTarget* target = this->prepareToDraw(&paint, BUFFERED_DRAW, &are, &acf);
    GrDrawState* drawState = target->drawState();

    GR_CREATE_TRACE_MARKER("GrContext::drawVertices", target);

    int colorOffset = -1, texOffset = -1;
    set_vertex_attributes(drawState, texCoords, colors, &colorOffset, &texOffset);

    size_t vertexSize = drawState->getVertexSize();
    if (sizeof(SkPoint) != vertexSize) {
        if (!geo.set(target, vertexCount, 0)) {
            GrPrintf("Failed to get space for vertices!\n");
            return;
        }
        void* curVertex = geo.vertices();

        for (int i = 0; i < vertexCount; ++i) {
            *((SkPoint*)curVertex) = positions[i];

            if (texOffset >= 0) {
                *(SkPoint*)((intptr_t)curVertex + texOffset) = texCoords[i];
            }
            if (colorOffset >= 0) {
                *(GrColor*)((intptr_t)curVertex + colorOffset) = colors[i];
            }
            curVertex = (void*)((intptr_t)curVertex + vertexSize);
        }
    } else {
        target->setVertexSourceToArray(positions, vertexCount);
    }

    
    

    if (NULL != indices) {
        target->setIndexSourceToArray(indices, indexCount);
        target->drawIndexed(primitiveType, 0, 0, vertexCount, indexCount);
        target->resetIndexSource();
    } else {
        target->drawNonIndexed(primitiveType, 0, vertexCount);
    }
}



void GrContext::drawRRect(const GrPaint& paint,
                          const SkRRect& rrect,
                          const GrStrokeInfo& strokeInfo) {
    if (rrect.isEmpty()) {
       return;
    }

    if (strokeInfo.isDashed()) {
        SkPath path;
        path.addRRect(rrect);
        this->drawPath(paint, path, strokeInfo);
        return;
    }

    AutoRestoreEffects are;
    AutoCheckFlush acf(this);
    GrDrawTarget* target = this->prepareToDraw(&paint, BUFFERED_DRAW, &are, &acf);

    GR_CREATE_TRACE_MARKER("GrContext::drawRRect", target);

    const SkStrokeRec& strokeRec = strokeInfo.getStrokeRec();

    if (!fOvalRenderer->drawRRect(target, this, paint.isAntiAlias(), rrect, strokeRec)) {
        SkPath path;
        path.addRRect(rrect);
        this->internalDrawPath(target, paint.isAntiAlias(), path, strokeInfo);
    }
}



void GrContext::drawDRRect(const GrPaint& paint,
                           const SkRRect& outer,
                           const SkRRect& inner) {
    if (outer.isEmpty()) {
       return;
    }

    AutoRestoreEffects are;
    AutoCheckFlush acf(this);
    GrDrawTarget* target = this->prepareToDraw(&paint, BUFFERED_DRAW, &are, &acf);

    GR_CREATE_TRACE_MARKER("GrContext::drawDRRect", target);

    if (!fOvalRenderer->drawDRRect(target, this, paint.isAntiAlias(), outer, inner)) {
        SkPath path;
        path.addRRect(inner);
        path.addRRect(outer);
        path.setFillType(SkPath::kEvenOdd_FillType);

        GrStrokeInfo fillRec(SkStrokeRec::kFill_InitStyle);
        this->internalDrawPath(target, paint.isAntiAlias(), path, fillRec);
    }
}



void GrContext::drawOval(const GrPaint& paint,
                         const SkRect& oval,
                         const GrStrokeInfo& strokeInfo) {
    if (oval.isEmpty()) {
       return;
    }

    if (strokeInfo.isDashed()) {
        SkPath path;
        path.addOval(oval);
        this->drawPath(paint, path, strokeInfo);
        return;
    }

    AutoRestoreEffects are;
    AutoCheckFlush acf(this);
    GrDrawTarget* target = this->prepareToDraw(&paint, BUFFERED_DRAW, &are, &acf);

    GR_CREATE_TRACE_MARKER("GrContext::drawOval", target);

    const SkStrokeRec& strokeRec = strokeInfo.getStrokeRec();


    if (!fOvalRenderer->drawOval(target, this, paint.isAntiAlias(), oval, strokeRec)) {
        SkPath path;
        path.addOval(oval);
        this->internalDrawPath(target, paint.isAntiAlias(), path, strokeInfo);
    }
}


static bool is_nested_rects(GrDrawTarget* target,
                            const SkPath& path,
                            const SkStrokeRec& stroke,
                            SkRect rects[2],
                            bool* useVertexCoverage) {
    SkASSERT(stroke.isFillStyle());

    if (path.isInverseFillType()) {
        return false;
    }

    const GrDrawState& drawState = target->getDrawState();

    
    
    if (!drawState.getViewMatrix().preservesAxisAlignment()) {
        return false;
    }

    *useVertexCoverage = false;
    if (!target->getDrawState().canTweakAlphaForCoverage()) {
        if (target->shouldDisableCoverageAAForBlend()) {
            return false;
        } else {
            *useVertexCoverage = true;
        }
    }

    SkPath::Direction dirs[2];
    if (!path.isNestedRects(rects, dirs)) {
        return false;
    }

    if (SkPath::kWinding_FillType == path.getFillType() && dirs[0] == dirs[1]) {
        
        return false;
    }

    
    
    const SkScalar* outer = rects[0].asScalars();
    const SkScalar* inner = rects[1].asScalars();

    SkScalar margin = SkScalarAbs(outer[0] - inner[0]);
    for (int i = 1; i < 4; ++i) {
        SkScalar temp = SkScalarAbs(outer[i] - inner[i]);
        if (!SkScalarNearlyEqual(margin, temp)) {
            return false;
        }
    }

    return true;
}

void GrContext::drawPath(const GrPaint& paint, const SkPath& path, const GrStrokeInfo& strokeInfo) {

    if (path.isEmpty()) {
       if (path.isInverseFillType()) {
           this->drawPaint(paint);
       }
       return;
    }

    if (strokeInfo.isDashed()) {
        SkPoint pts[2];
        if (path.isLine(pts)) {
            AutoRestoreEffects are;
            AutoCheckFlush acf(this);
            GrDrawTarget* target = this->prepareToDraw(&paint, BUFFERED_DRAW, &are, &acf);
            GrDrawState* drawState = target->drawState();

            SkMatrix origViewMatrix = drawState->getViewMatrix();
            GrDrawState::AutoViewMatrixRestore avmr;
            if (avmr.setIdentity(target->drawState())) {
                if (GrDashingEffect::DrawDashLine(pts, paint, strokeInfo, fGpu, target,
                                                  origViewMatrix)) {
                    return;
                }
            }
        }

        
        const SkPathEffect::DashInfo& info = strokeInfo.getDashInfo();
        SkTLazy<SkPath> effectPath;
        GrStrokeInfo newStrokeInfo(strokeInfo, false);
        SkStrokeRec* stroke = newStrokeInfo.getStrokeRecPtr();
        if (SkDashPath::FilterDashPath(effectPath.init(), path, stroke, NULL, info)) {
            this->drawPath(paint, *effectPath.get(), newStrokeInfo);
            return;
        }

        this->drawPath(paint, path, newStrokeInfo);
        return;
    }

    
    
    
    
    
    AutoRestoreEffects are;
    AutoCheckFlush acf(this);
    GrDrawTarget* target = this->prepareToDraw(&paint, BUFFERED_DRAW, &are, &acf);
    GrDrawState* drawState = target->drawState();

    GR_CREATE_TRACE_MARKER1("GrContext::drawPath", target, "Is Convex", path.isConvex());

    const SkStrokeRec& strokeRec = strokeInfo.getStrokeRec();

    bool useCoverageAA = paint.isAntiAlias() && !drawState->getRenderTarget()->isMultisampled();

    if (useCoverageAA && strokeRec.getWidth() < 0 && !path.isConvex()) {
        
        bool useVertexCoverage;
        SkRect rects[2];

        if (is_nested_rects(target, path, strokeRec, rects, &useVertexCoverage)) {
            SkMatrix origViewMatrix = drawState->getViewMatrix();
            GrDrawState::AutoViewMatrixRestore avmr;
            if (!avmr.setIdentity(target->drawState())) {
                return;
            }

            fAARectRenderer->fillAANestedRects(this->getGpu(), target,
                                               rects,
                                               origViewMatrix,
                                               useVertexCoverage);
            return;
        }
    }

    SkRect ovalRect;
    bool isOval = path.isOval(&ovalRect);

    if (!isOval || path.isInverseFillType()
        || !fOvalRenderer->drawOval(target, this, paint.isAntiAlias(), ovalRect, strokeRec)) {
        this->internalDrawPath(target, paint.isAntiAlias(), path, strokeInfo);
    }
}

void GrContext::internalDrawPath(GrDrawTarget* target, bool useAA, const SkPath& path,
                                 const GrStrokeInfo& strokeInfo) {
    SkASSERT(!path.isEmpty());

    GR_CREATE_TRACE_MARKER("GrContext::internalDrawPath", target);


    
    
    
    
    bool useCoverageAA = useAA &&
        !target->getDrawState().getRenderTarget()->isMultisampled() &&
        !target->shouldDisableCoverageAAForBlend();


    GrPathRendererChain::DrawType type =
        useCoverageAA ? GrPathRendererChain::kColorAntiAlias_DrawType :
                           GrPathRendererChain::kColor_DrawType;

    const SkPath* pathPtr = &path;
    SkTLazy<SkPath> tmpPath;
    SkTCopyOnFirstWrite<SkStrokeRec> stroke(strokeInfo.getStrokeRec());

    
    GrPathRenderer* pr = this->getPathRenderer(*pathPtr, *stroke, target, false, type);

    if (NULL == pr) {
        if (!GrPathRenderer::IsStrokeHairlineOrEquivalent(*stroke, this->getMatrix(), NULL)) {
            
            if (stroke->applyToPath(tmpPath.init(), *pathPtr)) {
                pathPtr = tmpPath.get();
                stroke.writable()->setFillStyle();
                if (pathPtr->isEmpty()) {
                    return;
                }
            }
        }

        
        pr = this->getPathRenderer(*pathPtr, *stroke, target, true, type);
    }

    if (NULL == pr) {
#ifdef SK_DEBUG
        GrPrintf("Unable to find path renderer compatible with path.\n");
#endif
        return;
    }

    pr->drawPath(*pathPtr, *stroke, target, useCoverageAA);
}



void GrContext::flush(int flagsBitfield) {
    if (NULL == fDrawBuffer) {
        return;
    }

    if (kDiscard_FlushBit & flagsBitfield) {
        fDrawBuffer->reset();
    } else {
        fDrawBuffer->flush();
    }
    fFlushToReduceCacheSize = false;
}

bool GrContext::writeTexturePixels(GrTexture* texture,
                                   int left, int top, int width, int height,
                                   GrPixelConfig config, const void* buffer, size_t rowBytes,
                                   uint32_t flags) {
    ASSERT_OWNED_RESOURCE(texture);

    if ((kUnpremul_PixelOpsFlag & flags) || !fGpu->canWriteTexturePixels(texture, config)) {
        if (NULL != texture->asRenderTarget()) {
            return this->writeRenderTargetPixels(texture->asRenderTarget(),
                                                 left, top, width, height,
                                                 config, buffer, rowBytes, flags);
        } else {
            return false;
        }
    }

    if (!(kDontFlush_PixelOpsFlag & flags)) {
        this->flush();
    }

    return fGpu->writeTexturePixels(texture, left, top, width, height,
                                    config, buffer, rowBytes);
}

bool GrContext::readTexturePixels(GrTexture* texture,
                                  int left, int top, int width, int height,
                                  GrPixelConfig config, void* buffer, size_t rowBytes,
                                  uint32_t flags) {
    ASSERT_OWNED_RESOURCE(texture);

    GrRenderTarget* target = texture->asRenderTarget();
    if (NULL != target) {
        return this->readRenderTargetPixels(target,
                                            left, top, width, height,
                                            config, buffer, rowBytes,
                                            flags);
    } else {
        
        

        
        GrAutoScratchTexture ast;
        GrTextureDesc desc;
        desc.fFlags = kRenderTarget_GrTextureFlagBit;
        desc.fWidth = width;
        desc.fHeight = height;
        desc.fConfig = config;
        desc.fOrigin = kTopLeft_GrSurfaceOrigin;
        ast.set(this, desc, kExact_ScratchTexMatch);
        GrTexture* dst = ast.texture();
        if (NULL != dst && NULL != (target = dst->asRenderTarget())) {
            this->copyTexture(texture, target, NULL);
            return this->readRenderTargetPixels(target,
                                                left, top, width, height,
                                                config, buffer, rowBytes,
                                                flags);
        }

        return false;
    }
}

#include "SkConfig8888.h"


static SkColorType toggle_colortype32(SkColorType ct) {
    if (kRGBA_8888_SkColorType == ct) {
        return kBGRA_8888_SkColorType;
    } else {
        SkASSERT(kBGRA_8888_SkColorType == ct);
        return kRGBA_8888_SkColorType;
    }
}

bool GrContext::readRenderTargetPixels(GrRenderTarget* target,
                                       int left, int top, int width, int height,
                                       GrPixelConfig dstConfig, void* buffer, size_t rowBytes,
                                       uint32_t flags) {
    ASSERT_OWNED_RESOURCE(target);

    if (NULL == target) {
        target = fRenderTarget.get();
        if (NULL == target) {
            return false;
        }
    }

    if (!(kDontFlush_PixelOpsFlag & flags)) {
        this->flush();
    }

    

    
    
    bool flipY = fGpu->readPixelsWillPayForYFlip(target, left, top,
                                                 width, height, dstConfig,
                                                 rowBytes);
    
    
    
    
    
    GrPixelConfig readConfig = dstConfig;
    bool swapRAndB = false;
    if (GrPixelConfigSwapRAndB(dstConfig) ==
        fGpu->preferredReadPixelsConfig(dstConfig, target->config())) {
        readConfig = GrPixelConfigSwapRAndB(readConfig);
        swapRAndB = true;
    }

    bool unpremul = SkToBool(kUnpremul_PixelOpsFlag & flags);

    if (unpremul && !GrPixelConfigIs8888(dstConfig)) {
        
        return false;
    }

    
    
    
    
    GrTexture* src = target->asTexture();
    GrAutoScratchTexture ast;
    if (NULL != src && (swapRAndB || unpremul || flipY)) {
        
        
        GrTextureDesc desc;
        desc.fFlags = kRenderTarget_GrTextureFlagBit;
        desc.fWidth = width;
        desc.fHeight = height;
        desc.fConfig = readConfig;
        desc.fOrigin = kTopLeft_GrSurfaceOrigin;

        
        
        
        
        ScratchTexMatch match = kApprox_ScratchTexMatch;
        if (0 == left &&
            0 == top &&
            target->width() == width &&
            target->height() == height &&
            fGpu->fullReadPixelsIsFasterThanPartial()) {
            match = kExact_ScratchTexMatch;
        }
        ast.set(this, desc, match);
        GrTexture* texture = ast.texture();
        if (texture) {
            
            SkMatrix textureMatrix;
            textureMatrix.setTranslate(SK_Scalar1 *left, SK_Scalar1 *top);
            textureMatrix.postIDiv(src->width(), src->height());

            SkAutoTUnref<const GrEffect> effect;
            if (unpremul) {
                effect.reset(this->createPMToUPMEffect(src, swapRAndB, textureMatrix));
                if (NULL != effect) {
                    unpremul = false; 
                }
            }
            
            
            if (NULL != effect || flipY || swapRAndB) {
                if (!effect) {
                    effect.reset(GrConfigConversionEffect::Create(
                                                    src,
                                                    swapRAndB,
                                                    GrConfigConversionEffect::kNone_PMConversion,
                                                    textureMatrix));
                }
                swapRAndB = false; 

                
                
                
                GrDrawTarget::AutoGeometryAndStatePush agasp(fGpu, GrDrawTarget::kReset_ASRInit);
                GrDrawState* drawState = fGpu->drawState();
                SkASSERT(effect);
                drawState->addColorEffect(effect);

                drawState->setRenderTarget(texture->asRenderTarget());
                SkRect rect = SkRect::MakeWH(SkIntToScalar(width), SkIntToScalar(height));
                fGpu->drawSimpleRect(rect, NULL);
                
                left = 0;
                top = 0;
                target = texture->asRenderTarget();
            }
        }
    }
    if (!fGpu->readPixels(target,
                          left, top, width, height,
                          readConfig, buffer, rowBytes)) {
        return false;
    }
    
    if (unpremul || swapRAndB) {
        SkDstPixelInfo dstPI;
        if (!GrPixelConfig2ColorType(dstConfig, &dstPI.fColorType)) {
            return false;
        }
        dstPI.fAlphaType = kUnpremul_SkAlphaType;
        dstPI.fPixels = buffer;
        dstPI.fRowBytes = rowBytes;

        SkSrcPixelInfo srcPI;
        srcPI.fColorType = swapRAndB ? toggle_colortype32(dstPI.fColorType) : dstPI.fColorType;
        srcPI.fAlphaType = kPremul_SkAlphaType;
        srcPI.fPixels = buffer;
        srcPI.fRowBytes = rowBytes;

        return srcPI.convertPixelsTo(&dstPI, width, height);
    }
    return true;
}

void GrContext::resolveRenderTarget(GrRenderTarget* target) {
    SkASSERT(target);
    ASSERT_OWNED_RESOURCE(target);
    
    
    
    this->flush();
    fGpu->resolveRenderTarget(target);
}

void GrContext::discardRenderTarget(GrRenderTarget* target) {
    SkASSERT(target);
    ASSERT_OWNED_RESOURCE(target);
    AutoRestoreEffects are;
    AutoCheckFlush acf(this);
    this->prepareToDraw(NULL, BUFFERED_DRAW, &are, &acf)->discard(target);
}

void GrContext::copyTexture(GrTexture* src, GrRenderTarget* dst, const SkIPoint* topLeft) {
    if (NULL == src || NULL == dst) {
        return;
    }
    ASSERT_OWNED_RESOURCE(src);

    
    
    
    
    this->flush();

    GrDrawTarget::AutoStateRestore asr(fGpu, GrDrawTarget::kReset_ASRInit);
    GrDrawState* drawState = fGpu->drawState();
    drawState->setRenderTarget(dst);
    SkMatrix sampleM;
    sampleM.setIDiv(src->width(), src->height());
    SkIRect srcRect = SkIRect::MakeWH(dst->width(), dst->height());
    if (NULL != topLeft) {
        srcRect.offset(*topLeft);
    }
    SkIRect srcBounds = SkIRect::MakeWH(src->width(), src->height());
    if (!srcRect.intersect(srcBounds)) {
        return;
    }
    sampleM.preTranslate(SkIntToScalar(srcRect.fLeft), SkIntToScalar(srcRect.fTop));
    drawState->addColorTextureEffect(src, sampleM);
    SkRect dstR = SkRect::MakeWH(SkIntToScalar(srcRect.width()), SkIntToScalar(srcRect.height()));
    fGpu->drawSimpleRect(dstR, NULL);
}

bool GrContext::writeRenderTargetPixels(GrRenderTarget* target,
                                        int left, int top, int width, int height,
                                        GrPixelConfig srcConfig,
                                        const void* buffer,
                                        size_t rowBytes,
                                        uint32_t flags) {
    ASSERT_OWNED_RESOURCE(target);

    if (NULL == target) {
        target = fRenderTarget.get();
        if (NULL == target) {
            return false;
        }
    }

    
    

    
    
    

    
    
    

#if !defined(SK_BUILD_FOR_MAC)
    
    
    
    if (NULL != target->asTexture() && !(kUnpremul_PixelOpsFlag & flags) &&
        fGpu->canWriteTexturePixels(target->asTexture(), srcConfig)) {
        return this->writeTexturePixels(target->asTexture(),
                                        left, top, width, height,
                                        srcConfig, buffer, rowBytes, flags);
    }
#endif

    
    
    
    
    bool swapRAndB = false;
    GrPixelConfig writeConfig = srcConfig;
    if (GrPixelConfigSwapRAndB(srcConfig) ==
        fGpu->preferredWritePixelsConfig(srcConfig, target->config())) {
        writeConfig = GrPixelConfigSwapRAndB(srcConfig);
        swapRAndB = true;
    }

    GrTextureDesc desc;
    desc.fWidth = width;
    desc.fHeight = height;
    desc.fConfig = writeConfig;
    GrAutoScratchTexture ast(this, desc);
    GrTexture* texture = ast.texture();
    if (NULL == texture) {
        return false;
    }

    SkAutoTUnref<const GrEffect> effect;
    SkMatrix textureMatrix;
    textureMatrix.setIDiv(texture->width(), texture->height());

    
    SkAutoSTMalloc<128 * 128, uint32_t> tmpPixels(0);

    if (kUnpremul_PixelOpsFlag & flags) {
        if (!GrPixelConfigIs8888(srcConfig)) {
            return false;
        }
        effect.reset(this->createUPMToPMEffect(texture, swapRAndB, textureMatrix));
        
        if (NULL == effect) {
            SkSrcPixelInfo srcPI;
            if (!GrPixelConfig2ColorType(srcConfig, &srcPI.fColorType)) {
                return false;
            }
            srcPI.fAlphaType = kUnpremul_SkAlphaType;
            srcPI.fPixels = buffer;
            srcPI.fRowBytes = rowBytes;

            tmpPixels.reset(width * height);

            SkDstPixelInfo dstPI;
            dstPI.fColorType = srcPI.fColorType;
            dstPI.fAlphaType = kPremul_SkAlphaType;
            dstPI.fPixels = tmpPixels.get();
            dstPI.fRowBytes = 4 * width;

            if (!srcPI.convertPixelsTo(&dstPI, width, height)) {
                return false;
            }

            buffer = tmpPixels.get();
            rowBytes = 4 * width;
        }
    }
    if (NULL == effect) {
        effect.reset(GrConfigConversionEffect::Create(texture,
                                                      swapRAndB,
                                                      GrConfigConversionEffect::kNone_PMConversion,
                                                      textureMatrix));
    }

    if (!this->writeTexturePixels(texture,
                                  0, 0, width, height,
                                  writeConfig, buffer, rowBytes,
                                  flags & ~kUnpremul_PixelOpsFlag)) {
        return false;
    }

    
    
    
    SkMatrix matrix;
    matrix.setTranslate(SkIntToScalar(left), SkIntToScalar(top));
    GrDrawTarget::AutoGeometryAndStatePush agasp(fGpu, GrDrawTarget::kReset_ASRInit, &matrix);
    GrDrawState* drawState = fGpu->drawState();
    SkASSERT(effect);
    drawState->addColorEffect(effect);

    drawState->setRenderTarget(target);

    fGpu->drawSimpleRect(SkRect::MakeWH(SkIntToScalar(width), SkIntToScalar(height)), NULL);
    return true;
}


GrDrawTarget* GrContext::prepareToDraw(const GrPaint* paint,
                                       BufferedDraw buffered,
                                       AutoRestoreEffects* are,
                                       AutoCheckFlush* acf) {
    
    
    SkASSERT(0 == fDrawState->numColorStages() && 0 == fDrawState->numCoverageStages());

    if (kNo_BufferedDraw == buffered && kYes_BufferedDraw == fLastDrawWasBuffered) {
        fDrawBuffer->flush();
        fLastDrawWasBuffered = kNo_BufferedDraw;
    }
    ASSERT_OWNED_RESOURCE(fRenderTarget.get());
    if (NULL != paint) {
        SkASSERT(NULL != are);
        SkASSERT(NULL != acf);
        are->set(fDrawState);
        fDrawState->setFromPaint(*paint, fViewMatrix, fRenderTarget.get());
#if GR_DEBUG_PARTIAL_COVERAGE_CHECK
        if ((paint->hasMask() || 0xff != paint->fCoverage) &&
            !fGpu->canApplyCoverage()) {
            GrPrintf("Partial pixel coverage will be incorrectly blended.\n");
        }
#endif
    } else {
        fDrawState->reset(fViewMatrix);
        fDrawState->setRenderTarget(fRenderTarget.get());
    }
    GrDrawTarget* target;
    if (kYes_BufferedDraw == buffered) {
        fLastDrawWasBuffered = kYes_BufferedDraw;
        target = fDrawBuffer;
    } else {
        SkASSERT(kNo_BufferedDraw == buffered);
        fLastDrawWasBuffered = kNo_BufferedDraw;
        target = fGpu;
    }
    fDrawState->setState(GrDrawState::kClip_StateBit, NULL != fClip &&
                                                     !fClip->fClipStack->isWideOpen());
    target->setClip(fClip);
    SkASSERT(fDrawState == target->drawState());
    return target;
}







GrPathRenderer* GrContext::getPathRenderer(const SkPath& path,
                                           const SkStrokeRec& stroke,
                                           const GrDrawTarget* target,
                                           bool allowSW,
                                           GrPathRendererChain::DrawType drawType,
                                           GrPathRendererChain::StencilSupport* stencilSupport) {

    if (NULL == fPathRendererChain) {
        fPathRendererChain = SkNEW_ARGS(GrPathRendererChain, (this));
    }

    GrPathRenderer* pr = fPathRendererChain->getPathRenderer(path,
                                                             stroke,
                                                             target,
                                                             drawType,
                                                             stencilSupport);

    if (NULL == pr && allowSW) {
        if (NULL == fSoftwarePathRenderer) {
            fSoftwarePathRenderer = SkNEW_ARGS(GrSoftwarePathRenderer, (this));
        }
        pr = fSoftwarePathRenderer;
    }

    return pr;
}


bool GrContext::isConfigRenderable(GrPixelConfig config, bool withMSAA) const {
    return fGpu->caps()->isConfigRenderable(config, withMSAA);
}

int GrContext::getRecommendedSampleCount(GrPixelConfig config,
                                         SkScalar dpi) const {
    if (!this->isConfigRenderable(config, true)) {
        return 0;
    }
    int chosenSampleCount = 0;
    if (fGpu->caps()->pathRenderingSupport()) {
        if (dpi >= 250.0f) {
            chosenSampleCount = 4;
        } else {
            chosenSampleCount = 16;
        }
    }
    return chosenSampleCount <= fGpu->caps()->maxSampleCount() ?
        chosenSampleCount : 0;
}

void GrContext::setupDrawBuffer() {
    SkASSERT(NULL == fDrawBuffer);
    SkASSERT(NULL == fDrawBufferVBAllocPool);
    SkASSERT(NULL == fDrawBufferIBAllocPool);

    fDrawBufferVBAllocPool =
        SkNEW_ARGS(GrVertexBufferAllocPool, (fGpu, false,
                                    DRAW_BUFFER_VBPOOL_BUFFER_SIZE,
                                    DRAW_BUFFER_VBPOOL_PREALLOC_BUFFERS));
    fDrawBufferIBAllocPool =
        SkNEW_ARGS(GrIndexBufferAllocPool, (fGpu, false,
                                   DRAW_BUFFER_IBPOOL_BUFFER_SIZE,
                                   DRAW_BUFFER_IBPOOL_PREALLOC_BUFFERS));

    fDrawBuffer = SkNEW_ARGS(GrInOrderDrawBuffer, (fGpu,
                                                   fDrawBufferVBAllocPool,
                                                   fDrawBufferIBAllocPool));

    fDrawBuffer->setDrawState(fDrawState);
}

GrDrawTarget* GrContext::getTextTarget() {
    return this->prepareToDraw(NULL, BUFFERED_DRAW, NULL, NULL);
}

const GrIndexBuffer* GrContext::getQuadIndexBuffer() const {
    return fGpu->getQuadIndexBuffer();
}

namespace {
void test_pm_conversions(GrContext* ctx, int* pmToUPMValue, int* upmToPMValue) {
    GrConfigConversionEffect::PMConversion pmToUPM;
    GrConfigConversionEffect::PMConversion upmToPM;
    GrConfigConversionEffect::TestForPreservingPMConversions(ctx, &pmToUPM, &upmToPM);
    *pmToUPMValue = pmToUPM;
    *upmToPMValue = upmToPM;
}
}

const GrEffect* GrContext::createPMToUPMEffect(GrTexture* texture,
                                               bool swapRAndB,
                                               const SkMatrix& matrix) {
    if (!fDidTestPMConversions) {
        test_pm_conversions(this, &fPMToUPMConversion, &fUPMToPMConversion);
        fDidTestPMConversions = true;
    }
    GrConfigConversionEffect::PMConversion pmToUPM =
        static_cast<GrConfigConversionEffect::PMConversion>(fPMToUPMConversion);
    if (GrConfigConversionEffect::kNone_PMConversion != pmToUPM) {
        return GrConfigConversionEffect::Create(texture, swapRAndB, pmToUPM, matrix);
    } else {
        return NULL;
    }
}

const GrEffect* GrContext::createUPMToPMEffect(GrTexture* texture,
                                               bool swapRAndB,
                                               const SkMatrix& matrix) {
    if (!fDidTestPMConversions) {
        test_pm_conversions(this, &fPMToUPMConversion, &fUPMToPMConversion);
        fDidTestPMConversions = true;
    }
    GrConfigConversionEffect::PMConversion upmToPM =
        static_cast<GrConfigConversionEffect::PMConversion>(fUPMToPMConversion);
    if (GrConfigConversionEffect::kNone_PMConversion != upmToPM) {
        return GrConfigConversionEffect::Create(texture, swapRAndB, upmToPM, matrix);
    } else {
        return NULL;
    }
}

GrPath* GrContext::createPath(const SkPath& inPath, const SkStrokeRec& stroke) {
    SkASSERT(fGpu->caps()->pathRenderingSupport());

    
    GrResourceKey resourceKey = GrPath::ComputeKey(inPath, stroke);
    GrPath* path = static_cast<GrPath*>(fResourceCache->find(resourceKey));
    if (NULL != path && path->isEqualTo(inPath, stroke)) {
        path->ref();
    } else {
        path = fGpu->createPath(inPath, stroke);
        fResourceCache->purgeAsNeeded(1, path->gpuMemorySize());
        fResourceCache->addResource(resourceKey, path);
    }
    return path;
}

void GrContext::addResourceToCache(const GrResourceKey& resourceKey, GrGpuResource* resource) {
    fResourceCache->purgeAsNeeded(1, resource->gpuMemorySize());
    fResourceCache->addResource(resourceKey, resource);
}

GrGpuResource* GrContext::findAndRefCachedResource(const GrResourceKey& resourceKey) {
    GrGpuResource* resource = fResourceCache->find(resourceKey);
    SkSafeRef(resource);
    return resource;
}

void GrContext::addGpuTraceMarker(const GrGpuTraceMarker* marker) {
    fGpu->addGpuTraceMarker(marker);
    if (NULL != fDrawBuffer) {
        fDrawBuffer->addGpuTraceMarker(marker);
    }
}

void GrContext::removeGpuTraceMarker(const GrGpuTraceMarker* marker) {
    fGpu->removeGpuTraceMarker(marker);
    if (NULL != fDrawBuffer) {
        fDrawBuffer->removeGpuTraceMarker(marker);
    }
}


#if GR_CACHE_STATS
void GrContext::printCacheStats() const {
    fResourceCache->printStats();
}
#endif
