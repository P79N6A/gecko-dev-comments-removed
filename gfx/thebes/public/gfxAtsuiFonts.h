







































#ifndef GFX_ATSUIFONTS_H
#define GFX_ATSUIFONTS_H

#ifndef __LP64__ 

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxFontUtils.h"
#include "gfxPlatform.h"

#include <Carbon/Carbon.h>

class MacOSFontEntry;

#define kLiGothicBadCharUnicode  0x775B // ATSUI failure on 10.6 (bug 532346)
#define kLiGothicBadCharGlyph    3774   // the expected glyph for this char

class gfxAtsuiFont : public gfxFont {
public:

    gfxAtsuiFont(MacOSFontEntry *aFontEntry,
                 const gfxFontStyle *fontStyle, PRBool aNeedsBold);

    virtual ~gfxAtsuiFont();

    virtual const gfxFont::Metrics& GetMetrics();

    float GetCharWidth(PRUnichar c, PRUint32 *aGlyphID = nsnull);
    float GetCharHeight(PRUnichar c);

    ATSFontRef GetATSFontRef();

    cairo_font_face_t *CairoFontFace() { return mFontFace; }
    cairo_scaled_font_t *CairoScaledFont() { return mScaledFont; }

    ATSUStyle GetATSUStyle() { return mATSUStyle; }

    virtual nsString GetUniqueName();

    virtual PRUint32 GetSpaceGlyph() { return mSpaceGlyph; }

    PRBool HasMirroringInfo();

    virtual void SetupGlyphExtents(gfxContext *aContext, PRUint32 aGlyphID,
            PRBool aNeedTight, gfxGlyphExtents *aExtents);

protected:
    const gfxFontStyle *mFontStyle;

    ATSUStyle mATSUStyle;

    PRBool mHasMirroring;
    PRBool mHasMirroringLookedUp;

    nsString mUniqueName;

    cairo_font_face_t *mFontFace;
    cairo_scaled_font_t *mScaledFont;

    gfxFont::Metrics mMetrics;

    gfxFloat mAdjustedSize;
    PRUint32 mSpaceGlyph;    

    void InitMetrics(ATSUFontID aFontID, ATSFontRef aFontRef);

    virtual void InitTextRun(gfxTextRun *aTextRun,
                             const PRUnichar *aString,
                             PRUint32 aRunStart,
                             PRUint32 aRunLength);

    virtual PRBool SetupCairoFont(gfxContext *aContext);
};

#endif 

#endif 
