







#include "GrStencilBuffer.h"

#include "GrContext.h"
#include "GrGpu.h"
#include "GrResourceCache.h"

SK_DEFINE_INST_COUNT(GrStencilBuffer)

void GrStencilBuffer::transferToCache() {
    GrAssert(NULL == this->getCacheEntry());

    this->getGpu()->getContext()->addStencilBuffer(this);
}

namespace {

void gen_cache_id(int width, int height, int sampleCnt, GrCacheID* cacheID) {
    static const GrCacheID::Domain gStencilBufferDomain = GrCacheID::GenerateDomain();
    GrCacheID::Key key;
    uint32_t* keyData = key.fData32;
    keyData[0] = width;
    keyData[1] = height;
    keyData[2] = sampleCnt;
    memset(keyData + 3, 0, sizeof(key) - 3 * sizeof(uint32_t));
    GR_STATIC_ASSERT(sizeof(key) >= 3 * sizeof(uint32_t));
    cacheID->reset(gStencilBufferDomain, key);
}
}

GrResourceKey GrStencilBuffer::ComputeKey(int width,
                                          int height,
                                          int sampleCnt) {
    
    static const GrResourceKey::ResourceType gStencilBufferResourceType =
        GrResourceKey::GenerateResourceType();
    GrCacheID id;
    gen_cache_id(width, height, sampleCnt, &id);

    
    return GrResourceKey(id, gStencilBufferResourceType, 0);
}
