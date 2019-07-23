









































#ifndef GFX_FT2FONTBASE_H
#define GFX_FT2FONTBASE_H

#include "cairo.h"
#include "gfxContext.h"
#include "gfxFont.h"

class gfxFT2FontBase : public gfxFont {
public:
    gfxFT2FontBase(cairo_scaled_font_t *aScaledFont,
                   gfxFontEntry *aFontEntry,
                   const gfxFontStyle *aFontStyle);
    virtual ~gfxFT2FontBase();

    PRUint32 GetGlyph(PRUint32 aCharCode);
    void GetGlyphExtents(PRUint32 aGlyph,
                         cairo_text_extents_t* aExtents);
    virtual const gfxFont::Metrics& GetMetrics();
    virtual nsString GetUniqueName();
    virtual PRUint32 GetSpaceGlyph();

    cairo_scaled_font_t *CairoScaledFont() { return mScaledFont; };
    virtual PRBool SetupCairoFont(gfxContext *aContext);

protected:
    cairo_scaled_font_t *mScaledFont;
    PRUint32 mSpaceGlyph;
    PRBool mHasMetrics;
    Metrics mMetrics;
};

#endif 
