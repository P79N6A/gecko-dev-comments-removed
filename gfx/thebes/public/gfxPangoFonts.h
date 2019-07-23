





































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

class THEBES_API gfxPangoFontGroup : public gfxFontGroup {
public:
    gfxPangoFontGroup (const nsAString& families,
                       const gfxFontStyle *aStyle,
                       gfxUserFontSet *aUserFontSet);
    virtual ~gfxPangoFontGroup ();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    
    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);

    virtual gfxFont *GetFontAt(PRInt32 i);

    static void Shutdown();

protected:
    PangoFont *mBasePangoFont;
    gfxFloat mAdjustedSize;

    

    





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
    PangoFont *GetBasePangoFont();

    
    gfxFloat GetAdjustedSize()
    {
        if (!mBasePangoFont)
            GetBasePangoFont();
        return mAdjustedSize;
    }
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
