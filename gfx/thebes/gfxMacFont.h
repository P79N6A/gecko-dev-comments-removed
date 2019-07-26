




#ifndef GFX_MACFONT_H
#define GFX_MACFONT_H

#include "gfxFont.h"
#include "gfxMacPlatformFontList.h"
#include "mozilla/gfx/2D.h"

#include "cairo.h"

class gfxMacFont : public gfxFont
{
public:
    gfxMacFont(MacOSFontEntry *aFontEntry, const gfxFontStyle *aFontStyle,
               bool aNeedsBold);

    virtual ~gfxMacFont();

    CGFontRef GetCGFontRef() const { return mCGFont; }

    
    virtual const gfxFont::Metrics& GetMetrics() {
        return mMetrics;
    }

    virtual uint32_t GetSpaceGlyph() {
        return mSpaceGlyph;
    }

    virtual bool SetupCairoFont(gfxContext *aContext);

    
    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               uint32_t aStart, uint32_t aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing);

    
    
    virtual hb_blob_t *GetFontTable(uint32_t aTag);

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont> GetScaledFont(mozilla::gfx::DrawTarget *aTarget);

    virtual void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontCacheSizes*   aSizes) const;
    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontCacheSizes*   aSizes) const;

    virtual FontType GetType() const { return FONT_TYPE_MAC; }

protected:
    virtual void CreatePlatformShaper();

    
    virtual bool ShapeText(gfxContext      *aContext,
                           const PRUnichar *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText,
                           bool             aPreferPlatformShaping = false);

    void InitMetrics();
    void InitMetricsFromPlatform();

    
    
    gfxFloat GetCharWidth(CFDataRef aCmap, PRUnichar aUniChar,
                          uint32_t *aGlyphID, gfxFloat aConvFactor);

    static void DestroyBlobFunc(void* aUserData);

    
    
    CGFontRef             mCGFont;

    cairo_font_face_t    *mFontFace;

    Metrics               mMetrics;
    uint32_t              mSpaceGlyph;
};

#endif 
