







































#ifndef GFX_GDIFONTLIST_H
#define GFX_GDIFONTLIST_H

#include "gfxWindowsPlatform.h"
#include "gfxPlatformFontList.h"
#include "gfxAtoms.h"

#include <windows.h>

class AutoDC 
{
public:
    AutoDC() {
        mDC = ::GetDC(NULL);
    }

    ~AutoDC() {
        ::ReleaseDC(NULL, mDC);
    }

    HDC GetDC() {
        return mDC;
    }

private:
    HDC mDC;
};

class AutoSelectFont 
{
public:
    AutoSelectFont(HDC aDC, LOGFONTW *aLogFont)
        : mOwnsFont(PR_FALSE)
    {
        mFont = ::CreateFontIndirectW(aLogFont);
        if (mFont) {
            mOwnsFont = PR_TRUE;
            mDC = aDC;
            mOldFont = (HFONT)::SelectObject(aDC, mFont);
        } else {
            mOldFont = NULL;
        }
    }

    AutoSelectFont(HDC aDC, HFONT aFont)
        : mOwnsFont(PR_FALSE)
    {
        mDC = aDC;
        mFont = aFont;
        mOldFont = (HFONT)::SelectObject(aDC, aFont);
    }

    ~AutoSelectFont() {
        if (mOldFont) {
            ::SelectObject(mDC, mOldFont);
            if (mOwnsFont) {
                ::DeleteObject(mFont);
            }
        }
    }

    PRBool IsValid() const {
        return mFont != NULL;
    }

    HFONT GetFont() const {
        return mFont;
    }

private:
    HDC    mDC;
    HFONT  mFont;
    HFONT  mOldFont;
    PRBool mOwnsFont;
};











enum gfxWindowsFontType {
    GFX_FONT_TYPE_UNKNOWN = 0,
    GFX_FONT_TYPE_DEVICE,
    GFX_FONT_TYPE_RASTER,
    GFX_FONT_TYPE_TRUETYPE,
    GFX_FONT_TYPE_PS_OPENTYPE,
    GFX_FONT_TYPE_TT_OPENTYPE,
    GFX_FONT_TYPE_TYPE1
};




class GDIFontEntry : public gfxFontEntry
{
public:
    LPLOGFONTW GetLogFont() { return &mLogFont; }

    nsresult ReadCMAP();

    virtual PRBool IsSymbolFont();

    void FillLogFont(LOGFONTW *aLogFont, PRBool aItalic,
                     PRUint16 aWeight, gfxFloat aSize, PRBool aUseCleartype);

    static gfxWindowsFontType DetermineFontType(const NEWTEXTMETRICW& metrics, 
                                                DWORD fontType)
    {
        gfxWindowsFontType feType;
        if (metrics.ntmFlags & NTM_TYPE1)
            feType = GFX_FONT_TYPE_TYPE1;
        else if (metrics.ntmFlags & NTM_PS_OPENTYPE)
            feType = GFX_FONT_TYPE_PS_OPENTYPE;
        else if (metrics.ntmFlags & NTM_TT_OPENTYPE)
            feType = GFX_FONT_TYPE_TT_OPENTYPE;
        else if (fontType == TRUETYPE_FONTTYPE)
            feType = GFX_FONT_TYPE_TRUETYPE;
        else if (fontType == RASTER_FONTTYPE)
            feType = GFX_FONT_TYPE_RASTER;
        else if (fontType == DEVICE_FONTTYPE)
            feType = GFX_FONT_TYPE_DEVICE;
        else
            feType = GFX_FONT_TYPE_UNKNOWN;
        
        return feType;
    }

    PRBool IsType1() const {
        return (mFontType == GFX_FONT_TYPE_TYPE1);
    }

    PRBool IsTrueType() const {
        return (mFontType == GFX_FONT_TYPE_TRUETYPE ||
                mFontType == GFX_FONT_TYPE_PS_OPENTYPE ||
                mFontType == GFX_FONT_TYPE_TT_OPENTYPE);
    }

    virtual PRBool MatchesGenericFamily(const nsACString& aGeneric) const {
        if (aGeneric.IsEmpty()) {
            return PR_TRUE;
        }

        
        
        if (mWindowsFamily == FF_ROMAN && mWindowsPitch & FIXED_PITCH) {
            return aGeneric.EqualsLiteral("monospace");
        }

        
        
        if (mWindowsFamily == FF_MODERN && mWindowsPitch & VARIABLE_PITCH) {
            return aGeneric.EqualsLiteral("sans-serif");
        }

        
        switch (mWindowsFamily) {
        case FF_DONTCARE:
            return PR_FALSE;
        case FF_ROMAN:
            return aGeneric.EqualsLiteral("serif");
        case FF_SWISS:
            return aGeneric.EqualsLiteral("sans-serif");
        case FF_MODERN:
            return aGeneric.EqualsLiteral("monospace");
        case FF_SCRIPT:
            return aGeneric.EqualsLiteral("cursive");
        case FF_DECORATIVE:
            return aGeneric.EqualsLiteral("fantasy");
        }

        return PR_FALSE;
    }

    virtual PRBool SupportsLangGroup(nsIAtom* aLangGroup) const {
        if (!aLangGroup || aLangGroup == gfxAtoms::x_unicode) {
            return PR_TRUE;
        }

        PRInt16 bit = -1;

        
        if (aLangGroup == gfxAtoms::x_western) {
            bit = ANSI_CHARSET;
        } else if (aLangGroup == gfxAtoms::ja) {
            bit = SHIFTJIS_CHARSET;
        } else if (aLangGroup == gfxAtoms::ko) {
            bit = HANGEUL_CHARSET;
        } else if (aLangGroup == gfxAtoms::ko_xxx) {
            bit = JOHAB_CHARSET;
        } else if (aLangGroup == gfxAtoms::zh_cn) {
            bit = GB2312_CHARSET;
        } else if (aLangGroup == gfxAtoms::zh_tw) {
            bit = CHINESEBIG5_CHARSET;
        } else if (aLangGroup == gfxAtoms::el) {
            bit = GREEK_CHARSET;
        } else if (aLangGroup == gfxAtoms::tr) {
            bit = TURKISH_CHARSET;
        } else if (aLangGroup == gfxAtoms::he) {
            bit = HEBREW_CHARSET;
        } else if (aLangGroup == gfxAtoms::ar) {
            bit = ARABIC_CHARSET;
        } else if (aLangGroup == gfxAtoms::x_baltic) {
            bit = BALTIC_CHARSET;
        } else if (aLangGroup == gfxAtoms::x_cyrillic) {
            bit = RUSSIAN_CHARSET;
        } else if (aLangGroup == gfxAtoms::th) {
            bit = THAI_CHARSET;
        } else if (aLangGroup == gfxAtoms::x_central_euro) {
            bit = EASTEUROPE_CHARSET;
        } else if (aLangGroup == gfxAtoms::x_symbol) {
            bit = SYMBOL_CHARSET;
        }

        if (bit != -1) {
            return mCharset.test(bit);
        }

        return PR_FALSE;
    }

    virtual PRBool SupportsRange(PRUint8 range) {
        return mUnicodeRanges.test(range);
    }

    virtual PRBool SkipDuringSystemFallback() { 
        return !HasCmapTable(); 
    }

    virtual PRBool TestCharacterMap(PRUint32 aCh);

    
    static GDIFontEntry* CreateFontEntry(const nsAString& aName, 
                                      gfxWindowsFontType aFontType, 
                                      PRBool aItalic, PRUint16 aWeight, 
                                      gfxUserFontData* aUserFontData);

    
    static GDIFontEntry* LoadLocalFont(const gfxProxyFontEntry &aProxyEntry,
                                    const nsAString& aFullname);

    PRUint8 mWindowsFamily;
    PRUint8 mWindowsPitch;

    gfxWindowsFontType mFontType;
    PRPackedBool mForceGDI    : 1;
    PRPackedBool mUnknownCMAP : 1;

    gfxSparseBitSet mCharset;
    gfxSparseBitSet mUnicodeRanges;

protected:
    friend class gfxWindowsFont;

    GDIFontEntry(const nsAString& aFaceName, gfxWindowsFontType aFontType,
                     PRBool aItalic, PRUint16 aWeight, gfxUserFontData *aUserFontData);

    void InitLogFont(const nsAString& aName, gfxWindowsFontType aFontType);

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle, PRBool aNeedsBold);

    virtual nsresult GetFontTable(PRUint32 aTableTag,
                                  FallibleTArray<PRUint8>& aBuffer);

    LOGFONTW mLogFont;
};


class GDIFontFamily : public gfxFontFamily
{
public:
    GDIFontFamily(nsAString &aName) :
        gfxFontFamily(aName) {}

    virtual void FindStyleVariations();

private:
    static int CALLBACK FamilyAddStylesProc(const ENUMLOGFONTEXW *lpelfe,
                                            const NEWTEXTMETRICEXW *nmetrics,
                                            DWORD fontType, LPARAM data);
};

class gfxGDIFontList : public gfxPlatformFontList {
public:
    static gfxGDIFontList* PlatformFontList() {
        return static_cast<gfxGDIFontList*>(sPlatformFontList);
    }

    
    virtual nsresult InitFontList();

    virtual gfxFontEntry* GetDefaultFont(const gfxFontStyle* aStyle, PRBool& aNeedsBold);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData, PRUint32 aLength);

    virtual PRBool ResolveFontName(const nsAString& aFontName,
                                   nsAString& aResolvedFontName);

private:
    friend class gfxWindowsPlatform;

    gfxGDIFontList();

    void InitializeFontEmbeddingProcs();

    nsresult GetFontSubstitutes();

    static int CALLBACK EnumFontFamExProc(ENUMLOGFONTEXW *lpelfe,
                                          NEWTEXTMETRICEXW *lpntme,
                                          DWORD fontType,
                                          LPARAM lParam);

    typedef nsDataHashtable<nsStringHashKey, nsRefPtr<gfxFontFamily> > FontTable;

    FontTable mFontSubstitutes;
    nsTArray<nsString> mNonExistingFonts;
};

#endif 
