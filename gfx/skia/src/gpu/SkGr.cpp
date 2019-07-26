









#include "SkGr.h"












static void build_compressed_data(void* buffer, const SkBitmap& bitmap) {
    SkASSERT(SkBitmap::kIndex8_Config == bitmap.config());

    SkAutoLockPixels apl(bitmap);
    if (!bitmap.readyToDraw()) {
        SkDEBUGFAIL("bitmap not ready to draw!");
        return;
    }

    SkColorTable* ctable = bitmap.getColorTable();
    char* dst = (char*)buffer;

    memcpy(dst, ctable->lockColors(), ctable->count() * sizeof(SkPMColor));
    ctable->unlockColors(false);

    
    dst += kGrColorTableSize;

    if ((unsigned)bitmap.width() == bitmap.rowBytes()) {
        memcpy(dst, bitmap.getPixels(), bitmap.getSize());
    } else {
        
        size_t width = bitmap.width();
        size_t rowBytes = bitmap.rowBytes();
        const char* src = (const char*)bitmap.getPixels();
        for (int y = 0; y < bitmap.height(); y++) {
            memcpy(dst, src, width);
            src += rowBytes;
            dst += width;
        }
    }
}



static void generate_bitmap_cache_id(const SkBitmap& bitmap, GrCacheID* id) {
    
    
    uint32_t genID = bitmap.getGenerationID();
    size_t offset = bitmap.pixelRefOffset();
    int16_t width = static_cast<int16_t>(bitmap.width());
    int16_t height = static_cast<int16_t>(bitmap.height());

    GrCacheID::Key key;
    memcpy(key.fData8, &genID, 4);
    memcpy(key.fData8 + 4, &width, 2);
    memcpy(key.fData8 + 6, &height, 2);
    memcpy(key.fData8 + 8, &offset, sizeof(size_t));
    static const size_t kKeyDataSize = 8 + sizeof(size_t);
    memset(key.fData8 + kKeyDataSize, 0, sizeof(key) - kKeyDataSize);
    GR_STATIC_ASSERT(sizeof(key) >= 8 + sizeof(size_t));
    static const GrCacheID::Domain gBitmapTextureDomain = GrCacheID::GenerateDomain();
    id->reset(gBitmapTextureDomain, key);
}

static void generate_bitmap_texture_desc(const SkBitmap& bitmap, GrTextureDesc* desc) {
    desc->fFlags = kNone_GrTextureFlags;
    desc->fWidth = bitmap.width();
    desc->fHeight = bitmap.height();
    desc->fConfig = SkBitmapConfig2GrPixelConfig(bitmap.config());
    desc->fSampleCnt = 0;
}

static GrTexture* sk_gr_create_bitmap_texture(GrContext* ctx,
                                              bool cache,
                                              const GrTextureParams* params,
                                              const SkBitmap& origBitmap) {
    SkAutoLockPixels alp(origBitmap);

    if (!origBitmap.readyToDraw()) {
        return NULL;
    }

    SkBitmap tmpBitmap;

    const SkBitmap* bitmap = &origBitmap;

    GrTextureDesc desc;
    generate_bitmap_texture_desc(*bitmap, &desc);

    if (SkBitmap::kIndex8_Config == bitmap->config()) {
        
        
        if (ctx->supportsIndex8PixelConfig(params,
                                           bitmap->width(), bitmap->height())) {
            size_t imagesize = bitmap->width() * bitmap->height() +
                                kGrColorTableSize;
            SkAutoMalloc storage(imagesize);

            build_compressed_data(storage.get(), origBitmap);

            
            

            if (cache) {
                GrCacheID cacheID;
                generate_bitmap_cache_id(origBitmap, &cacheID);
                return ctx->createTexture(params, desc, cacheID, storage.get(), bitmap->width());
            } else {
                GrTexture* result = ctx->lockAndRefScratchTexture(desc,
                                                            GrContext::kExact_ScratchTexMatch);
                result->writePixels(0, 0, bitmap->width(),
                                    bitmap->height(), desc.fConfig,
                                    storage.get());
                return result;
            }
        } else {
            origBitmap.copyTo(&tmpBitmap, SkBitmap::kARGB_8888_Config);
            
            bitmap = &tmpBitmap;
            desc.fConfig = SkBitmapConfig2GrPixelConfig(bitmap->config());
        }
    }

    if (cache) {
        
        GrCacheID cacheID;
        generate_bitmap_cache_id(origBitmap, &cacheID);
        return ctx->createTexture(params, desc, cacheID, bitmap->getPixels(), bitmap->rowBytes());
    } else {
        
        
        
        
        
        GrTexture* result = ctx->lockAndRefScratchTexture(desc, GrContext::kExact_ScratchTexMatch);
        result->writePixels(0, 0,
                            bitmap->width(), bitmap->height(),
                            desc.fConfig,
                            bitmap->getPixels(),
                            bitmap->rowBytes());
        return result;
    }
}

bool GrIsBitmapInCache(const GrContext* ctx,
                       const SkBitmap& bitmap,
                       const GrTextureParams* params) {
    GrCacheID cacheID;
    generate_bitmap_cache_id(bitmap, &cacheID);

    GrTextureDesc desc;
    generate_bitmap_texture_desc(bitmap, &desc);
    return ctx->isTextureInCache(desc, cacheID, params);
}

GrTexture* GrLockAndRefCachedBitmapTexture(GrContext* ctx,
                                           const SkBitmap& bitmap,
                                           const GrTextureParams* params) {
    GrTexture* result = NULL;

    bool cache = !bitmap.isVolatile();

    if (cache) {
        

        GrCacheID cacheID;
        generate_bitmap_cache_id(bitmap, &cacheID);

        GrTextureDesc desc;
        generate_bitmap_texture_desc(bitmap, &desc);

        result = ctx->findAndRefTexture(desc, cacheID, params);
    }
    if (NULL == result) {
        result = sk_gr_create_bitmap_texture(ctx, cache, params, bitmap);
    }
    if (NULL == result) {
        GrPrintf("---- failed to create texture for cache [%d %d]\n",
                    bitmap.width(), bitmap.height());
    }
    return result;
}

void GrUnlockAndUnrefCachedBitmapTexture(GrTexture* texture) {
    GrAssert(NULL != texture->getContext());

    texture->getContext()->unlockScratchTexture(texture);
    texture->unref();
}



GrPixelConfig SkBitmapConfig2GrPixelConfig(SkBitmap::Config config) {
    switch (config) {
        case SkBitmap::kA8_Config:
            return kAlpha_8_GrPixelConfig;
        case SkBitmap::kIndex8_Config:
            return kIndex_8_GrPixelConfig;
        case SkBitmap::kRGB_565_Config:
            return kRGB_565_GrPixelConfig;
        case SkBitmap::kARGB_4444_Config:
            return kRGBA_4444_GrPixelConfig;
        case SkBitmap::kARGB_8888_Config:
            return kSkia8888_GrPixelConfig;
        default:
            
            return kUnknown_GrPixelConfig;
    }
}
