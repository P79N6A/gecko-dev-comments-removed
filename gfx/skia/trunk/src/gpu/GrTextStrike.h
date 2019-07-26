









#ifndef GrTextStrike_DEFINED
#define GrTextStrike_DEFINED

#include "GrAllocPool.h"
#include "GrFontScaler.h"
#include "GrTHashTable.h"
#include "GrPoint.h"
#include "GrGlyph.h"
#include "GrDrawTarget.h"
#include "GrAtlas.h"

class GrFontCache;
class GrGpu;
class GrFontPurgeListener;





class GrTextStrike {
public:
    GrTextStrike(GrFontCache*, const GrKey* fontScalerKey, GrMaskFormat, GrAtlasMgr*);
    ~GrTextStrike();

    const GrKey* getFontScalerKey() const { return fFontScalerKey; }
    GrFontCache* getFontCache() const { return fFontCache; }
    GrMaskFormat getMaskFormat() const { return fMaskFormat; }

    inline GrGlyph* getGlyph(GrGlyph::PackedID, GrFontScaler*);
    bool addGlyphToAtlas(GrGlyph*, GrFontScaler*);

    SkISize getAtlasSize() const { return fAtlas.getSize(); }

    
    int countGlyphs() const { return fCache.getArray().count(); }
    const GrGlyph* glyphAt(int index) const {
        return fCache.getArray()[index];
    }

    
    void removePlot(const GrPlot* plot);

public:
    
    GrTextStrike*   fPrev;
    GrTextStrike*   fNext;

private:
    class Key;
    GrTHashTable<GrGlyph, Key, 7> fCache;
    const GrKey* fFontScalerKey;
    GrTAllocPool<GrGlyph> fPool;

    GrFontCache*    fFontCache;
    GrAtlasMgr*     fAtlasMgr;
    GrMaskFormat    fMaskFormat;
    bool            fUseDistanceField;

    GrAtlas         fAtlas;

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

    
    int countStrikes() const { return fCache.getArray().count(); }
    const GrTextStrike* strikeAt(int index) const {
        return fCache.getArray()[index];
    }
    GrTextStrike* getHeadStrike() const { return fHead; }

#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif

#ifdef SK_DEVELOPER
    void dump() const;
#endif

    enum AtlasType {
        kA8_AtlasType,   
        k565_AtlasType,  
        k8888_AtlasType, 

        kLast_AtlasType = k8888_AtlasType
    };
    static const int kAtlasCount = kLast_AtlasType + 1;

private:
    friend class GrFontPurgeListener;

    class Key;
    GrTHashTable<GrTextStrike, Key, 8> fCache;
    
    GrTextStrike* fHead;
    GrTextStrike* fTail;

    GrGpu*      fGpu;
    GrAtlasMgr* fAtlasMgr[kAtlasCount];

    GrTextStrike* generateStrike(GrFontScaler*, const Key&);
    inline void detachStrikeFromList(GrTextStrike*);
    void purgeStrike(GrTextStrike* strike);
};

#endif
