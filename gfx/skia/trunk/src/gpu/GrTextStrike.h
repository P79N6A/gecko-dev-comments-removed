









#ifndef GrTextStrike_DEFINED
#define GrTextStrike_DEFINED

#include "GrAllocPool.h"
#include "GrFontScaler.h"
#include "SkTDynamicHash.h"
#include "GrGlyph.h"
#include "GrDrawTarget.h"
#include "GrAtlas.h"

class GrFontCache;
class GrGpu;
class GrFontPurgeListener;





class GrTextStrike {
public:
    GrTextStrike(GrFontCache*, const GrFontDescKey* fontScalerKey, GrMaskFormat, GrAtlas*);
    ~GrTextStrike();

    const GrFontDescKey* getFontScalerKey() const { return fFontScalerKey; }
    GrFontCache* getFontCache() const { return fFontCache; }
    GrMaskFormat getMaskFormat() const { return fMaskFormat; }
    GrTexture*   getTexture() const { return fAtlas->getTexture(); }

    inline GrGlyph* getGlyph(GrGlyph::PackedID, GrFontScaler*);
    bool addGlyphToAtlas(GrGlyph*, GrFontScaler*);

    
    int countGlyphs() const { return fCache.count(); }

    
    void removePlot(const GrPlot* plot);

    static const GrFontDescKey& GetKey(const GrTextStrike& ts) {
        return *(ts.fFontScalerKey);
    }
    static uint32_t Hash(const GrFontDescKey& key) {
        return key.getHash();
    }

public:
    
    GrTextStrike*   fPrev;
    GrTextStrike*   fNext;

private:
    SkTDynamicHash<GrGlyph, GrGlyph::PackedID> fCache;
    const GrFontDescKey* fFontScalerKey;
    GrTAllocPool<GrGlyph> fPool;

    GrFontCache*    fFontCache;
    GrAtlas*        fAtlas;
    GrMaskFormat    fMaskFormat;
    bool            fUseDistanceField;

    GrAtlas::ClientPlotUsage fPlotUsage;

    GrGlyph* generateGlyph(GrGlyph::PackedID packed, GrFontScaler* scaler);

    friend class GrFontCache;
};

class GrFontCache {
public:
    GrFontCache(GrGpu*);
    ~GrFontCache();

    inline GrTextStrike* getStrike(GrFontScaler*, bool useDistanceField);

    void freeAll();

    
    bool freeUnusedPlot(GrTextStrike* preserveStrike);

    
    int countStrikes() const { return fCache.count(); }
    GrTextStrike* getHeadStrike() const { return fHead; }

    void updateTextures() {
        for (int i = 0; i < kAtlasCount; ++i) {
            if (fAtlases[i]) {
                fAtlases[i]->uploadPlotsToTexture();
            }
        }
    }

#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif

    void dump() const;

    enum AtlasType {
        kA8_AtlasType,   
        k565_AtlasType,  
        k8888_AtlasType, 

        kLast_AtlasType = k8888_AtlasType
    };
    static const int kAtlasCount = kLast_AtlasType + 1;

private:
    friend class GrFontPurgeListener;

    SkTDynamicHash<GrTextStrike, GrFontDescKey> fCache;
    
    GrTextStrike* fHead;
    GrTextStrike* fTail;

    GrGpu*      fGpu;
    GrAtlas*    fAtlases[kAtlasCount];

    GrTextStrike* generateStrike(GrFontScaler*);
    inline void detachStrikeFromList(GrTextStrike*);
    void purgeStrike(GrTextStrike* strike);
};

#endif
