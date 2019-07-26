







#include "GrStencilBuffer.h"

#include "GrContext.h"
#include "GrGpu.h"
#include "GrResourceCache.h"

SK_DEFINE_INST_COUNT(GrStencilBuffer)
GR_DEFINE_RESOURCE_CACHE_TYPE(GrStencilBuffer)

void GrStencilBuffer::transferToCache() {
    GrAssert(NULL == this->getCacheEntry());

    this->getGpu()->getContext()->addStencilBuffer(this);
}

namespace {


void gen_stencil_key_values(int width,
                            int height,
                            int sampleCnt,
                            GrCacheID* cacheID) {
    cacheID->fPublicID = GrCacheID::kDefaultPublicCacheID;
    cacheID->fResourceSpecific32 = width | (height << 16);
    cacheID->fDomain = GrCacheData::kScratch_ResourceDomain;

    GrAssert(sampleCnt >= 0 && sampleCnt < 256);
    cacheID->fResourceSpecific16 = sampleCnt << 8;

    
}
}

GrResourceKey GrStencilBuffer::ComputeKey(int width,
                                          int height,
                                          int sampleCnt) {
    GrCacheID id(GrStencilBuffer::GetResourceType());
    gen_stencil_key_values(width, height, sampleCnt, &id);

    uint32_t v[4];
    id.toRaw(v);
    return GrResourceKey(v);
}
