




#ifndef GFX_WINDOWSDWRITEFONTS_H
#define GFX_WINDOWSDWRITEFONTS_H

#include "mozilla/MemoryReporting.h"
#include <dwrite.h>

#include "gfxFont.h"
#include "gfxUserFontSet.h"
#include "cairo-win32.h"

#include "nsDataHashtable.h"
#include "nsHashKeys.h"




class gfxDWriteFont : public gfxFont 
{
public:
    gfxDWriteFont(gfxFontEntry *aFontEntry,
                  const gfxFontStyle *aFontStyle,
                  bool aNeedsBold = false,
                  AntialiasOption = kAntialiasDefault);
    ~gfxDWriteFont();

    virtual gfxFont*
    CopyWithAntialiasOption(AntialiasOption anAAOption) MOZ_OVERRIDE;

    virtual uint32_t GetSpaceGlyph() MOZ_OVERRIDE;

    virtual bool SetupCairoFont(gfxContext *aContext) MOZ_OVERRIDE;

    virtual bool AllowSubpixelAA() MOZ_OVERRIDE
    { return mAllowManualShowGlyphs; }

    bool IsValid() const;

    virtual gfxFloat GetAdjustedSize() const MOZ_OVERRIDE {
        return mAdjustedSize;
    }

    IDWriteFontFace *GetFontFace();

    
    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               uint32_t aStart, uint32_t aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing,
                               uint16_t aOrientation) MOZ_OVERRIDE;

    virtual bool ProvidesGlyphWidths() const MOZ_OVERRIDE;

    virtual int32_t GetGlyphWidth(DrawTarget& aDrawTarget,
                                  uint16_t aGID) MOZ_OVERRIDE;

    virtual mozilla::TemporaryRef<mozilla::gfx::GlyphRenderingOptions>
    GetGlyphRenderingOptions(const TextRunDrawParams* aRunParams = nullptr) MOZ_OVERRIDE;

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;

    virtual FontType GetType() const { return FONT_TYPE_DWRITE; }

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
    GetScaledFont(mozilla::gfx::DrawTarget *aTarget) MOZ_OVERRIDE;

    virtual cairo_scaled_font_t *GetCairoScaledFont() MOZ_OVERRIDE;

protected:
    virtual const Metrics& GetHorizontalMetrics() MOZ_OVERRIDE;

    bool GetFakeMetricsForArialBlack(DWRITE_FONT_METRICS *aFontMetrics);

    void ComputeMetrics(AntialiasOption anAAOption);

    bool HasBitmapStrikeForSize(uint32_t aSize);

    cairo_font_face_t *CairoFontFace();

    gfxFloat MeasureGlyphWidth(uint16_t aGlyph);

    DWRITE_MEASURING_MODE GetMeasuringMode();
    bool GetForceGDIClassic();

    nsRefPtr<IDWriteFontFace> mFontFace;
    cairo_font_face_t *mCairoFontFace;

    Metrics *mMetrics;

    
    nsAutoPtr<nsDataHashtable<nsUint32HashKey,int32_t> > mGlyphWidths;

    bool mNeedsOblique;
    bool mNeedsBold;
    bool mUseSubpixelPositions;
    bool mAllowManualShowGlyphs;
    bool mAzureScaledFontIsCairo;
};

#endif
