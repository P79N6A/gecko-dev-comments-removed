









#include "SkGr.h"












static void build_compressed_data(void* buffer, const SkBitmap& bitmap) {
    SkASSERT(SkBitmap::kIndex8_Config == bitmap.config());

    SkAutoLockPixels apl(bitmap);
    if (!bitmap.readyToDraw()) {
        SkASSERT(!"bitmap not ready to draw!");
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



GrContext::TextureCacheEntry sk_gr_create_bitmap_texture(GrContext* ctx,
                                                GrContext::TextureKey key,
                                                const GrSamplerState& sampler,
                                                const SkBitmap& origBitmap) {
    SkAutoLockPixels alp(origBitmap);
    GrContext::TextureCacheEntry entry;

    if (!origBitmap.readyToDraw()) {
        return entry;
    }

    SkBitmap tmpBitmap;

    const SkBitmap* bitmap = &origBitmap;

    GrTextureDesc desc = {
        kNone_GrTextureFlags,
        kNone_GrAALevel,
        bitmap->width(),
        bitmap->height(),
        SkGr::Bitmap2PixelConfig(*bitmap)
    };

    if (SkBitmap::kIndex8_Config == bitmap->config()) {
        
        
        if (ctx->supportsIndex8PixelConfig(sampler,
                                           bitmap->width(), bitmap->height())) {
            size_t imagesize = bitmap->width() * bitmap->height() +
                                kGrColorTableSize;
            SkAutoMalloc storage(imagesize);

            build_compressed_data(storage.get(), origBitmap);

            
            
            
            if (gUNCACHED_KEY != key) {
                return ctx->createAndLockTexture(key, sampler, desc, storage.get(),
                                                 bitmap->width());
            } else {
                entry = ctx->lockScratchTexture(desc,
                                        GrContext::kExact_ScratchTexMatch);
                entry.texture()->uploadTextureData(0, 0, bitmap->width(), 
                    bitmap->height(), storage.get(), 0);
                return entry;
            }

        } else {
            origBitmap.copyTo(&tmpBitmap, SkBitmap::kARGB_8888_Config);
            
            bitmap = &tmpBitmap;
        }
    }

    desc.fFormat = SkGr::Bitmap2PixelConfig(*bitmap);
    if (gUNCACHED_KEY != key) {
        return ctx->createAndLockTexture(key, sampler, desc,
                                         bitmap->getPixels(),
                                         bitmap->rowBytes());
    } else {
        entry = ctx->lockScratchTexture(desc,
                                        GrContext::kExact_ScratchTexMatch);
        entry.texture()->uploadTextureData(0, 0, bitmap->width(), 
            bitmap->height(), bitmap->getPixels(), bitmap->rowBytes());
        return entry;
    }
}



void SkGrClipIterator::reset(const SkClipStack& clipStack) {
    fClipStack = &clipStack;
    fIter.reset(clipStack);
    
    
    int lastReplace = 0;
    int curr = 0;
    while (NULL != (fCurr = fIter.next())) {
        if (SkRegion::kReplace_Op == fCurr->fOp) {
            lastReplace = curr;
        }
        ++curr;
    }
    fIter.reset(clipStack);
    for (int i = 0; i < lastReplace+1; ++i) {
        fCurr = fIter.next();
    }
}

GrClipType SkGrClipIterator::getType() const {
    GrAssert(!this->isDone());
    if (NULL == fCurr->fPath) {
        return kRect_ClipType;
    } else {
        return kPath_ClipType;
    }
}

GrSetOp SkGrClipIterator::getOp() const {
    
    
    
    
    GrSetOp skToGrOps[] = {
        kDifference_SetOp,         
        kIntersect_SetOp,          
        kUnion_SetOp,              
        kXor_SetOp,                
        kReverseDifference_SetOp,  
        kIntersect_SetOp           
    };
    GR_STATIC_ASSERT(0 == SkRegion::kDifference_Op);
    GR_STATIC_ASSERT(1 == SkRegion::kIntersect_Op);
    GR_STATIC_ASSERT(2 == SkRegion::kUnion_Op);
    GR_STATIC_ASSERT(3 == SkRegion::kXOR_Op);
    GR_STATIC_ASSERT(4 == SkRegion::kReverseDifference_Op);
    GR_STATIC_ASSERT(5 == SkRegion::kReplace_Op);
    return skToGrOps[fCurr->fOp];
}

GrPathFill SkGrClipIterator::getPathFill() const {
    switch (fCurr->fPath->getFillType()) {
        case SkPath::kWinding_FillType:
            return kWinding_PathFill;
        case SkPath::kEvenOdd_FillType:
            return  kEvenOdd_PathFill;
        case SkPath::kInverseWinding_FillType:
            return kInverseWinding_PathFill;
        case SkPath::kInverseEvenOdd_FillType:
            return kInverseEvenOdd_PathFill;
        default:
            GrCrash("Unsupported path fill in clip.");
            return kWinding_PathFill; 
    }
}



GrPixelConfig SkGr::BitmapConfig2PixelConfig(SkBitmap::Config config,
                                                    bool isOpaque) {
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
            if (isOpaque) {
                return kRGBX_8888_GrPixelConfig;
            } else {
                return kRGBA_8888_GrPixelConfig;
            }
        default:
            return kUnknown_GrPixelConfig;
    }
}

