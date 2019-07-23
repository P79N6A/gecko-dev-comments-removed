





































#ifndef GFX_PANGOFONTS_H
#define GFX_PANGOFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"

#include <pango/pango.h>
#include <X11/Xft/Xft.h>




#define ENABLE_XFT_FAST_PATH_8BIT



#include "nsDataHashtable.h"

class FontSelector;

class gfxPangoTextRun;

class gfxPangoFont : public gfxFont {
public:
    gfxPangoFont (const nsAString& aName,
                  const gfxFontStyle *aFontStyle);
    virtual ~gfxPangoFont ();

    static void Shutdown();

    virtual const gfxFont::Metrics& GetMetrics();

    PangoFontDescription *GetPangoFontDescription() { RealizeFont(); return mPangoFontDesc; }
    PangoContext *GetPangoContext() { RealizeFont(); return mPangoCtx; }

    void GetMozLang(nsACString &aMozLang);
    void GetActualFontFamily(nsACString &aFamily);
    PangoFont *GetPangoFont();

    XftFont *GetXftFont () { RealizeXftFont (); return mXftFont; }
    gfxFloat GetAdjustedSize() { RealizeFont(); return mAdjustedSize; }

    virtual gfxTextRun::Metrics Measure(gfxTextRun *aTextRun,
                                        PRUint32 aStart, PRUint32 aEnd,
                                        PRBool aTightBoundingBox,
                                        Spacing *aSpacing);

    virtual nsString GetUniqueName();

protected:
    PangoFontDescription *mPangoFontDesc;
    PangoContext *mPangoCtx;

    XftFont *mXftFont;
    cairo_scaled_font_t *mCairoFont;

    PRBool mHasMetrics;
    Metrics mMetrics;
    gfxFloat mAdjustedSize;

    void RealizeFont(PRBool force = PR_FALSE);
    void RealizeXftFont(PRBool force = PR_FALSE);
    void GetSize(const char *aString, PRUint32 aLength, gfxSize& inkSize, gfxSize& logSize);

    virtual void SetupCairoFont(cairo_t *aCR);
};

class FontSelector;

class THEBES_API gfxPangoFontGroup : public gfxFontGroup {
public:
    gfxPangoFontGroup (const nsAString& families,
                       const gfxFontStyle *aStyle);
    virtual ~gfxPangoFontGroup ();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    
    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                    Parameters *aParams);
    virtual gfxTextRun *MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                    Parameters *aParams);

    gfxPangoFont *GetFontAt(PRInt32 i) {
        return NS_STATIC_CAST(gfxPangoFont*, 
                              NS_STATIC_CAST(gfxFont*, mFonts[i]));
    }

protected:
    friend class FontSelector;

    

    
    void InitTextRun(gfxTextRun *aTextRun, const gchar *aUTF8Text,
                     PRUint32 aUTF8Length, PRUint32 aUTF8HeaderLength,
                     const PRUnichar *aUTF16Text, PRUint32 aUTF16Length);
    
    nsresult SetGlyphs(gfxTextRun *aTextRun, const gchar *aUTF8,
                       PRUint32 aUTF8Length,
                       PRUint32 *aUTF16Offset, PangoGlyphString *aGlyphs,
                       PangoGlyphUnit aOverrideSpaceWidth,
                       PRBool aAbortOnMissingGlyph);
    
    
    nsresult CreateGlyphRunsFast(gfxTextRun *aTextRun,
                                 const gchar *aUTF8, PRUint32 aUTF8Length,
                                 const PRUnichar *aUTF16Text, PRUint32 aUTF16Length);
    void CreateGlyphRunsItemizing(gfxTextRun *aTextRun,
                                  const gchar *aUTF8, PRUint32 aUTF8Length,
                                  PRUint32 aUTF8HeaderLength);
#if defined(ENABLE_XFT_FAST_PATH_8BIT) || defined(ENABLE_XFT_FAST_PATH_ALWAYS)
    void CreateGlyphRunsXft(gfxTextRun *aTextRun,
                            const gchar *aUTF8, PRUint32 aUTF8Length);
#endif

    static PRBool FontCallback (const nsAString& fontName,
                                const nsACString& genericName,
                                void *closure);

private:
    nsTArray<gfxFontStyle> mAdditionalStyles;
};

#endif 
