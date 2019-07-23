







































#ifndef GFX_GDIFONT_H
#define GFX_GDIFONT_H

#include "gfxFont.h"
#include "gfxGDIFontList.h"

#include "cairo.h"

class gfxGDIFont : public gfxFont
{
public:
    gfxGDIFont(GDIFontEntry *aFontEntry,
               const gfxFontStyle *aFontStyle,
               PRBool aNeedsBold,
               AntialiasOption anAAOption = kAntialiasDefault);

    virtual ~gfxGDIFont();

    HFONT GetHFONT() const { return mFont; }

    float GetAdjustedSize() const { return mAdjustedSize; }

    cairo_font_face_t   *CairoFontFace() { return mFontFace; }
    cairo_scaled_font_t *CairoScaledFont() { return mScaledFont; }

    
    virtual const gfxFont::Metrics& GetMetrics();

    virtual PRUint32 GetSpaceGlyph();

    virtual PRBool SetupCairoFont(gfxContext *aContext);

    
    virtual gfxFont* CopyWithAntialiasOption(AntialiasOption anAAOption);

    
    virtual void InitTextRun(gfxContext *aContext,
                             gfxTextRun *aTextRun,
                             const PRUnichar *aString,
                             PRUint32 aRunStart,
                             PRUint32 aRunLength);

protected:
    void InitMetrics();
    void FillLogFont(gfxFloat aSize);

    float GetCharWidth(PRUnichar aUniChar, PRUint32 *aGlyphID);
    float GetCharHeight(PRUnichar aUniChar);

    PRBool                mNeedsBold;

    LOGFONTW              mLogFont;
    HFONT                 mFont;

    cairo_font_face_t    *mFontFace;
    cairo_scaled_font_t  *mScaledFont;

    Metrics               mMetrics;
    PRUint32              mSpaceGlyph;
    float                 mAdjustedSize;
};

#endif 
