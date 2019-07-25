







































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
    CGFontRef GetCGFontRef() const { return mCGFont; }

    virtual PRBool InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength,
                               PRInt32 aRunScript,
                               PRBool aPreferPlatformShaping = PR_FALSE);

    
    virtual const gfxFont::Metrics& GetMetrics() {
        return mMetrics;
    }

    virtual PRUint32 GetSpaceGlyph() {
        return mSpaceGlyph;
    }

    virtual PRBool SetupCairoFont(gfxContext *aContext);

    
    
    virtual hb_blob_t *GetFontTable(PRUint32 aTag);

protected:
    virtual void CreatePlatformShaper();

    void InitMetrics();
    void InitMetricsFromATSMetrics();

    
    
    gfxFloat GetCharWidth(CFDataRef aCmap, PRUnichar aUniChar,
                          PRUint32 *aGlyphID, gfxFloat aConvFactor);

    static void DestroyBlobFunc(void* aUserData);

    ATSFontRef            mATSFont;
    CGFontRef             mCGFont;

    cairo_font_face_t    *mFontFace;
    cairo_scaled_font_t  *mScaledFont;

    Metrics               mMetrics;
    PRUint32              mSpaceGlyph;
};

#endif 
