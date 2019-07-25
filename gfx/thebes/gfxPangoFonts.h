





































#ifndef GFX_PANGOFONTS_H
#define GFX_PANGOFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"

#include "nsAutoRef.h"
#include "nsTArray.h"

#include <pango/pango.h>





#define ENABLE_FAST_PATH_8BIT

class gfxFcFontSet;
class gfxFcFont;
class gfxProxyFontEntry;
typedef struct _FcPattern FcPattern;
typedef struct FT_FaceRec_* FT_Face;
typedef struct FT_LibraryRec_  *FT_Library;

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

    virtual already_AddRefed<gfxFont>
        FindFontForChar(PRUint32 aCh, PRUint32 aPrevCh, PRInt32 aRunScript,
                        gfxFont *aPrevMatchedFont);

    static void Shutdown();

    
    static gfxFontEntry *NewFontEntry(const gfxProxyFontEntry &aProxyEntry,
                                      const nsAString &aFullname);
    
    static gfxFontEntry *NewFontEntry(const gfxProxyFontEntry &aProxyEntry,
                                      const PRUint8 *aFontData,
                                      PRUint32 aLength);

    
    

    
    PangoFont *GetBasePangoFont();

    
    PangoLanguage *GetPangoLanguage() { return mPangoLanguage; }

    
    
    
    gfxFcFontSet *GetFontSet(PangoLanguage *aLang = NULL);

private:
    class FontSetByLangEntry {
    public:
        FontSetByLangEntry(PangoLanguage *aLang, gfxFcFontSet *aFontSet);
        PangoLanguage *mLang;
        nsRefPtr<gfxFcFontSet> mFontSet;
    };
    
    
    nsAutoTArray<FontSetByLangEntry,1> mFontSets;

    gfxFloat mSizeAdjustFactor;
    PangoLanguage *mPangoLanguage;

    

    





    void InitTextRun(gfxTextRun *aTextRun, const PRUnichar *aString,
                     PRUint32 aLength, PRBool aTake8BitPath);

    void CreateGlyphRunsItemizing(gfxTextRun *aTextRun,
                                  const PRUnichar *aString, PRUint32 aLength);
#if defined(ENABLE_FAST_PATH_8BIT)
    nsresult CreateGlyphRunsFast(gfxTextRun *aTextRun,
                                 const PRUnichar *aString, PRUint32 aLength);
#endif

    void GetFcFamilies(nsTArray<nsString> *aFcFamilyList,
                       nsIAtom *aLanguage);

    
    
    
    already_AddRefed<gfxFcFontSet>
    MakeFontSet(PangoLanguage *aLang, gfxFloat aSizeAdjustFactor,
                nsAutoRef<FcPattern> *aMatchPattern = NULL);

    gfxFcFontSet *GetBaseFontSet();
    gfxFcFont *GetBaseFont();

    gfxFloat GetSizeAdjustFactor()
    {
        if (mFontSets.Length() == 0)
            GetBaseFontSet();
        return mSizeAdjustFactor;
    }

    static FT_Library GetFTLibrary();
};

#endif 
