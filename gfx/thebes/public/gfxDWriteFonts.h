




































#ifndef GFX_WINDOWSDWRITEFONTS_H
#define GFX_WINDOWSDWRITEFONTS_H

#include <dwrite.h>

#include "gfxFont.h"
#include "gfxUserFontSet.h"
#include "cairo-win32.h"




class gfxDWriteFont : public gfxFont 
{
public:
    gfxDWriteFont(gfxFontEntry *aFontEntry,
                  const gfxFontStyle *aFontStyle,
                  PRBool aNeedsBold = PR_FALSE);
    ~gfxDWriteFont();

    virtual nsString GetUniqueName();

    virtual const gfxFont::Metrics& GetMetrics();

    virtual PRUint32 GetSpaceGlyph();

    virtual PRBool SetupCairoFont(gfxContext *aContext);

    virtual PRBool IsValid() { return mFontFace != NULL; }

    gfxFloat GetAdjustedSize() const { return mAdjustedSize; }

    IDWriteFontFace *GetFontFace() { return mFontFace.get(); }

protected:
    void ComputeMetrics();

    cairo_font_face_t *CairoFontFace();

    cairo_scaled_font_t *CairoScaledFont();

    nsRefPtr<IDWriteFontFace> mFontFace;
    cairo_font_face_t *mCairoFontFace;
    cairo_scaled_font_t *mCairoScaledFont;

    gfxFloat mAdjustedSize;
    gfxFont::Metrics mMetrics;
    PRBool mNeedsOblique;
};

#endif
