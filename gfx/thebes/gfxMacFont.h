




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

    
    virtual uint32_t GetSpaceGlyph() override {
        return mSpaceGlyph;
    }

    virtual bool SetupCairoFont(gfxContext *aContext) override;

    
    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               uint32_t aStart, uint32_t aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing,
                               uint16_t aOrientation) override;

    
    
    
    virtual bool ProvidesGlyphWidths() const override {
        return mFontEntry->HasFontTable(TRUETYPE_TAG('s','b','i','x'));
    }

    virtual int32_t GetGlyphWidth(DrawTarget& aDrawTarget,
                                  uint16_t aGID) override;

    virtual already_AddRefed<mozilla::gfx::ScaledFont>
    GetScaledFont(mozilla::gfx::DrawTarget *aTarget) override;

    virtual already_AddRefed<mozilla::gfx::GlyphRenderingOptions>
      GetGlyphRenderingOptions(const TextRunDrawParams* aRunParams = nullptr) override;

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const override;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const override;

    virtual FontType GetType() const override { return FONT_TYPE_MAC; }

protected:
    virtual const Metrics& GetHorizontalMetrics() override {
        return mMetrics;
    }

    
    virtual bool ShapeText(gfxContext     *aContext,
                           const char16_t *aText,
                           uint32_t        aOffset,
                           uint32_t        aLength,
                           int32_t         aScript,
                           bool            aVertical,
                           gfxShapedText  *aShapedText) override;

    void InitMetrics();
    void InitMetricsFromPlatform();

    
    
    gfxFloat GetCharWidth(CFDataRef aCmap, char16_t aUniChar,
                          uint32_t *aGlyphID, gfxFloat aConvFactor);

    
    
    CGFontRef             mCGFont;

    
    
    CTFontRef             mCTFont;

    cairo_font_face_t    *mFontFace;

    nsAutoPtr<gfxFontShaper> mCoreTextShaper;

    Metrics               mMetrics;
    uint32_t              mSpaceGlyph;
};

#endif 
