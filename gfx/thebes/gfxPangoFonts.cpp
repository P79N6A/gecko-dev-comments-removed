




#include "prlink.h"
#include "gfxTypes.h"

#include "nsTArray.h"

#include "gfxContext.h"
#ifdef MOZ_WIDGET_GTK
#include "gfxPlatformGtk.h"
#endif
#ifdef MOZ_WIDGET_QT
#include "gfxQtPlatform.h"
#endif
#include "gfxPangoFonts.h"
#include "gfxFT2FontBase.h"
#include "gfxFT2Utils.h"
#include "harfbuzz/hb.h"
#include "harfbuzz/hb-ot.h"
#include "nsUnicodeProperties.h"
#include "nsUnicodeScriptCodes.h"
#include "gfxFontconfigUtils.h"
#include "gfxUserFontSet.h"
#include "gfxFontConstants.h"

#include <cairo.h>
#include <cairo-ft.h>

#include <fontconfig/fcfreetype.h>
#include <pango/pango.h>

#include FT_TRUETYPE_TABLES_H

#ifdef MOZ_WIDGET_GTK
#include <gdk/gdk.h>
#endif

#include <math.h>

using namespace mozilla;
using namespace mozilla::unicode;

#define PRINTING_FC_PROPERTY "gfx.printing"

static PangoLanguage *GuessPangoLanguage(nsIAtom *aLanguage);

static cairo_scaled_font_t *
CreateScaledFont(FcPattern *aPattern, cairo_font_face_t *aFace);

static FT_Library gFTLibrary;



#ifndef FC_FAMILYLANG
#define FC_FAMILYLANG "familylang"
#endif
#ifndef FC_FULLNAME
#define FC_FULLNAME "fullname"
#endif

static PRFuncPtr
FindFunctionSymbol(const char *name)
{
    PRLibrary *lib = nullptr;
    PRFuncPtr result = PR_FindFunctionSymbolAndLibrary(name, &lib);
    if (lib) {
        PR_UnloadLibrary(lib);
    }

    return result;
}

static bool HasChar(FcPattern *aFont, FcChar32 wc)
{
    FcCharSet *charset = nullptr;
    FcPatternGetCharSet(aFont, FC_CHARSET, 0, &charset);

    return charset && FcCharSetHasChar(charset, wc);
}








class gfxFcFontEntry : public gfxFontEntry {
public:
    
    
    
    
    
    
    const nsTArray< nsCountedRef<FcPattern> >& GetPatterns()
    {
        return mPatterns;
    }

    static gfxFcFontEntry *LookupFontEntry(cairo_font_face_t *aFace)
    {
        return static_cast<gfxFcFontEntry*>
            (cairo_font_face_get_user_data(aFace, &sFontEntryKey));
    }

    
    
    
    virtual nsString RealFaceName();

    
    virtual bool TestCharacterMap(uint32_t aCh)
    {
        for (uint32_t i = 0; i < mPatterns.Length(); ++i) {
            if (HasChar(mPatterns[i], aCh)) {
                return true;
            }
        }
        return false;
    }

protected:
    explicit gfxFcFontEntry(const nsAString& aName)
        : gfxFontEntry(aName)
    {
    }

    
    
    AutoFallibleTArray<nsCountedRef<FcPattern>,1> mPatterns;

    static cairo_user_data_key_t sFontEntryKey;
};

cairo_user_data_key_t gfxFcFontEntry::sFontEntryKey;

nsString
gfxFcFontEntry::RealFaceName()
{
    FcChar8 *name;
    if (!mPatterns.IsEmpty()) {
        if (FcPatternGetString(mPatterns[0],
                               FC_FULLNAME, 0, &name) == FcResultMatch) {
            return NS_ConvertUTF8toUTF16((const char*)name);
        }
        if (FcPatternGetString(mPatterns[0],
                               FC_FAMILY, 0, &name) == FcResultMatch) {
            NS_ConvertUTF8toUTF16 result((const char*)name);
            if (FcPatternGetString(mPatterns[0],
                                   FC_STYLE, 0, &name) == FcResultMatch) {
                result.Append(' ');
                AppendUTF8toUTF16((const char*)name, result);
            }
            return result;
        }
    }
    
    return gfxFontEntry::RealFaceName();
}











class gfxSystemFcFontEntry : public gfxFcFontEntry {
public:
    
    
    gfxSystemFcFontEntry(cairo_font_face_t *aFontFace,
                         FcPattern *aFontPattern,
                         const nsAString& aName)
        : gfxFcFontEntry(aName), mFontFace(aFontFace),
          mFTFace(nullptr), mFTFaceInitialized(false)
    {
        cairo_font_face_reference(mFontFace);
        cairo_font_face_set_user_data(mFontFace, &sFontEntryKey, this, nullptr);
        mPatterns.AppendElement();
        
        
        mPatterns[0] = aFontPattern;

        FcChar8 *name;
        if (FcPatternGetString(aFontPattern,
                               FC_FAMILY, 0, &name) == FcResultMatch) {
            mFamilyName = NS_ConvertUTF8toUTF16((const char*)name);
        }
    }

    ~gfxSystemFcFontEntry()
    {
        cairo_font_face_set_user_data(mFontFace,
                                      &sFontEntryKey,
                                      nullptr,
                                      nullptr);
        cairo_font_face_destroy(mFontFace);
    }

    virtual void ForgetHBFace() override;
    virtual void ReleaseGrFace(gr_face* aFace) override;

protected:
    virtual nsresult
    CopyFontTable(uint32_t aTableTag, FallibleTArray<uint8_t>& aBuffer) override;

    void MaybeReleaseFTFace();

private:
    cairo_font_face_t *mFontFace;
    FT_Face            mFTFace;
    bool               mFTFaceInitialized;
};

nsresult
gfxSystemFcFontEntry::CopyFontTable(uint32_t aTableTag,
                                    FallibleTArray<uint8_t>& aBuffer)
{
    if (!mFTFaceInitialized) {
        mFTFaceInitialized = true;
        FcChar8 *filename;
        if (FcPatternGetString(mPatterns[0], FC_FILE, 0, &filename) != FcResultMatch) {
            return NS_ERROR_FAILURE;
        }
        int index;
        if (FcPatternGetInteger(mPatterns[0], FC_INDEX, 0, &index) != FcResultMatch) {
            index = 0; 
        }
        if (FT_New_Face(gfxPangoFontGroup::GetFTLibrary(),
                        (const char*)filename, index, &mFTFace) != 0) {
            return NS_ERROR_FAILURE;
        }
    }

    if (!mFTFace) {
        return NS_ERROR_NOT_AVAILABLE;
    }

    FT_ULong length = 0;
    if (FT_Load_Sfnt_Table(mFTFace, aTableTag, 0, nullptr, &length) != 0) {
        return NS_ERROR_NOT_AVAILABLE;
    }
    if (!aBuffer.SetLength(length)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    if (FT_Load_Sfnt_Table(mFTFace, aTableTag, 0, aBuffer.Elements(), &length) != 0) {
        aBuffer.Clear();
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

void
gfxSystemFcFontEntry::MaybeReleaseFTFace()
{
    
    if (mHBFace || mGrFace) {
        return;
    }
    if (mFTFace) {
        FT_Done_Face(mFTFace);
        mFTFace = nullptr;
    }
    mFTFaceInitialized = false;
}

void
gfxSystemFcFontEntry::ForgetHBFace()
{
    gfxFontEntry::ForgetHBFace();
    MaybeReleaseFTFace();
}

void
gfxSystemFcFontEntry::ReleaseGrFace(gr_face* aFace)
{
    gfxFontEntry::ReleaseGrFace(aFace);
    MaybeReleaseFTFace();
}





#define FONT_FACE_FAMILY_PREFIX "@font-face:"
































class gfxUserFcFontEntry : public gfxFcFontEntry {
protected:
    explicit gfxUserFcFontEntry(const nsAString& aFontName,
                       uint16_t aWeight,
                       int16_t aStretch,
                       bool aItalic)
        : gfxFcFontEntry(aFontName)
    {
        mItalic = aItalic;
        mWeight = aWeight;
        mStretch = aStretch;
    }

    
    
    
    
    
    void AdjustPatternToCSS(FcPattern *aPattern);
};

void
gfxUserFcFontEntry::AdjustPatternToCSS(FcPattern *aPattern)
{
    int fontWeight = -1;
    FcPatternGetInteger(aPattern, FC_WEIGHT, 0, &fontWeight);
    int cssWeight = gfxFontconfigUtils::FcWeightForBaseWeight(mWeight / 100);
    if (cssWeight != fontWeight) {
        FcPatternDel(aPattern, FC_WEIGHT);
        FcPatternAddInteger(aPattern, FC_WEIGHT, cssWeight);
    }

    int fontSlant;
    FcResult res = FcPatternGetInteger(aPattern, FC_SLANT, 0, &fontSlant);
    
    
    if (res != FcResultMatch ||
        IsItalic() != (fontSlant != FC_SLANT_ROMAN)) {
        FcPatternDel(aPattern, FC_SLANT);
        FcPatternAddInteger(aPattern, FC_SLANT,
                            IsItalic() ? FC_SLANT_OBLIQUE : FC_SLANT_ROMAN);
    }

    int fontWidth = -1;
    FcPatternGetInteger(aPattern, FC_WIDTH, 0, &fontWidth);
    int cssWidth = gfxFontconfigUtils::FcWidthForThebesStretch(mStretch);
    if (cssWidth != fontWidth) {
        FcPatternDel(aPattern, FC_WIDTH);
        FcPatternAddInteger(aPattern, FC_WIDTH, cssWidth);
    }

    
    
    
    FcChar8 *unused;
    if (FcPatternGetString(aPattern,
                           FC_FULLNAME, 0, &unused) == FcResultNoMatch) {
        nsAutoCString fullname;
        if (gfxFontconfigUtils::GetFullnameFromFamilyAndStyle(aPattern,
                                                              &fullname)) {
            FcPatternAddString(aPattern, FC_FULLNAME,
                               gfxFontconfigUtils::ToFcChar8(fullname));
        }
    }

    nsAutoCString family;
    family.Append(FONT_FACE_FAMILY_PREFIX);
    AppendUTF16toUTF8(Name(), family);

    FcPatternDel(aPattern, FC_FAMILY);
    FcPatternDel(aPattern, FC_FAMILYLANG);
    FcPatternAddString(aPattern, FC_FAMILY,
                       gfxFontconfigUtils::ToFcChar8(family));
}












class gfxLocalFcFontEntry : public gfxUserFcFontEntry {
public:
    gfxLocalFcFontEntry(const nsAString& aFontName,
                        uint16_t aWeight,
                        int16_t aStretch,
                        bool aItalic,
                        const nsTArray< nsCountedRef<FcPattern> >& aPatterns)
        : gfxUserFcFontEntry(aFontName, aWeight, aStretch, aItalic)
    {
        if (!mPatterns.SetCapacity(aPatterns.Length()))
            return; 

        for (uint32_t i = 0; i < aPatterns.Length(); ++i) {
            FcPattern *pattern = FcPatternDuplicate(aPatterns.ElementAt(i));
            if (!pattern)
                return; 

            AdjustPatternToCSS(pattern);

            mPatterns.AppendElement();
            mPatterns[i].own(pattern);
        }
        mIsLocalUserFont = true;
    }
};










class gfxDownloadedFcFontEntry : public gfxUserFcFontEntry {
public:
    
    gfxDownloadedFcFontEntry(const nsAString& aFontName,
                             uint16_t aWeight,
                             int16_t aStretch,
                             bool aItalic,
                             const uint8_t *aData, FT_Face aFace)
        : gfxUserFcFontEntry(aFontName, aWeight, aStretch, aItalic),
          mFontData(aData), mFace(aFace)
    {
        NS_PRECONDITION(aFace != nullptr, "aFace is NULL!");
        mIsDataUserFont = true;
        InitPattern();
    }

    virtual ~gfxDownloadedFcFontEntry();

    
    bool SetCairoFace(cairo_font_face_t *aFace);

    virtual hb_blob_t* GetFontTable(uint32_t aTableTag) override;

protected:
    void InitPattern();

    
    
    
    const uint8_t* mFontData;

    FT_Face mFace;
};


static const char *kFontEntryFcProp = "-moz-font-entry";

static FcBool AddDownloadedFontEntry(FcPattern *aPattern,
                                     gfxDownloadedFcFontEntry *aFontEntry)
{
    FcValue value;
    value.type = FcTypeFTFace; 
    value.u.f = aFontEntry;

    return FcPatternAdd(aPattern, kFontEntryFcProp, value, FcFalse);
}

static FcBool DelDownloadedFontEntry(FcPattern *aPattern)
{
    return FcPatternDel(aPattern, kFontEntryFcProp);
}

static gfxDownloadedFcFontEntry *GetDownloadedFontEntry(FcPattern *aPattern)
{
    FcValue value;
    if (FcPatternGet(aPattern, kFontEntryFcProp, 0, &value) != FcResultMatch)
        return nullptr;

    if (value.type != FcTypeFTFace) {
        NS_NOTREACHED("Wrong type for -moz-font-entry font property");
        return nullptr;
    }

    return static_cast<gfxDownloadedFcFontEntry*>(value.u.f);
}

gfxDownloadedFcFontEntry::~gfxDownloadedFcFontEntry()
{
    if (mPatterns.Length() != 0) {
        
        
        NS_ASSERTION(mPatterns.Length() == 1,
                     "More than one pattern in gfxDownloadedFcFontEntry!");
        DelDownloadedFontEntry(mPatterns[0]);
        FcPatternDel(mPatterns[0], FC_FT_FACE);
    }
    FT_Done_Face(mFace);
    NS_Free((void*)mFontData);
}

typedef FcPattern* (*QueryFaceFunction)(const FT_Face face,
                                        const FcChar8 *file, int id,
                                        FcBlanks *blanks);

void
gfxDownloadedFcFontEntry::InitPattern()
{
    static QueryFaceFunction sQueryFacePtr =
        reinterpret_cast<QueryFaceFunction>
        (FindFunctionSymbol("FcFreeTypeQueryFace"));
    FcPattern *pattern;

    
    
    
    
    
    
    
    if (sQueryFacePtr) {
        
        
        
        
        
        
        
        
        
        pattern =
            (*sQueryFacePtr)(mFace,
                             gfxFontconfigUtils::ToFcChar8(""),
                             0,
                             nullptr);
        if (!pattern)
            
            
            
            
            return;

        
        FcPatternDel(pattern, FC_FILE);
        FcPatternDel(pattern, FC_INDEX);

    } else {
        

        
        nsAutoRef<FcCharSet> charset(FcFreeTypeCharSet(mFace, nullptr));
        
        
        if (!charset || FcCharSetCount(charset) == 0)
            return;

        pattern = FcPatternCreate();
        FcPatternAddCharSet(pattern, FC_CHARSET, charset);

        
        
        if (!(mFace->face_flags & FT_FACE_FLAG_SCALABLE)) {
            for (FT_Int i = 0; i < mFace->num_fixed_sizes; ++i) {
#if HAVE_FT_BITMAP_SIZE_Y_PPEM
                double size = FLOAT_FROM_26_6(mFace->available_sizes[i].y_ppem);
#else
                double size = mFace->available_sizes[i].height;
#endif
                FcPatternAddDouble (pattern, FC_PIXEL_SIZE, size);
            }

            
            
            FcPatternAddBool (pattern, FC_ANTIALIAS, FcFalse);
        }

        
        
        
        
        
        
        
        
        
    }

    AdjustPatternToCSS(pattern);

    FcPatternAddFTFace(pattern, FC_FT_FACE, mFace);
    AddDownloadedFontEntry(pattern, this);

    
    mPatterns.AppendElement();
    mPatterns[0].own(pattern);
}

static void ReleaseDownloadedFontEntry(void *data)
{
    gfxDownloadedFcFontEntry *downloadedFontEntry =
        static_cast<gfxDownloadedFcFontEntry*>(data);
    NS_RELEASE(downloadedFontEntry);
}

bool gfxDownloadedFcFontEntry::SetCairoFace(cairo_font_face_t *aFace)
{
    if (CAIRO_STATUS_SUCCESS !=
        cairo_font_face_set_user_data(aFace, &sFontEntryKey, this,
                                      ReleaseDownloadedFontEntry))
        return false;

    
    NS_ADDREF(this);
    return true;
}

hb_blob_t *
gfxDownloadedFcFontEntry::GetFontTable(uint32_t aTableTag)
{
    
    
    
    
    return GetTableFromFontData(mFontData, aTableTag);
}








class gfxFcFont : public gfxFT2FontBase {
public:
    virtual ~gfxFcFont();
    static already_AddRefed<gfxFcFont>
    GetOrMakeFont(FcPattern *aRequestedPattern, FcPattern *aFontPattern,
                  const gfxFontStyle *aFontStyle);

#ifdef USE_SKIA
    virtual mozilla::TemporaryRef<mozilla::gfx::GlyphRenderingOptions>
        GetGlyphRenderingOptions(const TextRunDrawParams* aRunParams = nullptr) override;
#endif

    
    virtual already_AddRefed<gfxFont>
    GetSubSuperscriptFont(int32_t aAppUnitsPerDevPixel) override;

protected:
    virtual already_AddRefed<gfxFont> MakeScaledFont(gfxFontStyle *aFontStyle,
                                                     gfxFloat aFontScale);
    virtual already_AddRefed<gfxFont> GetSmallCapsFont() override;

private:
    gfxFcFont(cairo_scaled_font_t *aCairoFont, gfxFcFontEntry *aFontEntry,
              const gfxFontStyle *aFontStyle);

    
    static cairo_user_data_key_t sGfxFontKey;
};








class gfxFcFontSet final {
public:
    NS_INLINE_DECL_REFCOUNTING(gfxFcFontSet)
    
    explicit gfxFcFontSet(FcPattern *aPattern,
                               gfxUserFontSet *aUserFontSet)
        : mSortPattern(aPattern), mUserFontSet(aUserFontSet),
          mFcFontsTrimmed(0),
          mHaveFallbackFonts(false)
    {
        bool waitForUserFont;
        mFcFontSet = SortPreferredFonts(waitForUserFont);
        mWaitingForUserFont = waitForUserFont;
    }

    
    
    gfxFcFont *GetFontAt(uint32_t i, const gfxFontStyle *aFontStyle)
    {
        if (i >= mFonts.Length() || !mFonts[i].mFont) { 
            
            FcPattern *fontPattern = GetFontPatternAt(i);
            if (!fontPattern)
                return nullptr;

            mFonts[i].mFont =
                gfxFcFont::GetOrMakeFont(mSortPattern, fontPattern,
                                         aFontStyle);
        }
        return mFonts[i].mFont;
    }

    FcPattern *GetFontPatternAt(uint32_t i);

    bool WaitingForUserFont() const {
        return mWaitingForUserFont;
    }

private:
    
    ~gfxFcFontSet()
    {
    }

    nsReturnRef<FcFontSet> SortPreferredFonts(bool& aWaitForUserFont);
    nsReturnRef<FcFontSet> SortFallbackFonts();

    struct FontEntry {
        explicit FontEntry(FcPattern *aPattern) : mPattern(aPattern) {}
        nsCountedRef<FcPattern> mPattern;
        nsRefPtr<gfxFcFont> mFont;
    };

    struct LangSupportEntry {
        LangSupportEntry(FcChar8 *aLang, FcLangResult aSupport) :
            mLang(aLang), mBestSupport(aSupport) {}
        FcChar8 *mLang;
        FcLangResult mBestSupport;
    };

public:
    
    class LangComparator {
    public:
        bool Equals(const LangSupportEntry& a, const FcChar8 *b) const
        {
            return FcStrCmpIgnoreCase(a.mLang, b) == 0;
        }
    };

private:
    
    nsCountedRef<FcPattern> mSortPattern;
    
    nsRefPtr<gfxUserFontSet> mUserFontSet;
    
    
    nsTArray<FontEntry> mFonts;
    
    
    
    
    nsAutoRef<FcFontSet> mFcFontSet;
    
    nsAutoRef<FcCharSet> mCharSet;
    
    
    int mFcFontsTrimmed;
    
    
    bool mHaveFallbackFonts;
    
    
    bool mWaitingForUserFont;
};



static const nsTArray< nsCountedRef<FcPattern> >*
FindFontPatterns(gfxUserFontSet *mUserFontSet,
                 const nsACString &aFamily, uint8_t aStyle,
                 uint16_t aWeight, int16_t aStretch,
                 bool& aWaitForUserFont)
{
    
    NS_ConvertUTF8toUTF16 utf16Family(aFamily);

    
    
    
    bool needsBold;

    gfxFontStyle style;
    style.style = aStyle;
    style.weight = aWeight;
    style.stretch = aStretch;

    gfxUserFcFontEntry *fontEntry = nullptr;
    gfxFontFamily *family = mUserFontSet->LookupFamily(utf16Family);
    if (family) {
        gfxUserFontEntry* userFontEntry =
            mUserFontSet->FindUserFontEntryAndLoad(family, style, needsBold,
                                                   aWaitForUserFont);
        if (userFontEntry) {
            fontEntry = static_cast<gfxUserFcFontEntry*>
                (userFontEntry->GetPlatformFontEntry());
        }

        
        
        
        
        if (!fontEntry && aStyle != NS_FONT_STYLE_NORMAL) {
            style.style = NS_FONT_STYLE_NORMAL;
            userFontEntry =
                mUserFontSet->FindUserFontEntryAndLoad(family, style,
                                                       needsBold,
                                                       aWaitForUserFont);
            if (userFontEntry) {
                fontEntry = static_cast<gfxUserFcFontEntry*>
                    (userFontEntry->GetPlatformFontEntry());
            }
        }
    }

    if (!fontEntry) {
        return nullptr;
    }

    return &fontEntry->GetPatterns();
}

typedef FcBool (*FcPatternRemoveFunction)(FcPattern *p, const char *object,
                                          int id);


static FcBool
moz_FcPatternRemove(FcPattern *p, const char *object, int id)
{
    static FcPatternRemoveFunction sFcPatternRemovePtr =
        reinterpret_cast<FcPatternRemoveFunction>
        (FindFunctionSymbol("FcPatternRemove"));

    if (!sFcPatternRemovePtr)
        return FcFalse;

    return (*sFcPatternRemovePtr)(p, object, id);
}



static bool
SizeIsAcceptable(FcPattern *aFont, double aRequestedSize)
{
    double size;
    int v = 0;
    while (FcPatternGetDouble(aFont,
                              FC_PIXEL_SIZE, v, &size) == FcResultMatch) {
        ++v;
        if (5.0 * fabs(size - aRequestedSize) < aRequestedSize)
            return true;
    }

    
    return v == 0;
}



nsReturnRef<FcFontSet>
gfxFcFontSet::SortPreferredFonts(bool &aWaitForUserFont)
{
    aWaitForUserFont = false;

    gfxFontconfigUtils *utils = gfxFontconfigUtils::GetFontconfigUtils();
    if (!utils)
        return nsReturnRef<FcFontSet>();

    
    
    
    
    
    
    
    

    
    
    
    
    
    nsAutoTArray<LangSupportEntry,10> requiredLangs;
    for (int v = 0; ; ++v) {
        FcChar8 *lang;
        FcResult result = FcPatternGetString(mSortPattern, FC_LANG, v, &lang);
        if (result != FcResultMatch) {
            
            
            
            NS_ASSERTION(result != FcResultTypeMismatch,
                         "Expected a string for FC_LANG");
            break;
        }

        if (!requiredLangs.Contains(lang, LangComparator())) {
            FcLangResult bestLangSupport = utils->GetBestLangSupport(lang);
            if (bestLangSupport != FcLangDifferentLang) {
                requiredLangs.
                    AppendElement(LangSupportEntry(lang, bestLangSupport));
            }
        }
    }

    nsAutoRef<FcFontSet> fontSet(FcFontSetCreate());
    if (!fontSet)
        return fontSet.out();

    
    
    int requestedSlant = FC_SLANT_ROMAN;
    FcPatternGetInteger(mSortPattern, FC_SLANT, 0, &requestedSlant);
    double requestedSize = -1.0;
    FcPatternGetDouble(mSortPattern, FC_PIXEL_SIZE, 0, &requestedSize);

    nsTHashtable<gfxFontconfigUtils::DepFcStrEntry> existingFamilies(32);
    FcChar8 *family;
    for (int v = 0;
         FcPatternGetString(mSortPattern,
                            FC_FAMILY, v, &family) == FcResultMatch; ++v) {
        const nsTArray< nsCountedRef<FcPattern> > *familyFonts = nullptr;

        
        bool isUserFont = false;
        if (mUserFontSet) {
            

            nsDependentCString cFamily(gfxFontconfigUtils::ToCString(family));
            NS_NAMED_LITERAL_CSTRING(userPrefix, FONT_FACE_FAMILY_PREFIX);

            if (StringBeginsWith(cFamily, userPrefix)) {
                isUserFont = true;

                
                nsDependentCSubstring cssFamily(cFamily, userPrefix.Length());

                uint8_t thebesStyle =
                    gfxFontconfigUtils::FcSlantToThebesStyle(requestedSlant);
                uint16_t thebesWeight =
                    gfxFontconfigUtils::GetThebesWeight(mSortPattern);
                int16_t thebesStretch =
                    gfxFontconfigUtils::GetThebesStretch(mSortPattern);

                bool waitForUserFont;
                familyFonts = FindFontPatterns(mUserFontSet, cssFamily,
                                               thebesStyle,
                                               thebesWeight, thebesStretch,
                                               waitForUserFont);
                if (waitForUserFont) {
                    aWaitForUserFont = true;
                }
            }
        }

        if (!isUserFont) {
            familyFonts = &utils->GetFontsForFamily(family);
        }

        if (!familyFonts || familyFonts->Length() == 0) {
            
            
            
            
            
            
            
            
            
            if (v != 0 && moz_FcPatternRemove(mSortPattern, FC_FAMILY, v)) {
                --v;
            }
            continue;
        }

        
        
        
        gfxFontconfigUtils::DepFcStrEntry *entry =
            existingFamilies.PutEntry(family);
        if (entry) {
            if (entry->mKey) 
                continue;

            entry->mKey = family; 
        }

        for (uint32_t f = 0; f < familyFonts->Length(); ++f) {
            FcPattern *font = familyFonts->ElementAt(f);

            
            
            
            if (isUserFont) {
                font = FcPatternDuplicate(font);
                FcPatternDel(font, FC_FAMILY);
                FcPatternAddString(font, FC_FAMILY, family);
            }

            
            
            if (requestedSize != -1.0 && !SizeIsAcceptable(font, requestedSize))
                continue;

            for (uint32_t r = 0; r < requiredLangs.Length(); ++r) {
                const LangSupportEntry& entry = requiredLangs[r];
                FcLangResult support =
                    gfxFontconfigUtils::GetLangSupport(font, entry.mLang);
                if (support <= entry.mBestSupport) { 
                    requiredLangs.RemoveElementAt(r);
                    --r;
                }
            }

            
            
            if (FcFontSetAdd(fontSet, font)) {
                
                
                
                if (!isUserFont) {
                    FcPatternReference(font);
                }
            }
        }
    }

    FcPattern *truncateMarker = nullptr;
    for (uint32_t r = 0; r < requiredLangs.Length(); ++r) {
        const nsTArray< nsCountedRef<FcPattern> >& langFonts =
            utils->GetFontsForLang(requiredLangs[r].mLang);

        bool haveLangFont = false;
        for (uint32_t f = 0; f < langFonts.Length(); ++f) {
            FcPattern *font = langFonts[f];
            if (requestedSize != -1.0 && !SizeIsAcceptable(font, requestedSize))
                continue;

            haveLangFont = true;
            if (FcFontSetAdd(fontSet, font)) {
                FcPatternReference(font);
            }
        }

        if (!haveLangFont && langFonts.Length() > 0) {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            FcPattern *font = langFonts[0];
            if (FcFontSetAdd(fontSet, font)) {
                FcPatternReference(font);
                truncateMarker = font;
            }
            break;
        }
    }

    FcFontSet *sets[1] = { fontSet };
    FcResult result;
#ifdef SOLARIS
    
    
    fontSet.own(FcFontSetSort(FcConfigGetCurrent(), sets, 1, mSortPattern,
                              FcFalse, nullptr, &result));
#else
    fontSet.own(FcFontSetSort(nullptr, sets, 1, mSortPattern,
                              FcFalse, nullptr, &result));
#endif

    if (truncateMarker != nullptr && fontSet) {
        nsAutoRef<FcFontSet> truncatedSet(FcFontSetCreate());

        for (int f = 0; f < fontSet->nfont; ++f) {
            FcPattern *font = fontSet->fonts[f];
            if (font == truncateMarker)
                break;

            if (FcFontSetAdd(truncatedSet, font)) {
                FcPatternReference(font);
            }
        }

        fontSet.steal(truncatedSet);
    }

    return fontSet.out();
}

nsReturnRef<FcFontSet>
gfxFcFontSet::SortFallbackFonts()
{
    
    
    
    
    
    
    
    
    FcResult result;
    return nsReturnRef<FcFontSet>(FcFontSort(nullptr, mSortPattern,
                                             FcFalse, nullptr, &result));
}


FcPattern *
gfxFcFontSet::GetFontPatternAt(uint32_t i)
{
    while (i >= mFonts.Length()) {
        while (!mFcFontSet) {
            if (mHaveFallbackFonts)
                return nullptr;

            mFcFontSet = SortFallbackFonts();
            mHaveFallbackFonts = true;
            mFcFontsTrimmed = 0;
            
        }

        while (mFcFontsTrimmed < mFcFontSet->nfont) {
            FcPattern *font = mFcFontSet->fonts[mFcFontsTrimmed];
            ++mFcFontsTrimmed;

            if (mFonts.Length() != 0) {
                
                
                
                
                FcCharSet *supportedChars = mCharSet;
                if (!supportedChars) {
                    FcPatternGetCharSet(mFonts[mFonts.Length() - 1].mPattern,
                                        FC_CHARSET, 0, &supportedChars);
                }

                if (supportedChars) {
                    FcCharSet *newChars = nullptr;
                    FcPatternGetCharSet(font, FC_CHARSET, 0, &newChars);
                    if (newChars) {
                        if (FcCharSetIsSubset(newChars, supportedChars))
                            continue;

                        mCharSet.own(FcCharSetUnion(supportedChars, newChars));
                    } else if (!mCharSet) {
                        mCharSet.own(FcCharSetCopy(supportedChars));
                    }
                }
            }

            mFonts.AppendElement(font);
            if (mFonts.Length() >= i)
                break;
        }

        if (mFcFontsTrimmed == mFcFontSet->nfont) {
            
            mFcFontSet.reset();
        }
    }

    return mFonts[i].mPattern;
}

#ifdef MOZ_WIDGET_GTK
static void ApplyGdkScreenFontOptions(FcPattern *aPattern);
#endif


static void
PrepareSortPattern(FcPattern *aPattern, double aFallbackSize,
                   double aSizeAdjustFactor, bool aIsPrinterFont)
{
    FcConfigSubstitute(nullptr, aPattern, FcMatchPattern);

    
    
    
    
    
    
    
    
    
    
    
    
    if(aIsPrinterFont) {
       cairo_font_options_t *options = cairo_font_options_create();
       cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
       cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_GRAY);
       cairo_ft_font_options_substitute(options, aPattern);
       cairo_font_options_destroy(options);
       FcPatternAddBool(aPattern, PRINTING_FC_PROPERTY, FcTrue);
    } else {
#ifdef MOZ_WIDGET_GTK
       ApplyGdkScreenFontOptions(aPattern);
#endif
    }

    
    
    double size = aFallbackSize;
    if (FcPatternGetDouble(aPattern, FC_PIXEL_SIZE, 0, &size) != FcResultMatch
        || aSizeAdjustFactor != 1.0) {
        FcPatternDel(aPattern, FC_PIXEL_SIZE);
        FcPatternAddDouble(aPattern, FC_PIXEL_SIZE, size * aSizeAdjustFactor);
    }

    FcDefaultSubstitute(aPattern);
}





gfxPangoFontGroup::gfxPangoFontGroup(const FontFamilyList& aFontFamilyList,
                                     const gfxFontStyle *aStyle,
                                     gfxUserFontSet *aUserFontSet)
    : gfxFontGroup(aFontFamilyList, aStyle, aUserFontSet),
      mPangoLanguage(GuessPangoLanguage(aStyle->language))
{
    
    
    if (mPangoLanguage) {
        mStyle.language = do_GetAtom(pango_language_to_string(mPangoLanguage));
    }

    
    mFonts.AppendElement(FamilyFace());
}

gfxPangoFontGroup::~gfxPangoFontGroup()
{
}

gfxFontGroup *
gfxPangoFontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxPangoFontGroup(mFamilyList, aStyle, mUserFontSet);
}

void
gfxPangoFontGroup::FindPlatformFont(const nsAString& fontName,
                                    bool aUseFontSet,
                                    void *aClosure)
{
    nsTArray<nsString> *list = static_cast<nsTArray<nsString>*>(aClosure);

    if (!list->Contains(fontName)) {
        
        if (aUseFontSet && mUserFontSet && mUserFontSet->HasFamily(fontName)) {
            nsAutoString userFontName =
                NS_LITERAL_STRING(FONT_FACE_FAMILY_PREFIX) + fontName;
            list->AppendElement(userFontName);
        } else {
            list->AppendElement(fontName);
        }
    }
}

gfxFcFont *
gfxPangoFontGroup::GetBaseFont()
{
    if (mFonts[0].Font() == nullptr) {
        gfxFont* font = GetBaseFontSet()->GetFontAt(0, GetStyle());
        mFonts[0] = FamilyFace(nullptr, font);
    }

    return static_cast<gfxFcFont*>(mFonts[0].Font());
}

gfxFont*
gfxPangoFontGroup::GetFirstValidFont(uint32_t aCh)
{
    return GetFontAt(0);
}

gfxFont *
gfxPangoFontGroup::GetFontAt(int32_t i, uint32_t aCh)
{
    
    
    
    
    NS_ASSERTION(!mUserFontSet || mCurrGeneration == GetGeneration(),
                 "Whoever was caching this font group should have "
                 "called UpdateUserFonts on it");

    NS_PRECONDITION(i == 0, "Only have one font");

    return GetBaseFont();
}

void
gfxPangoFontGroup::UpdateUserFonts()
{
    uint64_t newGeneration = GetGeneration();
    if (newGeneration == mCurrGeneration)
        return;

    mFonts[0] = FamilyFace();
    mFontSets.Clear();
    mCachedEllipsisTextRun = nullptr;
    mUnderlineOffset = UNDERLINE_OFFSET_NOT_SET;
    mCurrGeneration = newGeneration;
    mSkipDrawing = false;
}

already_AddRefed<gfxFcFontSet>
gfxPangoFontGroup::MakeFontSet(PangoLanguage *aLang, gfxFloat aSizeAdjustFactor,
                               nsAutoRef<FcPattern> *aMatchPattern)
{
    const char *lang = pango_language_to_string(aLang);

    nsRefPtr <nsIAtom> langGroup;
    if (aLang != mPangoLanguage) {
        
        langGroup = do_GetAtom(lang);
    }

    nsAutoTArray<nsString, 20> fcFamilyList;
    EnumerateFontList(langGroup ? langGroup.get() : mStyle.language.get(),
                      &fcFamilyList);

    

    
    nsAutoRef<FcPattern> pattern
        (gfxFontconfigUtils::NewPattern(fcFamilyList, mStyle, lang));

    PrepareSortPattern(pattern, mStyle.size, aSizeAdjustFactor, mStyle.printerFont);

    nsRefPtr<gfxFcFontSet> fontset =
        new gfxFcFontSet(pattern, mUserFontSet);

    mSkipDrawing = fontset->WaitingForUserFont();

    if (aMatchPattern)
        aMatchPattern->steal(pattern);

    return fontset.forget();
}

gfxPangoFontGroup::
FontSetByLangEntry::FontSetByLangEntry(PangoLanguage *aLang,
                                       gfxFcFontSet *aFontSet)
    : mLang(aLang), mFontSet(aFontSet)
{
}

gfxFcFontSet *
gfxPangoFontGroup::GetFontSet(PangoLanguage *aLang)
{
    GetBaseFontSet(); 

    if (!aLang)
        return mFontSets[0].mFontSet;

    for (uint32_t i = 0; i < mFontSets.Length(); ++i) {
        if (mFontSets[i].mLang == aLang)
            return mFontSets[i].mFontSet;
    }

    nsRefPtr<gfxFcFontSet> fontSet =
        MakeFontSet(aLang, mSizeAdjustFactor);
    mFontSets.AppendElement(FontSetByLangEntry(aLang, fontSet));

    return fontSet;
}

already_AddRefed<gfxFont>
gfxPangoFontGroup::FindFontForChar(uint32_t aCh, uint32_t aPrevCh,
                                   uint32_t aNextCh, int32_t aRunScript,
                                   gfxFont *aPrevMatchedFont,
                                   uint8_t *aMatchType)
{
    if (aPrevMatchedFont) {
        
        
        
        uint8_t category = GetGeneralCategory(aCh);
        if (category == HB_UNICODE_GENERAL_CATEGORY_CONTROL) {
            return nsRefPtr<gfxFont>(aPrevMatchedFont).forget();
        }

        
        
        if (gfxFontUtils::IsJoinControl(aCh) ||
            gfxFontUtils::IsJoinCauser(aPrevCh)) {
            if (aPrevMatchedFont->HasCharacter(aCh)) {
                return nsRefPtr<gfxFont>(aPrevMatchedFont).forget();
            }
        }
    }

    
    
    
    if (gfxFontUtils::IsVarSelector(aCh)) {
        if (aPrevMatchedFont) {
            return nsRefPtr<gfxFont>(aPrevMatchedFont).forget();
        }
        
        return nullptr;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    gfxFcFontSet *fontSet = GetBaseFontSet();
    uint32_t nextFont = 0;
    FcPattern *basePattern = nullptr;
    if (!mStyle.systemFont && mPangoLanguage) {
        basePattern = fontSet->GetFontPatternAt(0);
        if (HasChar(basePattern, aCh)) {
            *aMatchType = gfxTextRange::kFontGroup;
            return nsRefPtr<gfxFont>(GetBaseFont()).forget();
        }

        nextFont = 1;
    }

    
    
    const PangoScript script = static_cast<PangoScript>(aRunScript);
    
    
    PangoLanguage *scriptLang;
    if ((!basePattern ||
         !pango_language_includes_script(mPangoLanguage, script)) &&
        (scriptLang = pango_script_get_sample_language(script))) {
        fontSet = GetFontSet(scriptLang);
        nextFont = 0;
    }

    for (uint32_t i = nextFont;
         FcPattern *pattern = fontSet->GetFontPatternAt(i);
         ++i) {
        if (pattern == basePattern) {
            continue; 
        }

        if (HasChar(pattern, aCh)) {
            *aMatchType = gfxTextRange::kFontGroup;
            return nsRefPtr<gfxFont>(fontSet->GetFontAt(i, GetStyle())).forget();
        }
    }

    return nullptr;
}



#define CHECK_SCRIPT_CODE(script) \
    PR_STATIC_ASSERT(int32_t(MOZ_SCRIPT_##script) == \
                     int32_t(PANGO_SCRIPT_##script))

CHECK_SCRIPT_CODE(COMMON);
CHECK_SCRIPT_CODE(INHERITED);
CHECK_SCRIPT_CODE(ARABIC);
CHECK_SCRIPT_CODE(LATIN);
CHECK_SCRIPT_CODE(UNKNOWN);
CHECK_SCRIPT_CODE(NKO);





cairo_user_data_key_t gfxFcFont::sGfxFontKey;

gfxFcFont::gfxFcFont(cairo_scaled_font_t *aCairoFont,
                     gfxFcFontEntry *aFontEntry,
                     const gfxFontStyle *aFontStyle)
    : gfxFT2FontBase(aCairoFont, aFontEntry, aFontStyle)
{
    cairo_scaled_font_set_user_data(mScaledFont, &sGfxFontKey, this, nullptr);
}

gfxFcFont::~gfxFcFont()
{
    cairo_scaled_font_set_user_data(mScaledFont,
                                    &sGfxFontKey,
                                    nullptr,
                                    nullptr);
}

already_AddRefed<gfxFont>
gfxFcFont::GetSubSuperscriptFont(int32_t aAppUnitsPerDevPixel)
{
    gfxFontStyle style(*GetStyle());
    style.AdjustForSubSuperscript(aAppUnitsPerDevPixel);
    return MakeScaledFont(&style, style.size / GetStyle()->size);
}

already_AddRefed<gfxFont>
gfxFcFont::MakeScaledFont(gfxFontStyle *aFontStyle, gfxFloat aScaleFactor)
{
    gfxFcFontEntry* fe = static_cast<gfxFcFontEntry*>(GetFontEntry());
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(fe, aFontStyle);
    if (font) {
        return font.forget();
    }

    cairo_matrix_t fontMatrix;
    cairo_scaled_font_get_font_matrix(mScaledFont, &fontMatrix);
    cairo_matrix_scale(&fontMatrix, aScaleFactor, aScaleFactor);

    cairo_matrix_t ctm;
    cairo_scaled_font_get_ctm(mScaledFont, &ctm);

    cairo_font_options_t *options = cairo_font_options_create();
    cairo_scaled_font_get_font_options(mScaledFont, options);

    cairo_scaled_font_t *newFont =
        cairo_scaled_font_create(cairo_scaled_font_get_font_face(mScaledFont),
                                 &fontMatrix, &ctm, options);
    cairo_font_options_destroy(options);

    font = new gfxFcFont(newFont, fe, aFontStyle);
    gfxFontCache::GetCache()->AddNew(font);
    cairo_scaled_font_destroy(newFont);

    return font.forget();
}

already_AddRefed<gfxFont>
gfxFcFont::GetSmallCapsFont()
{
    gfxFontStyle style(*GetStyle());
    style.size *= SMALL_CAPS_SCALE_FACTOR;
    style.variantCaps = NS_FONT_VARIANT_CAPS_NORMAL;
    return MakeScaledFont(&style, SMALL_CAPS_SCALE_FACTOR);
}

 void
gfxPangoFontGroup::Shutdown()
{
    
    
    gFTLibrary = nullptr;
}

 gfxFontEntry *
gfxPangoFontGroup::NewFontEntry(const nsAString& aFontName,
                                uint16_t aWeight,
                                int16_t aStretch,
                                bool aItalic)
{
    gfxFontconfigUtils *utils = gfxFontconfigUtils::GetFontconfigUtils();
    if (!utils)
        return nullptr;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsAutoRef<FcPattern> pattern(FcPatternCreate());
    if (!pattern)
        return nullptr;

    NS_ConvertUTF16toUTF8 fullname(aFontName);
    FcPatternAddString(pattern, FC_FULLNAME,
                       gfxFontconfigUtils::ToFcChar8(fullname));
    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);

    FcChar8 *name;
    for (int v = 0;
         FcPatternGetString(pattern, FC_FULLNAME, v, &name) == FcResultMatch;
         ++v) {
        const nsTArray< nsCountedRef<FcPattern> >& fonts =
            utils->GetFontsForFullname(name);

        if (fonts.Length() != 0)
            return new gfxLocalFcFontEntry(aFontName,
                                           aWeight,
                                           aStretch,
                                           aItalic,
                                           fonts);
    }

    return nullptr;
}

 FT_Library
gfxPangoFontGroup::GetFTLibrary()
{
    if (!gFTLibrary) {
        
        
        
        
        
        
        
        
        gfxFontStyle style;
        nsRefPtr<gfxPangoFontGroup> fontGroup =
            new gfxPangoFontGroup(FontFamilyList(eFamily_sans_serif),
                                  &style, nullptr);

        gfxFcFont *font = fontGroup->GetBaseFont();
        if (!font)
            return nullptr;

        gfxFT2LockedFace face(font);
        if (!face.get())
            return nullptr;

        gFTLibrary = face.get()->glyph->library;
    }

    return gFTLibrary;
}

 gfxFontEntry *
gfxPangoFontGroup::NewFontEntry(const nsAString& aFontName,
                                uint16_t aWeight,
                                int16_t aStretch,
                                bool aItalic,
                                const uint8_t* aFontData,
                                uint32_t aLength)
{
    
    

    
    
    FT_Face face;
    FT_Error error =
        FT_New_Memory_Face(GetFTLibrary(), aFontData, aLength, 0, &face);
    if (error != 0) {
        NS_Free((void*)aFontData);
        return nullptr;
    }

    return new gfxDownloadedFcFontEntry(aFontName, aWeight,
                                        aStretch, aItalic,
                                        aFontData, face);
}


static double
GetPixelSize(FcPattern *aPattern)
{
    double size;
    if (FcPatternGetDouble(aPattern,
                           FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
        return size;

    NS_NOTREACHED("No size on pattern");
    return 0.0;
}















already_AddRefed<gfxFcFont>
gfxFcFont::GetOrMakeFont(FcPattern *aRequestedPattern, FcPattern *aFontPattern,
                         const gfxFontStyle *aFontStyle)
{
    nsAutoRef<FcPattern> renderPattern
        (FcFontRenderPrepare(nullptr, aRequestedPattern, aFontPattern));

    
    
    if (!aFontStyle->allowSyntheticWeight) {
        int weight;
        if (FcPatternGetInteger(aFontPattern, FC_WEIGHT, 0,
                                &weight) == FcResultMatch) {
            FcPatternDel(renderPattern, FC_WEIGHT);
            FcPatternAddInteger(renderPattern, FC_WEIGHT, weight);
        }
    }
    if (!aFontStyle->allowSyntheticStyle) {
        int slant;
        if (FcPatternGetInteger(aFontPattern, FC_SLANT, 0,
                                &slant) == FcResultMatch) {
            FcPatternDel(renderPattern, FC_SLANT);
            FcPatternAddInteger(renderPattern, FC_SLANT, slant);
        }
    }

    cairo_font_face_t *face =
        cairo_ft_font_face_create_for_pattern(renderPattern);

    
    nsRefPtr<gfxFcFontEntry> fe = gfxFcFontEntry::LookupFontEntry(face);
    if (!fe) {
        gfxDownloadedFcFontEntry *downloadedFontEntry =
            GetDownloadedFontEntry(aFontPattern);
        if (downloadedFontEntry) {
            
            fe = downloadedFontEntry;
            if (cairo_font_face_status(face) == CAIRO_STATUS_SUCCESS) {
                
                
                
                if (!downloadedFontEntry->SetCairoFace(face)) {
                    
                    cairo_font_face_destroy(face);
                    face = cairo_ft_font_face_create_for_pattern(aRequestedPattern);
                    fe = gfxFcFontEntry::LookupFontEntry(face);
                }
            }
        }
        if (!fe) {
            
            nsAutoString name;
            FcChar8 *fc_file;
            if (FcPatternGetString(renderPattern,
                                   FC_FILE, 0, &fc_file) == FcResultMatch) {
                int index;
                if (FcPatternGetInteger(renderPattern,
                                        FC_INDEX, 0, &index) != FcResultMatch) {
                    
                    index = 0;
                }

                AppendUTF8toUTF16(gfxFontconfigUtils::ToCString(fc_file), name);
                if (index != 0) {
                    name.Append('/');
                    name.AppendInt(index);
                }
            }

            fe = new gfxSystemFcFontEntry(face, aFontPattern, name);
        }
    }

    gfxFontStyle style(*aFontStyle);
    style.size = GetPixelSize(renderPattern);
    style.style = gfxFontconfigUtils::GetThebesStyle(renderPattern);
    style.weight = gfxFontconfigUtils::GetThebesWeight(renderPattern);

    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(fe, &style);
    if (!font) {
        
        
        
        
        
        
        
        cairo_scaled_font_t *cairoFont = CreateScaledFont(renderPattern, face);
        font = new gfxFcFont(cairoFont, fe, &style);
        gfxFontCache::GetCache()->AddNew(font);
        cairo_scaled_font_destroy(cairoFont);
    }

    cairo_font_face_destroy(face);

    nsRefPtr<gfxFcFont> retval(static_cast<gfxFcFont*>(font.get()));
    return retval.forget();
}

gfxFcFontSet *
gfxPangoFontGroup::GetBaseFontSet()
{
    if (mFontSets.Length() > 0)
        return mFontSets[0].mFontSet;

    mSizeAdjustFactor = 1.0; 
    nsAutoRef<FcPattern> pattern;
    nsRefPtr<gfxFcFontSet> fontSet =
        MakeFontSet(mPangoLanguage, mSizeAdjustFactor, &pattern);

    double size = GetPixelSize(pattern);
    if (size != 0.0 && mStyle.sizeAdjust > 0.0) {
        gfxFcFont *font = fontSet->GetFontAt(0, GetStyle());
        if (font) {
            const gfxFont::Metrics& metrics =
                font->GetMetrics(gfxFont::eHorizontal); 

            
            
            
            if (metrics.xHeight > 0.1 * metrics.emHeight) {
                mSizeAdjustFactor =
                    mStyle.sizeAdjust * metrics.emHeight / metrics.xHeight;

                size *= mSizeAdjustFactor;
                FcPatternDel(pattern, FC_PIXEL_SIZE);
                FcPatternAddDouble(pattern, FC_PIXEL_SIZE, size);

                fontSet = new gfxFcFontSet(pattern, mUserFontSet);
            }
        }
    }

    PangoLanguage *pangoLang = mPangoLanguage;
    FcChar8 *fcLang;
    if (!pangoLang &&
        FcPatternGetString(pattern, FC_LANG, 0, &fcLang) == FcResultMatch) {
        pangoLang =
            pango_language_from_string(gfxFontconfigUtils::ToCString(fcLang));
    }

    mFontSets.AppendElement(FontSetByLangEntry(pangoLang, fontSet));

    return fontSet;
}












static cairo_scaled_font_t *
CreateScaledFont(FcPattern *aPattern, cairo_font_face_t *aFace)
{
    double size = GetPixelSize(aPattern);
        
    cairo_matrix_t fontMatrix;
    FcMatrix *fcMatrix;
    if (FcPatternGetMatrix(aPattern, FC_MATRIX, 0, &fcMatrix) == FcResultMatch)
        cairo_matrix_init(&fontMatrix, fcMatrix->xx, -fcMatrix->yx, -fcMatrix->xy, fcMatrix->yy, 0, 0);
    else
        cairo_matrix_init_identity(&fontMatrix);
    cairo_matrix_scale(&fontMatrix, size, size);

    FcBool printing;
    if (FcPatternGetBool(aPattern, PRINTING_FC_PROPERTY, 0, &printing) != FcResultMatch) {
        printing = FcFalse;
    }

    
    
    
    cairo_matrix_t identityMatrix;
    cairo_matrix_init_identity(&identityMatrix);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    cairo_font_options_t *fontOptions = cairo_font_options_create();

    
    
    
    
    
    
    if (printing) {
        cairo_font_options_set_hint_metrics(fontOptions, CAIRO_HINT_METRICS_OFF);
    } else {
        cairo_font_options_set_hint_metrics(fontOptions, CAIRO_HINT_METRICS_ON);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    FcBool hinting = FcFalse;
    if (FcPatternGetBool(aPattern, FC_HINTING, 0, &hinting) != FcResultMatch) {
        hinting = FcTrue;
    }

    cairo_hint_style_t hint_style;
    if (printing || !hinting) {
        hint_style = CAIRO_HINT_STYLE_NONE;
    } else {
#ifdef FC_HINT_STYLE  
        int fc_hintstyle;
        if (FcPatternGetInteger(aPattern, FC_HINT_STYLE,
                                0, &fc_hintstyle        ) != FcResultMatch) {
            fc_hintstyle = FC_HINT_FULL;
        }
        switch (fc_hintstyle) {
            case FC_HINT_NONE:
                hint_style = CAIRO_HINT_STYLE_NONE;
                break;
            case FC_HINT_SLIGHT:
                hint_style = CAIRO_HINT_STYLE_SLIGHT;
                break;
            case FC_HINT_MEDIUM:
            default: 
                hint_style = CAIRO_HINT_STYLE_MEDIUM;
                break;
            case FC_HINT_FULL:
                hint_style = CAIRO_HINT_STYLE_FULL;
                break;
        }
#else 
        hint_style = CAIRO_HINT_STYLE_FULL;
#endif
    }
    cairo_font_options_set_hint_style(fontOptions, hint_style);

    int rgba;
    if (FcPatternGetInteger(aPattern,
                            FC_RGBA, 0, &rgba) != FcResultMatch) {
        rgba = FC_RGBA_UNKNOWN;
    }
    cairo_subpixel_order_t subpixel_order = CAIRO_SUBPIXEL_ORDER_DEFAULT;
    switch (rgba) {
        case FC_RGBA_UNKNOWN:
        case FC_RGBA_NONE:
        default:
            
            
            rgba = FC_RGBA_NONE;
            
            
            
        case FC_RGBA_RGB:
            subpixel_order = CAIRO_SUBPIXEL_ORDER_RGB;
            break;
        case FC_RGBA_BGR:
            subpixel_order = CAIRO_SUBPIXEL_ORDER_BGR;
            break;
        case FC_RGBA_VRGB:
            subpixel_order = CAIRO_SUBPIXEL_ORDER_VRGB;
            break;
        case FC_RGBA_VBGR:
            subpixel_order = CAIRO_SUBPIXEL_ORDER_VBGR;
            break;
    }
    cairo_font_options_set_subpixel_order(fontOptions, subpixel_order);

    FcBool fc_antialias;
    if (FcPatternGetBool(aPattern,
                         FC_ANTIALIAS, 0, &fc_antialias) != FcResultMatch) {
        fc_antialias = FcTrue;
    }
    cairo_antialias_t antialias;
    if (!fc_antialias) {
        antialias = CAIRO_ANTIALIAS_NONE;
    } else if (rgba == FC_RGBA_NONE) {
        antialias = CAIRO_ANTIALIAS_GRAY;
    } else {
        antialias = CAIRO_ANTIALIAS_SUBPIXEL;
    }
    cairo_font_options_set_antialias(fontOptions, antialias);

    cairo_scaled_font_t *scaledFont =
        cairo_scaled_font_create(aFace, &fontMatrix, &identityMatrix,
                                 fontOptions);

    cairo_font_options_destroy(fontOptions);

    NS_ASSERTION(cairo_scaled_font_status(scaledFont) == CAIRO_STATUS_SUCCESS,
                 "Failed to create scaled font");
    return scaledFont;
}


PangoLanguage *
GuessPangoLanguage(nsIAtom *aLanguage)
{
    if (!aLanguage)
        return nullptr;

    
    
    nsAutoCString lang;
    gfxFontconfigUtils::GetSampleLangForGroup(aLanguage, &lang);
    if (lang.IsEmpty())
        return nullptr;

    return pango_language_from_string(lang.get());
}

#ifdef MOZ_WIDGET_GTK







#if MOZ_TREE_CAIRO


#undef cairo_ft_font_options_substitute



extern "C" {
NS_VISIBILITY_DEFAULT void
cairo_ft_font_options_substitute (const cairo_font_options_t *options,
                                  FcPattern                  *pattern);
}
#endif

static void
ApplyGdkScreenFontOptions(FcPattern *aPattern)
{
    const cairo_font_options_t *options =
        gdk_screen_get_font_options(gdk_screen_get_default());

    cairo_ft_font_options_substitute(options, aPattern);
}

#endif 

#ifdef USE_SKIA
mozilla::TemporaryRef<mozilla::gfx::GlyphRenderingOptions>
gfxFcFont::GetGlyphRenderingOptions(const TextRunDrawParams* aRunParams)
{
  cairo_scaled_font_t *scaled_font = CairoScaledFont();
  cairo_font_options_t *options = cairo_font_options_create();
  cairo_scaled_font_get_font_options(scaled_font, options);
  cairo_hint_style_t hint_style = cairo_font_options_get_hint_style(options);     
  cairo_font_options_destroy(options);

  mozilla::gfx::FontHinting hinting;

  switch (hint_style) {
    case CAIRO_HINT_STYLE_NONE:
      hinting = mozilla::gfx::FontHinting::NONE;
      break;
    case CAIRO_HINT_STYLE_SLIGHT:
      hinting = mozilla::gfx::FontHinting::LIGHT;
      break;
    case CAIRO_HINT_STYLE_FULL:
      hinting = mozilla::gfx::FontHinting::FULL;
      break;
    default:
      hinting = mozilla::gfx::FontHinting::NORMAL;
      break;
  }

  
  return mozilla::gfx::Factory::CreateCairoGlyphRenderingOptions(hinting, false);
}
#endif

