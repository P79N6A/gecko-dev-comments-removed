







































#ifndef GFX_MACFONT_H
#define GFX_MACFONT_H

#include "gfxFont.h"
#include "gfxMacPlatformFontList.h"

#include "cairo.h"

class gfxMacFont : public gfxFont
{
public:
    gfxMacFont(MacOSFontEntry *aFontEntry, const gfxFontStyle *aFontStyle,
               PRBool aNeedsBold);

    virtual ~gfxMacFont();

    ATSFontRef GetATSFontRef() const { return mATSFont; }

    
    
    float GetAdjustedSize() const { return mAdjustedSize; }

    
    virtual const gfxFont::Metrics& GetMetrics() {
        return mMetrics;
    }

    virtual PRUint32 GetSpaceGlyph() {
        return mSpaceGlyph;
    }

    virtual PRBool SetupCairoFont(gfxContext *aContext);

protected:
    void InitMetrics();

    float GetCharWidth(CTFontRef aCTFont, PRUnichar aUniChar,
                       PRUint32 *aGlyphID);
    float GetCharHeight(CTFontRef aCTFont, PRUnichar aUniChar);

    ATSFontRef            mATSFont;

    cairo_font_face_t    *mFontFace;
    cairo_scaled_font_t  *mScaledFont;

    Metrics               mMetrics;
    PRUint32              mSpaceGlyph;
    float                 mAdjustedSize;
};

#endif 
