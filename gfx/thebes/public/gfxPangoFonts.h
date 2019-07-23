





































#ifndef GFX_PANGOFONTS_H
#define GFX_PANGOFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"

#include <pango/pango.h>





#define ENABLE_FAST_PATH_8BIT




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

    PangoFontDescription *GetPangoFontDescription() { if (!mPangoFontDesc) RealizeFont(); return mPangoFontDesc; }
    PangoContext *GetPangoContext() { if (!mPangoFontDesc) RealizeFont(); return mPangoCtx; }

    void GetMozLang(nsACString &aMozLang);
    void GetActualFontFamily(nsACString &aFamily);

    PangoFont *GetPangoFont() { if (!mPangoFont) RealizePangoFont(); return mPangoFont; }
    gfxFloat GetAdjustedSize() { if (!mPangoFontDesc) RealizeFont(); return mAdjustedSize; }

    PRUint32 GetGlyph(const PRUint32 aChar);

    virtual nsString GetUniqueName();

    
    virtual PRUint32 GetSpaceGlyph() {
        GetMetrics();
        return mSpaceGlyph;
    }

protected:
    PangoFontDescription *mPangoFontDesc;
    PangoContext *mPangoCtx;

    PangoFont *mPangoFont;
    cairo_scaled_font_t *mCairoFont;

    PRBool   mHasMetrics;
    PRUint32 mSpaceGlyph;
    Metrics  mMetrics;
    gfxFloat mAdjustedSize;

    void RealizeFont(PRBool force = PR_FALSE);
    void RealizePangoFont(PRBool aForce = PR_FALSE);
    void GetCharSize(const char aChar, gfxSize& aInkSize, gfxSize& aLogSize,
                     PRUint32 *aGlyphID = nsnull);

    virtual PRBool SetupCairoFont(cairo_t *aCR);
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
                     PRUint32 aUTF8Length, PRBool aTake8BitPath);

    
    nsresult SetGlyphs(gfxTextRun *aTextRun, gfxPangoFont *aFont,
                       const gchar *aUTF8, PRUint32 aUTF8Length,
                       PRUint32 *aUTF16Offset, PangoGlyphString *aGlyphs,
                       PangoGlyphUnit aOverrideSpaceWidth,
                       PRBool aAbortOnMissingGlyph);
    nsresult SetMissingGlyphs(gfxTextRun *aTextRun,
                              const gchar *aUTF8, PRUint32 aUTF8Length,
                              PRUint32 *aUTF16Offset);
    void CreateGlyphRunsItemizing(gfxTextRun *aTextRun,
                                  const gchar *aUTF8, PRUint32 aUTF8Length);
#if defined(ENABLE_FAST_PATH_8BIT) || defined(ENABLE_FAST_PATH_ALWAYS)
    PRBool CanTakeFastPath(PRUint32 aFlags);
    void CreateGlyphRunsFast(gfxTextRun *aTextRun,
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

#endif 
