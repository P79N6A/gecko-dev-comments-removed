









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

    if (bitmap.width() == bitmap.rowBytes()) {
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



static GrTexture* sk_gr_create_bitmap_texture(GrContext* ctx,
                                              uint64_t key,
                                              const GrTextureParams* params,
                                              const SkBitmap& origBitmap) {
    SkAutoLockPixels alp(origBitmap);

    if (!origBitmap.readyToDraw()) {
        return NULL;
    }

    SkBitmap tmpBitmap;

    const SkBitmap* bitmap = &origBitmap;

    GrTextureDesc desc;
    desc.fWidth = bitmap->width();
    desc.fHeight = bitmap->height();
    desc.fConfig = SkBitmapConfig2GrPixelConfig(bitmap->config());

    GrCacheData cacheData(key);

    if (SkBitmap::kIndex8_Config == bitmap->config()) {
        
        
        if (ctx->supportsIndex8PixelConfig(params,
                                           bitmap->width(), bitmap->height())) {
            size_t imagesize = bitmap->width() * bitmap->height() +
                                kGrColorTableSize;
            SkAutoMalloc storage(imagesize);

            build_compressed_data(storage.get(), origBitmap);

            
            

            if (GrCacheData::kScratch_CacheID != key) {
                return ctx->createTexture(params, desc, cacheData,
                                          storage.get(),
                                          bitmap->width());
            } else {
                GrTexture* result = ctx->lockScratchTexture(desc,
                                          GrContext::kExact_ScratchTexMatch);
                result->writePixels(0, 0, bitmap->width(),
                                    bitmap->height(), desc.fConfig,
                                    storage.get());
                return result;
            }

        } else {
            origBitmap.copyTo(&tmpBitmap, SkBitmap::kARGB_8888_Config);
            
            bitmap = &tmpBitmap;
        }
    }

    desc.fConfig = SkBitmapConfig2GrPixelConfig(bitmap->config());
    if (GrCacheData::kScratch_CacheID != key) {
        
        
        return ctx->createTexture(params, desc, cacheData,
                                  bitmap->getPixels(),
                                  bitmap->rowBytes());
    } else {
        
        
        
        
        
        GrTexture* result = ctx->lockScratchTexture(desc,
                                         GrContext::kExact_ScratchTexMatch);
        result->writePixels(0, 0,
                            bitmap->width(), bitmap->height(),
                            desc.fConfig,
                            bitmap->getPixels(),
                            bitmap->rowBytes());
        return result;
    }
}



GrTexture* GrLockCachedBitmapTexture(GrContext* ctx,
                                     const SkBitmap& bitmap,
                                     const GrTextureParams* params) {
    GrTexture* result = NULL;

    if (!bitmap.isVolatile()) {
        
        uint64_t key = bitmap.getGenerationID();
        key |= ((uint64_t) bitmap.pixelRefOffset()) << 32;

        GrTextureDesc desc;
        desc.fWidth = bitmap.width();
        desc.fHeight = bitmap.height();
        desc.fConfig = SkBitmapConfig2GrPixelConfig(bitmap.config());

        GrCacheData cacheData(key);

        result = ctx->findTexture(desc, cacheData, params);
        if (NULL == result) {
            
            result = sk_gr_create_bitmap_texture(ctx, key, params, bitmap);
        }
    } else {
        result = sk_gr_create_bitmap_texture(ctx, GrCacheData::kScratch_CacheID, params, bitmap);
    }
    if (NULL == result) {
        GrPrintf("---- failed to create texture for cache [%d %d]\n",
                    bitmap.width(), bitmap.height());
    }
    return result;
}

void GrUnlockCachedBitmapTexture(GrTexture* texture) {
    GrAssert(NULL != texture->getContext());

    texture->getContext()->unlockScratchTexture(texture);
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
            return kSkia8888_PM_GrPixelConfig;
        default:
            
            return kUnknown_GrPixelConfig;
    }
}

