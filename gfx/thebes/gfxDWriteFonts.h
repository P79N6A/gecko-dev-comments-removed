




#ifndef GFX_WINDOWSDWRITEFONTS_H
#define GFX_WINDOWSDWRITEFONTS_H

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

    virtual gfxFont* CopyWithAntialiasOption(AntialiasOption anAAOption);

    virtual const gfxFont::Metrics& GetMetrics();

    virtual uint32_t GetSpaceGlyph();

    virtual bool SetupCairoFont(gfxContext *aContext);

    virtual bool AllowSubpixelAA() { return mAllowManualShowGlyphs; }

    virtual bool IsValid();

    gfxFloat GetAdjustedSize() {
        return mAdjustedSize;
    }

    IDWriteFontFace *GetFontFace();

    
    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               uint32_t aStart, uint32_t aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing);

    
    
    virtual hb_blob_t *GetFontTable(uint32_t aTag);

    virtual bool ProvidesGlyphWidths();

    virtual int32_t GetGlyphWidth(gfxContext *aCtx, uint16_t aGID);

    virtual mozilla::TemporaryRef<mozilla::gfx::GlyphRenderingOptions> GetGlyphRenderingOptions();

    virtual void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontCacheSizes*   aSizes) const;
    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontCacheSizes*   aSizes) const;

    virtual FontType GetType() const { return FONT_TYPE_DWRITE; }

protected:
    friend class gfxDWriteShaper;

    virtual void CreatePlatformShaper();

    bool GetFakeMetricsForArialBlack(DWRITE_FONT_METRICS *aFontMetrics);

    void ComputeMetrics(AntialiasOption anAAOption);

    bool HasBitmapStrikeForSize(uint32_t aSize);

    cairo_font_face_t *CairoFontFace();

    cairo_scaled_font_t *CairoScaledFont();

    gfxFloat MeasureGlyphWidth(uint16_t aGlyph);

    static void DestroyBlobFunc(void* userArg);

    DWRITE_MEASURING_MODE GetMeasuringMode();
    bool GetForceGDIClassic();

    nsRefPtr<IDWriteFontFace> mFontFace;
    cairo_font_face_t *mCairoFontFace;

    gfxFont::Metrics          *mMetrics;

    
    nsDataHashtable<nsUint32HashKey,int32_t>    mGlyphWidths;

    bool mNeedsOblique;
    bool mNeedsBold;
    bool mUseSubpixelPositions;
    bool mAllowManualShowGlyphs;
};

#endif
