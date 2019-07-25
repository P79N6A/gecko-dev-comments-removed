









































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
    virtual PRUint32 GetSpaceGlyph();
    virtual hb_blob_t *GetFontTable(PRUint32 aTag);
    virtual PRBool ProvidesGetGlyph() const { return PR_TRUE; }
    virtual PRUint32 GetGlyph(PRUint32 unicode, PRUint32 variation_selector);
    virtual PRBool ProvidesGlyphWidths() { return PR_TRUE; }
    virtual PRInt32 GetGlyphWidth(gfxContext *aCtx, PRUint16 aGID);

    cairo_scaled_font_t *CairoScaledFont() { return mScaledFont; };
    virtual PRBool SetupCairoFont(gfxContext *aContext);

protected:
    cairo_scaled_font_t *mScaledFont;
    PRUint32 mSpaceGlyph;
    PRBool mHasMetrics;
    Metrics mMetrics;
};

#endif 
