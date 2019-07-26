




#ifndef GFX_PANGOFONTS_H
#define GFX_PANGOFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"

#include "nsAutoRef.h"
#include "nsTArray.h"

#include <pango/pango.h>

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

    virtual gfxFont *GetFontAt(int32_t i);

    virtual void UpdateFontList();

    virtual already_AddRefed<gfxFont>
        FindFontForChar(uint32_t aCh, uint32_t aPrevCh, int32_t aRunScript,
                        gfxFont *aPrevMatchedFont,
                        uint8_t *aMatchType);

    static void Shutdown();

    
    static gfxFontEntry *NewFontEntry(const gfxProxyFontEntry &aProxyEntry,
                                      const nsAString &aFullname);
    
    static gfxFontEntry *NewFontEntry(const gfxProxyFontEntry &aProxyEntry,
                                      const uint8_t *aFontData,
                                      uint32_t aLength);

    
    

    
    PangoLanguage *GetPangoLanguage() { return mPangoLanguage; }

private:
    
    
    
    gfxFcFontSet *GetFontSet(PangoLanguage *aLang = NULL);

    class FontSetByLangEntry {
    public:
        FontSetByLangEntry(PangoLanguage *aLang, gfxFcFontSet *aFontSet);
        PangoLanguage *mLang;
        nsRefPtr<gfxFcFontSet> mFontSet;
    };
    
    
    nsAutoTArray<FontSetByLangEntry,1> mFontSets;

    gfxFloat mSizeAdjustFactor;
    PangoLanguage *mPangoLanguage;

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

    friend class gfxSystemFcFontEntry;
    static FT_Library GetFTLibrary();
};

#endif 
