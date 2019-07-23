





































#ifndef GFX_PANGOFONTS_H
#define GFX_PANGOFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"

#include <pango/pango.h>





#define ENABLE_FAST_PATH_8BIT




#include "nsDataHashtable.h"
#include "nsClassHashtable.h"

class gfxPangoTextRun;


class gfxPangoFontEntry : public gfxFontEntry {
public:
    gfxPangoFontEntry(const nsAString& aName)
        : gfxFontEntry(aName)
    { }

    ~gfxPangoFontEntry() {}
        
};

class gfxPangoFont : public gfxFont {
public:
    gfxPangoFont (gfxPangoFontEntry *aFontEntry,
                  const gfxFontStyle *aFontStyle);
    virtual ~gfxPangoFont ();
    static already_AddRefed<gfxPangoFont> GetOrMakeFont(PangoFont *aPangoFont);

    static void Shutdown();

    virtual const gfxFont::Metrics& GetMetrics();

    PangoFont *GetPangoFont() { if (!mPangoFont) RealizePangoFont(); return mPangoFont; }

    
    gfxFloat GetAdjustedSize() { if (!mPangoFont) RealizePangoFont(); return mAdjustedSize; }

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

    gfxPangoFont(PangoFont *aPangoFont, gfxPangoFontEntry *aFontEntry,
                 const gfxFontStyle *aFontStyle);
    void RealizePangoFont();
    void GetCharSize(const char aChar, gfxSize& aInkSize, gfxSize& aLogSize,
                     PRUint32 *aGlyphID = nsnull);

    virtual PRBool SetupCairoFont(gfxContext *aContext);
};

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

    virtual gfxPangoFont *GetFontAt(PRInt32 i);

protected:
    

    





    void InitTextRun(gfxTextRun *aTextRun, const gchar *aUTF8Text,
                     PRUint32 aUTF8Length, PRUint32 aUTF8HeaderLength,
                     PRBool aTake8BitPath);

    
    nsresult SetGlyphs(gfxTextRun *aTextRun,
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

    void GetFcFamilies(nsAString &aFcFamilies);
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
