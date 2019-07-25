




































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
                  PRBool aNeedsBold = PR_FALSE,
                  AntialiasOption = kAntialiasDefault);
    ~gfxDWriteFont();

    virtual gfxFont* CopyWithAntialiasOption(AntialiasOption anAAOption);

    virtual const gfxFont::Metrics& GetMetrics();

    virtual PRUint32 GetSpaceGlyph();

    virtual PRBool SetupCairoFont(gfxContext *aContext);

    virtual PRBool IsValid();

    gfxFloat GetAdjustedSize() {
        return mAdjustedSize;
    }

    IDWriteFontFace *GetFontFace();

    
    
    virtual hb_blob_t *GetFontTable(PRUint32 aTag);

    virtual PRBool ProvidesGlyphWidths();

    virtual PRInt32 GetGlyphWidth(gfxContext *aCtx, PRUint16 aGID);

protected:
    friend class gfxDWriteShaper;

    virtual void CreatePlatformShaper();

    PRBool GetFakeMetricsForArialBlack(DWRITE_FONT_METRICS *aFontMetrics);

    void ComputeMetrics();

    PRBool HasBitmapStrikeForSize(PRUint32 aSize);

    cairo_font_face_t *CairoFontFace();

    cairo_scaled_font_t *CairoScaledFont();

    gfxFloat MeasureGlyphWidth(PRUint16 aGlyph);

    static void DestroyBlobFunc(void* userArg);

    DWRITE_MEASURING_MODE GetMeasuringMode();

    nsRefPtr<IDWriteFontFace> mFontFace;
    cairo_font_face_t *mCairoFontFace;
    cairo_scaled_font_t *mCairoScaledFont;

    gfxFont::Metrics          *mMetrics;

    
    nsDataHashtable<nsUint32HashKey,PRInt32>    mGlyphWidths;

    PRPackedBool mNeedsOblique;
    PRPackedBool mNeedsBold;
    PRPackedBool mUseSubpixelPositions;
    PRPackedBool mAllowManualShowGlyphs;
};

#endif
