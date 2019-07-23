





































#ifndef GFX_PANGOFONTS_H
#define GFX_PANGOFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"

#include "nsAutoRef.h"
#include "nsTArray.h"

#include <pango/pango.h>





#define ENABLE_FAST_PATH_8BIT




class gfxFcPangoFontSet;
class gfxProxyFontEntry;
typedef struct _FcPattern FcPattern;
typedef struct FT_FaceRec_* FT_Face;

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

    virtual void UpdateFontList();

    static void Shutdown();

    
    static gfxFontEntry *NewFontEntry(const gfxProxyFontEntry &aProxyEntry,
                                      const nsAString &aFullname);
    
    static gfxFontEntry *NewFontEntry(const gfxProxyFontEntry &aProxyEntry,
                                      const PRUint8 *aFontData,
                                      PRUint32 aLength);

    
    

    
    PangoFont *GetBasePangoFont();

    
    PangoLanguage *GetPangoLanguage() { return mPangoLanguage; }

    
    
    
    gfxFcPangoFontSet *GetFontSet(PangoLanguage *aLang = NULL);

protected:
    class FontSetByLangEntry {
    public:
        FontSetByLangEntry(PangoLanguage *aLang, gfxFcPangoFontSet *aFontSet);
        PangoLanguage *mLang;
        nsRefPtr<gfxFcPangoFontSet> mFontSet;
    };
    
    
    nsAutoTArray<FontSetByLangEntry,1> mFontSets;

    gfxFloat mSizeAdjustFactor;
    PangoLanguage *mPangoLanguage;

    

    





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

    void GetFcFamilies(nsTArray<nsString> *aFcFamilyList,
                       const nsACString& aLangGroup);

    
    
    
    already_AddRefed<gfxFcPangoFontSet>
    MakeFontSet(PangoLanguage *aLang, gfxFloat aSizeAdjustFactor,
                nsAutoRef<FcPattern> *aMatchPattern = NULL);

    gfxFcPangoFontSet *GetBaseFontSet();

    gfxFloat GetSizeAdjustFactor()
    {
        if (mFontSets.Length() == 0)
            GetBaseFontSet();
        return mSizeAdjustFactor;
    }
};

#endif 
