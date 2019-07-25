









#include "GrAtlas.h"
#include "GrGpu.h"
#include "GrRectanizer.h"
#include "GrTextStrike.h"
#include "GrTextStrike_impl.h"
#include "GrRect.h"

GrFontCache::GrFontCache(GrGpu* gpu) : fGpu(gpu) {
    gpu->ref();
    fAtlasMgr = NULL;

    fHead = fTail = NULL;
}

GrFontCache::~GrFontCache() {
    fCache.deleteAll();
    delete fAtlasMgr;
    fGpu->unref();
}

GrTextStrike* GrFontCache::generateStrike(GrFontScaler* scaler,
                                          const Key& key) {
    if (NULL == fAtlasMgr) {
        fAtlasMgr = new GrAtlasMgr(fGpu);
    }
    GrTextStrike* strike = new GrTextStrike(this, scaler->getKey(),
                                            scaler->getMaskFormat(), fAtlasMgr);
    fCache.insert(key, strike);

    if (fHead) {
        fHead->fPrev = strike;
    } else {
        GrAssert(NULL == fTail);
        fTail = strike;
    }
    strike->fPrev = NULL;
    strike->fNext = fHead;
    fHead = strike;

    return strike;
}

void GrFontCache::freeAll() {
    fCache.deleteAll();
    delete fAtlasMgr;
    fAtlasMgr = NULL;
    fHead = NULL;
    fTail = NULL;
}

void GrFontCache::purgeExceptFor(GrTextStrike* preserveStrike) {
    GrTextStrike* strike = fTail;
    while (strike) {
        if (strike == preserveStrike) {
            strike = strike->fPrev;
            continue;
        }
        GrTextStrike* strikeToPurge = strike;
        
        strike = (NULL == strikeToPurge->fAtlas) ? strikeToPurge->fPrev : NULL;
        int index = fCache.slowFindIndex(strikeToPurge);
        GrAssert(index >= 0);
        fCache.removeAt(index, strikeToPurge->fFontScalerKey->getHash());
        this->detachStrikeFromList(strikeToPurge);
        delete strikeToPurge;
    }
}

#if GR_DEBUG
void GrFontCache::validate() const {
    int count = fCache.count();
    if (0 == count) {
        GrAssert(!fHead);
        GrAssert(!fTail);
    } else if (1 == count) {
        GrAssert(fHead == fTail);
    } else {
        GrAssert(fHead != fTail);
    }

    int count2 = 0;
    const GrTextStrike* strike = fHead;
    while (strike) {
        count2 += 1;
        strike = strike->fNext;
    }
    GrAssert(count == count2);

    count2 = 0;
    strike = fTail;
    while (strike) {
        count2 += 1;
        strike = strike->fPrev;
    }
    GrAssert(count == count2);
}
#endif



#if GR_DEBUG
    static int gCounter;
#endif









GrTextStrike::GrTextStrike(GrFontCache* cache, const GrKey* key,
                           GrMaskFormat format,
                           GrAtlasMgr* atlasMgr) : fPool(64) {
    fFontScalerKey = key;
    fFontScalerKey->ref();

    fFontCache = cache;     
    fAtlasMgr = atlasMgr;   
    fAtlas = NULL;

    fMaskFormat = format;

#if GR_DEBUG

    gCounter += 1;
#endif
}

static void FreeGlyph(GrGlyph*& glyph) { glyph->free(); }

GrTextStrike::~GrTextStrike() {
    GrAtlas::FreeLList(fAtlas);
    fFontScalerKey->unref();
    fCache.getArray().visit(FreeGlyph);

#if GR_DEBUG
    gCounter -= 1;

#endif
}

GrGlyph* GrTextStrike::generateGlyph(GrGlyph::PackedID packed,
                                     GrFontScaler* scaler) {
    GrIRect bounds;
    if (!scaler->getPackedGlyphBounds(packed, &bounds)) {
        return NULL;
    }

    GrGlyph* glyph = fPool.alloc();
    glyph->init(packed, bounds);
    fCache.insert(packed, glyph);
    return glyph;
}

bool GrTextStrike::getGlyphAtlas(GrGlyph* glyph, GrFontScaler* scaler) {
#if 0   
    static int gCounter;
    if ((++gCounter % 10) == 0) return false;
#endif

    GrAssert(glyph);
    GrAssert(scaler);
    GrAssert(fCache.contains(glyph));
    if (glyph->fAtlas) {
        return true;
    }

    GrAutoRef ar(scaler);

    int bytesPerPixel = GrMaskFormatBytesPerPixel(fMaskFormat);
    size_t size = glyph->fBounds.area() * bytesPerPixel;
    SkAutoSMalloc<1024> storage(size);
    if (!scaler->getPackedGlyphImage(glyph->fPackedID, glyph->width(),
                                     glyph->height(),
                                     glyph->width() * bytesPerPixel,
                                     storage.get())) {
        return false;
    }

    GrAtlas* atlas = fAtlasMgr->addToAtlas(fAtlas, glyph->width(),
                                           glyph->height(), storage.get(),
                                           fMaskFormat,
                                           &glyph->fAtlasLocation);
    if (NULL == atlas) {
        return false;
    }

    
    glyph->fAtlas = fAtlas = atlas;
    return true;
}


