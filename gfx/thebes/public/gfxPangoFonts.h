





































#ifndef GFX_PANGOFONTS_H
#define GFX_PANGOFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"

#include <pango/pango.h>





#define ENABLE_FAST_PATH_8BIT




class gfxPangoTextRun;

class gfxPangoFont : public gfxFont {
public:
    gfxPangoFont (const nsAString& aName,
                  const gfxFontStyle *aFontStyle);
    virtual ~gfxPangoFont ();

    static void Shutdown();

    virtual const gfxFont::Metrics& GetMetrics();

    void GetMozLang(nsACString &aMozLang);
    void GetActualFontFamily(nsACString &aFamily);

    PangoFont *GetPangoFont() { if (!mPangoFont) RealizePangoFont(); return mPangoFont; }
    PRUint32 GetGlyph(const PRUint32 aChar);

    virtual nsString GetUniqueName();

    
    virtual PRUint32 GetSpaceGlyph() {
        NS_ASSERTION(GetStyle()->size != 0,
                     "forgot to short-circuit a text run with zero-sized font?");
        GetMetrics();
        return mSpaceGlyph;
    }

protected:
    PangoFont *mPangoFont;
    cairo_scaled_font_t *mCairoFont;

    PRBool   mHasMetrics;
    PRUint32 mSpaceGlyph;
    Metrics  mMetrics;
    gfxFloat mAdjustedSize;

    void RealizePangoFont();
    void GetCharSize(const char aChar, gfxSize& aInkSize, gfxSize& aLogSize,
                     PRUint32 *aGlyphID = nsnull);

    virtual PRBool SetupCairoFont(gfxContext *aContext);
};

class FontSelector;

class THEBES_API gfxPangoFontGroup : public gfxFontGroup {
public:
    gfxPangoFontGroup (const nsAString& families,
                       const gfxFontStyle *aStyle);
    virtual ~gfxPangoFontGroup ();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    
    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);

    gfxPangoFont *GetFontAt(PRInt32 i) {
        return static_cast<gfxPangoFont*>(static_cast<gfxFont*>(mFonts[i]));
    }

protected:
    friend class FontSelector;

    

    





    void InitTextRun(gfxTextRun *aTextRun, const gchar *aUTF8Text,
                     PRUint32 aUTF8Length, PRUint32 aUTF8HeaderLength,
                     PRBool aTake8BitPath);

    
    nsresult SetGlyphs(gfxTextRun *aTextRun, gfxPangoFont *aFont,
                       const gchar *aUTF8, PRUint32 aUTF8Length,
                       PRUint32 *aUTF16Offset, PangoGlyphString *aGlyphs,
                       PangoGlyphUnit aOverrideSpaceWidth,
                       PRBool aAbortOnMissingGlyph);
    nsresult SetMissingGlyphs(gfxTextRun *aTextRun,
                              const gchar *aUTF8, PRUint32 aUTF8Length,
                              PRUint32 *aUTF16Offset);
    void CreateGlyphRunsItemizing(gfxTextRun *aTextRun,
                                  const gchar *aUTF8, PRUint32 aUTF8Length,
                                  PRUint32 aUTF8HeaderLength);
#if defined(ENABLE_FAST_PATH_8BIT) || defined(ENABLE_FAST_PATH_ALWAYS)
    PRBool CanTakeFastPath(PRUint32 aFlags);
    nsresult CreateGlyphRunsFast(gfxTextRun *aTextRun,
                                 const gchar *aUTF8, PRUint32 aUTF8Length);
#endif

    static PRBool FontCallback (const nsAString& fontName,
                                const nsACString& genericName,
                                void *closure);
};

#endif 
