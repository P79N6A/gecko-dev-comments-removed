




#ifndef GFX_MACFONT_H
#define GFX_MACFONT_H

#include "mozilla/MemoryReporting.h"
#include "gfxFont.h"
#include "cairo.h"
#include <ApplicationServices/ApplicationServices.h>

class MacOSFontEntry;

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

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont> GetScaledFont(mozilla::gfx::DrawTarget *aTarget);

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;

    virtual FontType GetType() const { return FONT_TYPE_MAC; }

protected:
    virtual void CreatePlatformShaper();

    
    virtual bool ShapeText(gfxContext      *aContext,
                           const char16_t *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText,
                           bool             aPreferPlatformShaping = false);

    void InitMetrics();
    void InitMetricsFromPlatform();

    
    
    gfxFloat GetCharWidth(CFDataRef aCmap, char16_t aUniChar,
                          uint32_t *aGlyphID, gfxFloat aConvFactor);

    
    
    CGFontRef             mCGFont;

    cairo_font_face_t    *mFontFace;

    Metrics               mMetrics;
    uint32_t              mSpaceGlyph;
};

#endif 
