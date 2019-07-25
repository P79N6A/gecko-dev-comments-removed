




































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

    virtual PRUint32 GetSpaceGlyph();

    virtual bool SetupCairoFont(gfxContext *aContext);

    virtual bool IsValid();

    gfxFloat GetAdjustedSize() {
        return mAdjustedSize;
    }

    IDWriteFontFace *GetFontFace();

    
    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               PRUint32 aStart, PRUint32 aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing);

    
    
    virtual hb_blob_t *GetFontTable(PRUint32 aTag);

    virtual bool ProvidesGlyphWidths();

    virtual PRInt32 GetGlyphWidth(gfxContext *aCtx, PRUint16 aGID);

protected:
    friend class gfxDWriteShaper;

    virtual void CreatePlatformShaper();

    bool GetFakeMetricsForArialBlack(DWRITE_FONT_METRICS *aFontMetrics);

    void ComputeMetrics(AntialiasOption anAAOption);

    bool HasBitmapStrikeForSize(PRUint32 aSize);

    cairo_font_face_t *CairoFontFace();

    cairo_scaled_font_t *CairoScaledFont();

    gfxFloat MeasureGlyphWidth(PRUint16 aGlyph);

    static void DestroyBlobFunc(void* userArg);

    DWRITE_MEASURING_MODE GetMeasuringMode();
    bool GetForceGDIClassic();

    nsRefPtr<IDWriteFontFace> mFontFace;
    cairo_font_face_t *mCairoFontFace;

    gfxFont::Metrics          *mMetrics;

    
    nsDataHashtable<nsUint32HashKey,PRInt32>    mGlyphWidths;

    bool mNeedsOblique;
    bool mNeedsBold;
    bool mUseSubpixelPositions;
    bool mAllowManualShowGlyphs;
};

#endif
