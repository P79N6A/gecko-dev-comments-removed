







































#ifndef GFX_GDIFONT_H
#define GFX_GDIFONT_H

#include "gfxFont.h"
#include "gfxGDIFontList.h"

#include "nsDataHashtable.h"
#include "nsHashKeys.h"

#include "cairo.h"

class gfxGDIFont : public gfxFont
{
public:
    gfxGDIFont(GDIFontEntry *aFontEntry,
               const gfxFontStyle *aFontStyle,
               PRBool aNeedsBold,
               AntialiasOption anAAOption = kAntialiasDefault);

    virtual ~gfxGDIFont();

    HFONT GetHFONT() { if (!mMetrics) Initialize(); return mFont; }

    gfxFloat GetAdjustedSize() { if (!mMetrics) Initialize(); return mAdjustedSize; }

    cairo_font_face_t   *CairoFontFace() { return mFontFace; }
    cairo_scaled_font_t *CairoScaledFont() { return mScaledFont; }

    
    virtual const gfxFont::Metrics& GetMetrics();

    virtual PRUint32 GetSpaceGlyph();

    virtual PRBool SetupCairoFont(gfxContext *aContext);

    
    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               PRUint32 aStart, PRUint32 aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing);

    
    virtual gfxFont* CopyWithAntialiasOption(AntialiasOption anAAOption);

    virtual PRBool ProvidesGlyphWidths() { return PR_TRUE; }

    
    virtual PRInt32 GetGlyphWidth(gfxContext *aCtx, PRUint16 aGID);

protected:
    virtual void CreatePlatformShaper();

    
    virtual PRBool InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength,
                               PRInt32 aRunScript,
                               PRBool aPreferPlatformShaping = PR_FALSE);

    void Initialize(); 

    void FillLogFont(LOGFONTW& aLogFont, gfxFloat aSize);

    
    
    nsAutoPtr<gfxFontShaper>   mUniscribeShaper;

    HFONT                 mFont;
    cairo_font_face_t    *mFontFace;
    cairo_scaled_font_t  *mScaledFont;

    Metrics              *mMetrics;
    PRUint32              mSpaceGlyph;

    PRBool                mNeedsBold;

    
    nsDataHashtable<nsUint32HashKey,PRInt32>    mGlyphWidths;
};

#endif 
