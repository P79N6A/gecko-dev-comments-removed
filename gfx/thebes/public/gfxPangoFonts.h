





































#ifndef GFX_PANGOFONTS_H
#define GFX_PANGOFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"

#include <pango/pango.h>
#include <X11/Xft/Xft.h>




#define ENABLE_XFT_FAST_PATH_8BIT



#include "nsDataHashtable.h"
#include "nsClassHashtable.h"

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

    XftFont *GetXftFont () { RealizeXftFont (); return mXftFont; }
    PangoFont *GetPangoFont() { RealizePangoFont(); return mPangoFont; }
    gfxFloat GetAdjustedSize() { RealizeFont(); return mAdjustedSize; }

    PRBool HasGlyph(const PRUint32 aChar);
    PRUint32 GetGlyph(const PRUint32 aChar);

    virtual gfxTextRun::Metrics Measure(gfxTextRun *aTextRun,
                                        PRUint32 aStart, PRUint32 aEnd,
                                        PRBool aTightBoundingBox,
                                        Spacing *aSpacing);

    virtual nsString GetUniqueName();

protected:
    PangoFontDescription *mPangoFontDesc;
    PangoContext *mPangoCtx;

    XftFont *mXftFont;
    PangoFont *mPangoFont;
    PangoFont *mGlyphTestingFont;
    cairo_scaled_font_t *mCairoFont;

    PRBool mHasMetrics;
    Metrics mMetrics;
    gfxFloat mAdjustedSize;

    void RealizeFont(PRBool force = PR_FALSE);
    void RealizeXftFont(PRBool force = PR_FALSE);
    void GetSize(const char *aString, PRUint32 aLength, gfxSize& inkSize, gfxSize& logSize);
    void RealizePangoFont(PRBool aForce = PR_FALSE);
    void GetCharSize(const char aChar, gfxSize& aInkSize, gfxSize& aLogSize);

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
                     PRUint32 aUTF8Length, PRUint32 aUTF8HeaderLength);
    
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

class gfxPangoFontWrapper {
public:
    gfxPangoFontWrapper(PangoFont *aFont) {
        mFont = aFont;
        g_object_ref(mFont);
    }
    ~gfxPangoFontWrapper() {
        if (mFont)
            g_object_unref(mFont);
    }
    PangoFont* Get() { return mFont; }
private:
    PangoFont *mFont;
};

class gfxPangoFontCache
{
public:
    gfxPangoFontCache();
    ~gfxPangoFontCache();

    static gfxPangoFontCache* GetPangoFontCache() {
        if (!sPangoFontCache)
            sPangoFontCache = new gfxPangoFontCache();
        return sPangoFontCache;
    }
    static void Shutdown() {
        if (sPangoFontCache)
            delete sPangoFontCache;
        sPangoFontCache = nsnull;
    }

    void Put(const PangoFontDescription *aFontDesc, PangoFont *aPangoFont);
    PangoFont* Get(const PangoFontDescription *aFontDesc);
private:
    static gfxPangoFontCache *sPangoFontCache;
    nsClassHashtable<nsUint32HashKey,  gfxPangoFontWrapper> mPangoFonts;
};



class gfxPangoFontNameMap
{
public:
    gfxPangoFontNameMap();
    ~gfxPangoFontNameMap();

    static gfxPangoFontNameMap* GetPangoFontNameMap() {
        if (!sPangoFontNameMap)
            sPangoFontNameMap = new gfxPangoFontNameMap();
        return sPangoFontNameMap;
    }
    static void Shutdown() {
        if (sPangoFontNameMap)
            delete sPangoFontNameMap;
        sPangoFontNameMap = nsnull;
    }

    void Put(const nsACString &aName, PangoFont *aPangoFont);
    PangoFont* Get(const nsACString &aName);

private:
    static gfxPangoFontNameMap *sPangoFontNameMap;
    nsClassHashtable<nsCStringHashKey, gfxPangoFontWrapper> mPangoFonts;
};
#endif 
