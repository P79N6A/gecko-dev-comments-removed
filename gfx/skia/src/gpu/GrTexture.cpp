








#include "GrTexture.h"

#include "GrContext.h"
#include "GrGpu.h"
#include "GrRenderTarget.h"
#include "GrResourceCache.h"

SK_DEFINE_INST_COUNT(GrTexture)
GR_DEFINE_RESOURCE_CACHE_TYPE(GrTexture)





void GrTexture::internal_dispose() const {

    if (this->isSetFlag((GrTextureFlags) kReturnToCache_FlagBit) &&
        NULL != this->INHERITED::getContext()) {
        GrTexture* nonConstThis = const_cast<GrTexture *>(this);
        this->fRefCnt = 1;      

        nonConstThis->resetFlag((GrTextureFlags) kReturnToCache_FlagBit);
        nonConstThis->INHERITED::getContext()->addExistingTextureToCache(nonConstThis);

        
        
        return;
    }

    this->INHERITED::internal_dispose();
}

bool GrTexture::readPixels(int left, int top, int width, int height,
                           GrPixelConfig config, void* buffer,
                           size_t rowBytes, uint32_t pixelOpsFlags) {
    
    GrContext* context = this->getContext();
    if (NULL == context) {
        return false;
    }
    return context->readTexturePixels(this,
                                      left, top, width, height,
                                      config, buffer, rowBytes,
                                      pixelOpsFlags);
}

void GrTexture::writePixels(int left, int top, int width, int height,
                            GrPixelConfig config, const void* buffer,
                            size_t rowBytes, uint32_t pixelOpsFlags) {
    
    GrContext* context = this->getContext();
    if (NULL == context) {
        return;
    }
    context->writeTexturePixels(this,
                                left, top, width, height,
                                config, buffer, rowBytes,
                                pixelOpsFlags);
}

void GrTexture::releaseRenderTarget() {
    if (NULL != fRenderTarget) {
        GrAssert(fRenderTarget->asTexture() == this);
        GrAssert(fDesc.fFlags & kRenderTarget_GrTextureFlagBit);

        fRenderTarget->onTextureReleaseRenderTarget();
        fRenderTarget->unref();
        fRenderTarget = NULL;

        fDesc.fFlags = fDesc.fFlags &
            ~(kRenderTarget_GrTextureFlagBit|kNoStencil_GrTextureFlagBit);
        fDesc.fSampleCnt = 0;
    }
}

void GrTexture::onRelease() {
    GrAssert(!this->isSetFlag((GrTextureFlags) kReturnToCache_FlagBit));
    this->releaseRenderTarget();

    INHERITED::onRelease();
}

void GrTexture::onAbandon() {
    if (NULL != fRenderTarget) {
        fRenderTarget->abandon();
    }

    INHERITED::onAbandon();
}

void GrTexture::validateDesc() const {
    if (NULL != this->asRenderTarget()) {
        
        GrAssert(0 != (fDesc.fFlags & kRenderTarget_GrTextureFlagBit));

        if (NULL != this->asRenderTarget()->getStencilBuffer()) {
            GrAssert(0 != (fDesc.fFlags & kNoStencil_GrTextureFlagBit));
        } else {
            GrAssert(0 == (fDesc.fFlags & kNoStencil_GrTextureFlagBit));
        }

        GrAssert(fDesc.fSampleCnt == this->asRenderTarget()->numSamples());
    } else {
        GrAssert(0 == (fDesc.fFlags & kRenderTarget_GrTextureFlagBit));
        GrAssert(0 == (fDesc.fFlags & kNoStencil_GrTextureFlagBit));
        GrAssert(0 == fDesc.fSampleCnt);
    }
}



enum TextureBits {
    



    kNPOT_TextureBit            = 0x1,
    





    kFilter_TextureBit          = 0x2,
    



    kScratch_TextureBit         = 0x4,
};

namespace {
void gen_texture_key_values(const GrGpu* gpu,
                            const GrTextureParams* params,
                            const GrTextureDesc& desc,
                            const GrCacheData& cacheData,
                            bool scratch,
                            GrCacheID* cacheID) {

    uint64_t clientKey = cacheData.fClientCacheID;

    if (scratch) {
        
        
        GrAssert(GrCacheData::kScratch_CacheID == clientKey);
        clientKey = (desc.fFlags << 8) | ((uint64_t) desc.fConfig << 32);
    }

    cacheID->fPublicID = clientKey;
    cacheID->fDomain = cacheData.fResourceDomain;

    
    
    
    GrAssert(gpu->getCaps().maxTextureSize() <= SK_MaxU16);
    cacheID->fResourceSpecific32 = desc.fWidth | (desc.fHeight << 16);

    GrAssert(desc.fSampleCnt >= 0 && desc.fSampleCnt < 256);
    cacheID->fResourceSpecific16 = desc.fSampleCnt << 8;

    if (!gpu->getCaps().npotTextureTileSupport()) {
        bool isPow2 = GrIsPow2(desc.fWidth) && GrIsPow2(desc.fHeight);

        bool tiled = NULL != params && params->isTiled();

        if (tiled && !isPow2) {
            cacheID->fResourceSpecific16 |= kNPOT_TextureBit;
            if (params->isBilerp()) {
                cacheID->fResourceSpecific16 |= kFilter_TextureBit;
            }
        }
    }

    if (scratch) {
        cacheID->fResourceSpecific16 |= kScratch_TextureBit;
    }
}
}

GrResourceKey GrTexture::ComputeKey(const GrGpu* gpu,
                                    const GrTextureParams* params,
                                    const GrTextureDesc& desc,
                                    const GrCacheData& cacheData,
                                    bool scratch) {
    GrCacheID id(GrTexture::GetResourceType());
    gen_texture_key_values(gpu, params, desc, cacheData, scratch, &id);

    uint32_t v[4];
    id.toRaw(v);
    return GrResourceKey(v);
}

bool GrTexture::NeedsResizing(const GrResourceKey& key) {
    return 0 != (key.getValue32(3) & kNPOT_TextureBit);
}

bool GrTexture::IsScratchTexture(const GrResourceKey& key) {
    return 0 != (key.getValue32(3) & kScratch_TextureBit);
}

bool GrTexture::NeedsFiltering(const GrResourceKey& key) {
    return 0 != (key.getValue32(3) & kFilter_TextureBit);
}
