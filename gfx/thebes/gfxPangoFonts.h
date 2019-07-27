




#ifndef GFX_PANGOFONTS_H
#define GFX_PANGOFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxTextRun.h"

#include "nsAutoRef.h"
#include "nsTArray.h"

#include <pango/pango.h>

class gfxFcFontSet;
class gfxFcFont;
typedef struct _FcPattern FcPattern;
typedef struct FT_FaceRec_* FT_Face;
typedef struct FT_LibraryRec_  *FT_Library;

class gfxPangoFontGroup : public gfxFontGroup {
public:
    gfxPangoFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                      const gfxFontStyle *aStyle,
                      gfxUserFontSet *aUserFontSet);
    virtual ~gfxPangoFontGroup();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    virtual gfxFont* GetFirstValidFont(uint32_t aCh = 0x20);

    virtual void UpdateUserFonts();

    virtual already_AddRefed<gfxFont>
        FindFontForChar(uint32_t aCh, uint32_t aPrevCh, uint32_t aNextCh,
                        int32_t aRunScript, gfxFont *aPrevMatchedFont,
                        uint8_t *aMatchType);

    static void Shutdown();

    
    static gfxFontEntry *NewFontEntry(const nsAString& aFontName,
                                      uint16_t aWeight,
                                      int16_t aStretch,
                                      bool aItalic);
    
    static gfxFontEntry *NewFontEntry(const nsAString& aFontName,
                                      uint16_t aWeight,
                                      int16_t aStretch,
                                      bool aItalic,
                                      const uint8_t* aFontData,
                                      uint32_t aLength);

private:

    virtual gfxFont *GetFontAt(int32_t i, uint32_t aCh = 0x20);

    
    
    
    
    gfxFcFontSet *GetFontSet(PangoLanguage *aLang = nullptr);

    class FontSetByLangEntry {
    public:
        FontSetByLangEntry(PangoLanguage *aLang, gfxFcFontSet *aFontSet);
        PangoLanguage *mLang;
        nsRefPtr<gfxFcFontSet> mFontSet;
    };
    
    
    nsAutoTArray<FontSetByLangEntry,1> mFontSets;

    gfxFloat mSizeAdjustFactor;
    PangoLanguage *mPangoLanguage;

    
    
    
    already_AddRefed<gfxFcFontSet>
    MakeFontSet(PangoLanguage *aLang, gfxFloat aSizeAdjustFactor,
                nsAutoRef<FcPattern> *aMatchPattern = nullptr);

    gfxFcFontSet *GetBaseFontSet();
    gfxFcFont *GetBaseFont();

    gfxFloat GetSizeAdjustFactor()
    {
        if (mFontSets.Length() == 0)
            GetBaseFontSet();
        return mSizeAdjustFactor;
    }

    virtual void FindPlatformFont(const nsAString& aName,
                                  bool aUseFontSet,
                                  void *aClosure);

    friend class gfxSystemFcFontEntry;
    static FT_Library GetFTLibrary();
};

#endif 
