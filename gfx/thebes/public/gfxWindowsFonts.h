





































#ifndef GFX_WINDOWSFONTS_H
#define GFX_WINDOWSFONTS_H

#include "prtypes.h"
#include "gfxTypes.h"
#include "gfxColor.h"
#include "gfxFont.h"
#include "gfxMatrix.h"
#include "gfxFontUtils.h"

#include "nsDataHashtable.h"

#include <usp10.h>
#include <cairo-win32.h>







class FontEntry
{
public:
    THEBES_INLINE_DECL_REFCOUNTING(FontEntry)

    FontEntry(const nsAString& aName, PRUint16 aFontType) : 
        mName(aName), mFontType(aFontType), mDefaultWeight(0),
        mUnicodeFont(PR_FALSE), mSymbolFont(PR_FALSE), mIsBadUnderlineFont(PR_FALSE),
        mCharset(0), mUnicodeRanges(0)
    {
    }

    PRBool IsCrappyFont() const {
        
        return (!mUnicodeFont || mSymbolFont || mFontType != TRUETYPE_FONTTYPE);
    }

    PRBool MatchesGenericFamily(const nsACString& aGeneric) const {
        if (aGeneric.IsEmpty())
            return PR_TRUE;

        
        
        if (mFamily == FF_ROMAN && mPitch & FIXED_PITCH) {
            return aGeneric.EqualsLiteral("monospace");
        }

        
        
        if (mFamily == FF_MODERN && mPitch & VARIABLE_PITCH) {
            return aGeneric.EqualsLiteral("sans-serif");
        }

        
        switch (mFamily) {
        case FF_DONTCARE:
            return PR_TRUE;
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

    PRBool SupportsLangGroup(const nsACString& aLangGroup) const {
        if (aLangGroup.IsEmpty())
            return PR_TRUE;

        PRInt16 bit = -1;

        
        if (aLangGroup.EqualsLiteral("x-western")) {
            bit = ANSI_CHARSET;
        } else if (aLangGroup.EqualsLiteral("ja")) {
            bit = SHIFTJIS_CHARSET;
        } else if (aLangGroup.EqualsLiteral("ko")) {
            bit = HANGEUL_CHARSET;
        } else if (aLangGroup.EqualsLiteral("ko-XXX")) {
            bit = JOHAB_CHARSET;
        } else if (aLangGroup.EqualsLiteral("zh-CN")) {
            bit = GB2312_CHARSET;
        } else if (aLangGroup.EqualsLiteral("zh-TW")) {
            bit = CHINESEBIG5_CHARSET;
        } else if (aLangGroup.EqualsLiteral("el")) {
            bit = GREEK_CHARSET;
        } else if (aLangGroup.EqualsLiteral("tr")) {
            bit = TURKISH_CHARSET;
        } else if (aLangGroup.EqualsLiteral("he")) {
            bit = HEBREW_CHARSET;
        } else if (aLangGroup.EqualsLiteral("ar")) {
            bit = ARABIC_CHARSET;
        } else if (aLangGroup.EqualsLiteral("x-baltic")) {
            bit = BALTIC_CHARSET;
        } else if (aLangGroup.EqualsLiteral("x-cyrillic")) {
            bit = RUSSIAN_CHARSET;
        } else if (aLangGroup.EqualsLiteral("th")) {
            bit = THAI_CHARSET;
        } else if (aLangGroup.EqualsLiteral("x-central-euro")) {
            bit = EASTEUROPE_CHARSET;
        } else if (aLangGroup.EqualsLiteral("x-symbol")) {
            bit = SYMBOL_CHARSET;
        }

        if (bit != -1)
            return mCharset[bit];

        return PR_FALSE;
    }

    PRBool SupportsRange(PRUint8 range) {
        return mUnicodeRanges[range];
    }

    class WeightTable
    {
    public:
        THEBES_INLINE_DECL_REFCOUNTING(WeightTable)
            
        WeightTable() : mWeights(0) {}
        ~WeightTable() {}
        PRBool TriedWeight(PRUint8 aWeight) {
            return mWeights[aWeight - 1 + 10];
        }
        PRBool HasWeight(PRUint8 aWeight) {
            return mWeights[aWeight - 1];
        }
        void SetWeight(PRUint8 aWeight, PRBool aValue) {
            mWeights[aWeight - 1] = aValue;
            mWeights[aWeight - 1 + 10] = PR_TRUE;
        }
    private:
        std::bitset<20> mWeights;
    };

    
    PRBool IsBadUnderlineFont() { return mIsBadUnderlineFont != 0; }

    
    nsString mName;

    PRUint16 mFontType;
    PRUint16 mDefaultWeight;

    PRUint8 mFamily;
    PRUint8 mPitch;
    PRPackedBool mUnicodeFont;
    PRPackedBool mSymbolFont;
    PRPackedBool mIsBadUnderlineFont;

    std::bitset<256> mCharset;
    std::bitset<128> mUnicodeRanges;

    WeightTable mWeightTable;

    gfxSparseBitSet mCharacterMap;
};








class gfxWindowsFont : public gfxFont {
public:
    gfxWindowsFont(const nsAString& aName, const gfxFontStyle *aFontStyle);
    virtual ~gfxWindowsFont();

    virtual const gfxFont::Metrics& GetMetrics();

    HFONT GetHFONT() { return mFont; }
    cairo_font_face_t *CairoFontFace();
    cairo_scaled_font_t *CairoScaledFont();
    SCRIPT_CACHE *ScriptCache() { return &mScriptCache; }
    gfxFloat GetAdjustedSize() { MakeHFONT(); return mAdjustedSize; }

    virtual nsString GetUniqueName();

    virtual void Draw(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aEnd,
                      gfxContext *aContext, PRBool aDrawToPath, gfxPoint *aBaselineOrigin,
                      Spacing *aSpacing);

    virtual PRUint32 GetSpaceGlyph() {
        GetMetrics(); 
        return mSpaceGlyph;
    };

    FontEntry *GetFontEntry() { return mFontEntry; }

protected:
    HFONT MakeHFONT();
    void FillLogFont(gfxFloat aSize, PRInt16 aWeight);

    HFONT    mFont;
    gfxFloat mAdjustedSize;
    PRUint32 mSpaceGlyph;

private:
    void ComputeMetrics();

    SCRIPT_CACHE mScriptCache;

    cairo_font_face_t *mFontFace;
    cairo_scaled_font_t *mScaledFont;

    gfxFont::Metrics *mMetrics;

    LOGFONTW mLogFont;

    nsRefPtr<FontEntry> mFontEntry;
    
    virtual PRBool SetupCairoFont(gfxContext *aContext);
};







class THEBES_API gfxWindowsFontGroup : public gfxFontGroup {

public:
    gfxWindowsFontGroup(const nsAString& aFamilies, const gfxFontStyle* aStyle);
    virtual ~gfxWindowsFontGroup();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    virtual gfxTextRun *MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);

    const nsACString& GetGenericFamily() const {
        return mGenericFamily;
    }

    const nsTArray<nsRefPtr<FontEntry> >& GetFontList() const {
        return mFontEntries;
    }
    PRUint32 FontListLength() const {
        return mFontEntries.Length();
    }

    FontEntry *GetFontEntryAt(PRInt32 i) {
        return mFontEntries[i];
    }

    virtual gfxWindowsFont *GetFontAt(PRInt32 i);

protected:
    void InitTextRunGDI(gfxContext *aContext, gfxTextRun *aRun, const char *aString, PRUint32 aLength);
    void InitTextRunGDI(gfxContext *aContext, gfxTextRun *aRun, const PRUnichar *aString, PRUint32 aLength);

    void InitTextRunUniscribe(gfxContext *aContext, gfxTextRun *aRun, const PRUnichar *aString, PRUint32 aLength);

private:
    nsCString mGenericFamily;
    nsTArray<nsRefPtr<FontEntry> > mFontEntries;
};

#endif 
