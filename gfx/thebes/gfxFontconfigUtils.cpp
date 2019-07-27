




#include "mozilla/ArrayUtils.h"

#include "gfxFontconfigUtils.h"
#include "gfxFont.h"
#include "nsGkAtoms.h"

#include <locale.h>
#include <fontconfig/fontconfig.h>

#include "nsServiceManagerUtils.h"
#include "nsILanguageAtomService.h"
#include "nsTArray.h"
#include "mozilla/Preferences.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"

#include "nsIAtom.h"
#include "nsCRT.h"
#include "gfxFontConstants.h"
#include "mozilla/gfx/2D.h"

using namespace mozilla;

 gfxFontconfigUtils* gfxFontconfigUtils::sUtils = nullptr;
static nsILanguageAtomService* gLangService = nullptr;

 void
gfxFontconfigUtils::Shutdown() {
    if (sUtils) {
        delete sUtils;
        sUtils = nullptr;
    }
    NS_IF_RELEASE(gLangService);
}

 uint8_t
gfxFontconfigUtils::FcSlantToThebesStyle(int aFcSlant)
{
    switch (aFcSlant) {
        case FC_SLANT_ITALIC:
            return NS_FONT_STYLE_ITALIC;
        case FC_SLANT_OBLIQUE:
            return NS_FONT_STYLE_OBLIQUE;
        default:
            return NS_FONT_STYLE_NORMAL;
    }
}

 uint8_t
gfxFontconfigUtils::GetThebesStyle(FcPattern *aPattern)
{
    int slant;
    if (FcPatternGetInteger(aPattern, FC_SLANT, 0, &slant) != FcResultMatch) {
        return NS_FONT_STYLE_NORMAL;
    }

    return FcSlantToThebesStyle(slant);
}

 int
gfxFontconfigUtils::GetFcSlant(const gfxFontStyle& aFontStyle)
{
    if (aFontStyle.style == NS_FONT_STYLE_ITALIC)
        return FC_SLANT_ITALIC;
    if (aFontStyle.style == NS_FONT_STYLE_OBLIQUE)
        return FC_SLANT_OBLIQUE;

    return FC_SLANT_ROMAN;
}


#ifndef FC_WEIGHT_THIN 
#define FC_WEIGHT_THIN              0 // 2.1.93
#define FC_WEIGHT_EXTRALIGHT        40 // 2.1.93
#define FC_WEIGHT_REGULAR           80 // 2.1.93
#define FC_WEIGHT_EXTRABOLD         205 // 2.1.93
#endif

#ifndef FC_WEIGHT_BOOK
#define FC_WEIGHT_BOOK              75
#endif

#ifndef FC_WEIGHT_EXTRABLACK
#define FC_WEIGHT_EXTRABLACK        215
#endif

 uint16_t
gfxFontconfigUtils::GetThebesWeight(FcPattern *aPattern)
{
    int weight;
    if (FcPatternGetInteger(aPattern, FC_WEIGHT, 0, &weight) != FcResultMatch)
        return NS_FONT_WEIGHT_NORMAL;

    if (weight <= (FC_WEIGHT_THIN + FC_WEIGHT_EXTRALIGHT) / 2)
        return 100;
    if (weight <= (FC_WEIGHT_EXTRALIGHT + FC_WEIGHT_LIGHT) / 2)
        return 200;
    if (weight <= (FC_WEIGHT_LIGHT + FC_WEIGHT_BOOK) / 2)
        return 300;
    if (weight <= (FC_WEIGHT_REGULAR + FC_WEIGHT_MEDIUM) / 2)
        
        return 400;
    if (weight <= (FC_WEIGHT_MEDIUM + FC_WEIGHT_DEMIBOLD) / 2)
        return 500;
    if (weight <= (FC_WEIGHT_DEMIBOLD + FC_WEIGHT_BOLD) / 2)
        return 600;
    if (weight <= (FC_WEIGHT_BOLD + FC_WEIGHT_EXTRABOLD) / 2)
        return 700;
    if (weight <= (FC_WEIGHT_EXTRABOLD + FC_WEIGHT_BLACK) / 2)
        return 800;
    if (weight <= FC_WEIGHT_BLACK)
        return 900;

    
    return 901;
}

 int
gfxFontconfigUtils::FcWeightForBaseWeight(int8_t aBaseWeight)
{
    NS_PRECONDITION(aBaseWeight >= 0 && aBaseWeight <= 10,
                    "base weight out of range");

    switch (aBaseWeight) {
        case 2:
            return FC_WEIGHT_EXTRALIGHT;
        case 3:
            return FC_WEIGHT_LIGHT;
        case 4:
            return FC_WEIGHT_REGULAR;
        case 5:
            return FC_WEIGHT_MEDIUM;
        case 6:
            return FC_WEIGHT_DEMIBOLD;
        case 7:
            return FC_WEIGHT_BOLD;
        case 8:
            return FC_WEIGHT_EXTRABOLD;
        case 9:
            return FC_WEIGHT_BLACK;
    }

    
    return aBaseWeight < 2 ? FC_WEIGHT_THIN : FC_WEIGHT_EXTRABLACK;
}

 int16_t
gfxFontconfigUtils::GetThebesStretch(FcPattern *aPattern)
{
    int width;
    if (FcPatternGetInteger(aPattern, FC_WIDTH, 0, &width) != FcResultMatch) {
        return NS_FONT_STRETCH_NORMAL;
    }

    if (width <= (FC_WIDTH_ULTRACONDENSED + FC_WIDTH_EXTRACONDENSED) / 2) {
        return NS_FONT_STRETCH_ULTRA_CONDENSED;
    }
    if (width <= (FC_WIDTH_EXTRACONDENSED + FC_WIDTH_CONDENSED) / 2) {
        return NS_FONT_STRETCH_EXTRA_CONDENSED;
    }
    if (width <= (FC_WIDTH_CONDENSED + FC_WIDTH_SEMICONDENSED) / 2) {
        return NS_FONT_STRETCH_CONDENSED;
    }
    if (width <= (FC_WIDTH_SEMICONDENSED + FC_WIDTH_NORMAL) / 2) {
        return NS_FONT_STRETCH_SEMI_CONDENSED;
    }
    if (width <= (FC_WIDTH_NORMAL + FC_WIDTH_SEMIEXPANDED) / 2) {
        return NS_FONT_STRETCH_NORMAL;
    }
    if (width <= (FC_WIDTH_SEMIEXPANDED + FC_WIDTH_EXPANDED) / 2) {
        return NS_FONT_STRETCH_SEMI_EXPANDED;
    }
    if (width <= (FC_WIDTH_EXPANDED + FC_WIDTH_EXTRAEXPANDED) / 2) {
        return NS_FONT_STRETCH_EXPANDED;
    }
    if (width <= (FC_WIDTH_EXTRAEXPANDED + FC_WIDTH_ULTRAEXPANDED) / 2) {
        return NS_FONT_STRETCH_EXTRA_EXPANDED;
    }
    return NS_FONT_STRETCH_ULTRA_EXPANDED;
}

 int
gfxFontconfigUtils::FcWidthForThebesStretch(int16_t aStretch)
{
    switch (aStretch) {
        default: 
            return FC_WIDTH_NORMAL;
        case NS_FONT_STRETCH_ULTRA_CONDENSED:
            return FC_WIDTH_ULTRACONDENSED;
        case NS_FONT_STRETCH_EXTRA_CONDENSED:
            return FC_WIDTH_EXTRACONDENSED;
        case NS_FONT_STRETCH_CONDENSED:
            return FC_WIDTH_CONDENSED;
        case NS_FONT_STRETCH_SEMI_CONDENSED:
            return FC_WIDTH_SEMICONDENSED;
        case NS_FONT_STRETCH_SEMI_EXPANDED:
            return FC_WIDTH_SEMIEXPANDED;
        case NS_FONT_STRETCH_EXPANDED:
            return FC_WIDTH_EXPANDED;
        case NS_FONT_STRETCH_EXTRA_EXPANDED:
            return FC_WIDTH_EXTRAEXPANDED;
        case NS_FONT_STRETCH_ULTRA_EXPANDED:
            return FC_WIDTH_ULTRAEXPANDED;
    }
}




 int
GuessFcWeight(const gfxFontStyle& aFontStyle)
{
    







    int8_t weight = aFontStyle.ComputeWeight();

    
    NS_ASSERTION(weight >= 0 && weight <= 10,
                 "base weight out of range");

    return gfxFontconfigUtils::FcWeightForBaseWeight(weight);
}

static void
AddString(FcPattern *aPattern, const char *object, const char *aString)
{
    FcPatternAddString(aPattern, object,
                       gfxFontconfigUtils::ToFcChar8(aString));
}

static void
AddWeakString(FcPattern *aPattern, const char *object, const char *aString)
{
    FcValue value;
    value.type = FcTypeString;
    value.u.s = gfxFontconfigUtils::ToFcChar8(aString);

    FcPatternAddWeak(aPattern, object, value, FcTrue);
}

static void
AddLangGroup(FcPattern *aPattern, nsIAtom *aLangGroup)
{
    
    nsAutoCString lang;
    gfxFontconfigUtils::GetSampleLangForGroup(aLangGroup, &lang);

    if (!lang.IsEmpty()) {
        AddString(aPattern, FC_LANG, lang.get());
    }
}

nsReturnRef<FcPattern>
gfxFontconfigUtils::NewPattern(const nsTArray<nsString>& aFamilies,
                               const gfxFontStyle& aFontStyle,
                               const char *aLang)
{
    static const char* sFontconfigGenerics[] =
        { "sans-serif", "serif", "monospace", "fantasy", "cursive" };

    nsAutoRef<FcPattern> pattern(FcPatternCreate());
    if (!pattern)
        return nsReturnRef<FcPattern>();

    FcPatternAddDouble(pattern, FC_PIXEL_SIZE, aFontStyle.size);
    FcPatternAddInteger(pattern, FC_SLANT, GetFcSlant(aFontStyle));
    FcPatternAddInteger(pattern, FC_WEIGHT, GuessFcWeight(aFontStyle));
    FcPatternAddInteger(pattern, FC_WIDTH, FcWidthForThebesStretch(aFontStyle.stretch));

    if (aLang) {
        AddString(pattern, FC_LANG, aLang);
    }

    bool useWeakBinding = false;
    for (uint32_t i = 0; i < aFamilies.Length(); ++i) {
        NS_ConvertUTF16toUTF8 family(aFamilies[i]);
        if (!useWeakBinding) {
            AddString(pattern, FC_FAMILY, family.get());

            
            
            
            
            
            
            for (uint32_t g = 0;
                 g < ArrayLength(sFontconfigGenerics);
                 ++g) {
                if (0 == FcStrCmpIgnoreCase(ToFcChar8(sFontconfigGenerics[g]),
                                            ToFcChar8(family.get()))) {
                    useWeakBinding = true;
                    break;
                }
            }
        } else {
            AddWeakString(pattern, FC_FAMILY, family.get());
        }
    }

    return pattern.out();
}

gfxFontconfigUtils::gfxFontconfigUtils()
    : mFontsByFamily(32)
    , mFontsByFullname(32)
    , mLangSupportTable(32)
    , mLastConfig(nullptr)
#ifdef MOZ_BUNDLED_FONTS
    , mBundledFontsInitialized(false)
#endif
{
    UpdateFontListInternal();
}

nsresult
gfxFontconfigUtils::GetFontList(nsIAtom *aLangGroup,
                                const nsACString& aGenericFamily,
                                nsTArray<nsString>& aListOfFonts)
{
    aListOfFonts.Clear();

    nsTArray<nsCString> fonts;
    nsresult rv = GetFontListInternal(fonts, aLangGroup);
    if (NS_FAILED(rv))
        return rv;

    for (uint32_t i = 0; i < fonts.Length(); ++i) {
        aListOfFonts.AppendElement(NS_ConvertUTF8toUTF16(fonts[i]));
    }

    aListOfFonts.Sort();

    int32_t serif = 0, sansSerif = 0, monospace = 0;

    
    
    if (aGenericFamily.IsEmpty())
        serif = sansSerif = monospace = 1;
    else if (aGenericFamily.LowerCaseEqualsLiteral("serif"))
        serif = 1;
    else if (aGenericFamily.LowerCaseEqualsLiteral("sans-serif"))
        sansSerif = 1;
    else if (aGenericFamily.LowerCaseEqualsLiteral("monospace"))
        monospace = 1;
    else if (aGenericFamily.LowerCaseEqualsLiteral("cursive") ||
             aGenericFamily.LowerCaseEqualsLiteral("fantasy"))
        serif = sansSerif = 1;
    else
        NS_NOTREACHED("unexpected CSS generic font family");

    
    
    
    if (monospace)
        aListOfFonts.InsertElementAt(0, NS_LITERAL_STRING("monospace"));
    if (sansSerif)
        aListOfFonts.InsertElementAt(0, NS_LITERAL_STRING("sans-serif"));
    if (serif)
        aListOfFonts.InsertElementAt(0, NS_LITERAL_STRING("serif"));

    return NS_OK;
}

struct MozLangGroupData {
    nsIAtom* const& mozLangGroup;
    const char *defaultLang;
};

const MozLangGroupData MozLangGroups[] = {
    { nsGkAtoms::x_western,      "en" },
    { nsGkAtoms::x_cyrillic,     "ru" },
    { nsGkAtoms::x_devanagari,   "hi" },
    { nsGkAtoms::x_tamil,        "ta" },
    { nsGkAtoms::x_armn,         "hy" },
    { nsGkAtoms::x_beng,         "bn" },
    { nsGkAtoms::x_cans,         "iu" },
    { nsGkAtoms::x_ethi,         "am" },
    { nsGkAtoms::x_geor,         "ka" },
    { nsGkAtoms::x_gujr,         "gu" },
    { nsGkAtoms::x_guru,         "pa" },
    { nsGkAtoms::x_khmr,         "km" },
    { nsGkAtoms::x_knda,         "kn" },
    { nsGkAtoms::x_mlym,         "ml" },
    { nsGkAtoms::x_orya,         "or" },
    { nsGkAtoms::x_sinh,         "si" },
    { nsGkAtoms::x_telu,         "te" },
    { nsGkAtoms::x_tibt,         "bo" },
    { nsGkAtoms::Unicode,        0    },
};

static bool
TryLangForGroup(const nsACString& aOSLang, nsIAtom *aLangGroup,
                nsACString *aFcLang)
{
    
    
    
    
    
    
    const char *pos, *end;
    aOSLang.BeginReading(pos);
    aOSLang.EndReading(end);
    aFcLang->Truncate();
    while (pos < end) {
        switch (*pos) {
            case '.':
            case '@':
                end = pos;
                break;
            case '_':
                aFcLang->Append('-');
                break;
            default:
                aFcLang->Append(*pos);
        }
        ++pos;
    }

    nsIAtom *atom =
        gLangService->LookupLanguage(*aFcLang);

    return atom == aLangGroup;
}

 void
gfxFontconfigUtils::GetSampleLangForGroup(nsIAtom *aLangGroup,
                                          nsACString *aFcLang)
{
    NS_PRECONDITION(aFcLang != nullptr, "aFcLang must not be NULL");

    const MozLangGroupData *langGroup = nullptr;

    for (unsigned int i = 0; i < ArrayLength(MozLangGroups); ++i) {
        if (aLangGroup == MozLangGroups[i].mozLangGroup) {
            langGroup = &MozLangGroups[i];
            break;
        }
    }

    if (!langGroup) {
        
        
        aLangGroup->ToUTF8String(*aFcLang);
        return;
    }

    
    
    if (!gLangService) {
        CallGetService(NS_LANGUAGEATOMSERVICE_CONTRACTID, &gLangService);
    }

    if (gLangService) {
        const char *languages = getenv("LANGUAGE");
        if (languages) {
            const char separator = ':';

            for (const char *pos = languages; true; ++pos) {
                if (*pos == '\0' || *pos == separator) {
                    if (languages < pos &&
                        TryLangForGroup(Substring(languages, pos),
                                        aLangGroup, aFcLang))
                        return;

                    if (*pos == '\0')
                        break;

                    languages = pos + 1;
                }
            }
        }
        const char *ctype = setlocale(LC_CTYPE, nullptr);
        if (ctype &&
            TryLangForGroup(nsDependentCString(ctype), aLangGroup, aFcLang))
            return;
    }

    if (langGroup->defaultLang) {
        aFcLang->Assign(langGroup->defaultLang);
    } else {
        aFcLang->Truncate();
    }
}

nsresult
gfxFontconfigUtils::GetFontListInternal(nsTArray<nsCString>& aListOfFonts,
                                        nsIAtom *aLangGroup)
{
    FcPattern *pat = nullptr;
    FcObjectSet *os = nullptr;
    FcFontSet *fs = nullptr;
    nsresult rv = NS_ERROR_FAILURE;

    aListOfFonts.Clear();

    pat = FcPatternCreate();
    if (!pat)
        goto end;

    os = FcObjectSetBuild(FC_FAMILY, nullptr);
    if (!os)
        goto end;

    
    if (aLangGroup) {
        AddLangGroup(pat, aLangGroup);
    }

    fs = FcFontList(nullptr, pat, os);
    if (!fs)
        goto end;

    for (int i = 0; i < fs->nfont; i++) {
        char *family;

        if (FcPatternGetString(fs->fonts[i], FC_FAMILY, 0,
                               (FcChar8 **) &family) != FcResultMatch)
        {
            continue;
        }

        
        nsAutoCString strFamily(family);
        if (aListOfFonts.Contains(strFamily))
            continue;

        aListOfFonts.AppendElement(strFamily);
    }

    rv = NS_OK;

  end:
    if (NS_FAILED(rv))
        aListOfFonts.Clear();

    if (pat)
        FcPatternDestroy(pat);
    if (os)
        FcObjectSetDestroy(os);
    if (fs)
        FcFontSetDestroy(fs);

    return rv;
}

nsresult
gfxFontconfigUtils::UpdateFontList()
{
    return UpdateFontListInternal(true);
}

nsresult
gfxFontconfigUtils::UpdateFontListInternal(bool aForce)
{
    if (!aForce) {
        
        
        FcInitBringUptoDate();
    } else if (!FcConfigUptoDate(nullptr)) { 
        mLastConfig = nullptr;
        FcInitReinitialize();
    }

    
    
    
    
    
    FcConfig *currentConfig = FcConfigGetCurrent();
    if (currentConfig == mLastConfig)
        return NS_OK;

#ifdef MOZ_BUNDLED_FONTS
    ActivateBundledFonts();
#endif

    
    FcFontSet *fontSets[] = {
        FcConfigGetFonts(currentConfig, FcSetSystem)
#ifdef MOZ_BUNDLED_FONTS
        , FcConfigGetFonts(currentConfig, FcSetApplication)
#endif
    };

    mFontsByFamily.Clear();
    mFontsByFullname.Clear();
    mLangSupportTable.Clear();

    
    for (unsigned fs = 0; fs < ArrayLength(fontSets); ++fs) {
        FcFontSet *fontSet = fontSets[fs];
        if (!fontSet) { 
            continue;
        }
        for (int f = 0; f < fontSet->nfont; ++f) {
            FcPattern *font = fontSet->fonts[f];

            FcChar8 *family;
            for (int v = 0;
             FcPatternGetString(font, FC_FAMILY, v, &family) == FcResultMatch;
             ++v) {
                FontsByFcStrEntry *entry = mFontsByFamily.PutEntry(family);
                if (entry) {
                    bool added = entry->AddFont(font);

                    if (!entry->mKey) {
                        
                        
                        
                        if (added) {
                            entry->mKey = family;
                        } else {
                            mFontsByFamily.RawRemoveEntry(entry);
                        }
                    }
                }
            }
        }
    }

    mLastConfig = currentConfig;
    return NS_OK;
}

nsresult
gfxFontconfigUtils::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    aFamilyName.Truncate();

    
    if (aFontName.EqualsLiteral("serif") ||
        aFontName.EqualsLiteral("sans-serif") ||
        aFontName.EqualsLiteral("monospace")) {
        aFamilyName.Assign(aFontName);
        return NS_OK;
    }

    nsresult rv = UpdateFontListInternal();
    if (NS_FAILED(rv))
        return rv;

    NS_ConvertUTF16toUTF8 fontname(aFontName);

    
    if (!IsExistingFamily(fontname))
        return NS_OK;

    FcPattern *pat = nullptr;
    FcObjectSet *os = nullptr;
    FcFontSet *givenFS = nullptr;
    nsTArray<nsCString> candidates;
    FcFontSet *candidateFS = nullptr;
    rv = NS_ERROR_FAILURE;

    pat = FcPatternCreate();
    if (!pat)
        goto end;

    FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)fontname.get());

    os = FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_INDEX, nullptr);
    if (!os)
        goto end;

    givenFS = FcFontList(nullptr, pat, os);
    if (!givenFS)
        goto end;

    
    

    
    for (int i = 0; i < givenFS->nfont; ++i) {
        char *firstFamily;
        if (FcPatternGetString(givenFS->fonts[i], FC_FAMILY, 0,
                               (FcChar8 **) &firstFamily) != FcResultMatch)
            continue;

        nsDependentCString first(firstFamily);
        if (!candidates.Contains(first)) {
            candidates.AppendElement(first);

            if (fontname.Equals(first)) {
                aFamilyName.Assign(aFontName);
                rv = NS_OK;
                goto end;
            }
        }
    }

    
    
    for (uint32_t j = 0; j < candidates.Length(); ++j) {
        FcPatternDel(pat, FC_FAMILY);
        FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)candidates[j].get());

        candidateFS = FcFontList(nullptr, pat, os);
        if (!candidateFS)
            goto end;

        if (candidateFS->nfont != givenFS->nfont)
            continue;

        bool equal = true;
        for (int i = 0; i < givenFS->nfont; ++i) {
            if (!FcPatternEqual(candidateFS->fonts[i], givenFS->fonts[i])) {
                equal = false;
                break;
            }
        }
        if (equal) {
            AppendUTF8toUTF16(candidates[j], aFamilyName);
            rv = NS_OK;
            goto end;
        }
    }

    
    rv = NS_OK;

  end:
    if (pat)
        FcPatternDestroy(pat);
    if (os)
        FcObjectSetDestroy(os);
    if (givenFS)
        FcFontSetDestroy(givenFS);
    if (candidateFS)
        FcFontSetDestroy(candidateFS);

    return rv;
}

bool
gfxFontconfigUtils::IsExistingFamily(const nsCString& aFamilyName)
{
    return mFontsByFamily.GetEntry(ToFcChar8(aFamilyName)) != nullptr;
}

const nsTArray< nsCountedRef<FcPattern> >&
gfxFontconfigUtils::GetFontsForFamily(const FcChar8 *aFamilyName)
{
    FontsByFcStrEntry *entry = mFontsByFamily.GetEntry(aFamilyName);

    if (!entry)
        return mEmptyPatternArray;

    return entry->GetFonts();
}






bool
gfxFontconfigUtils::GetFullnameFromFamilyAndStyle(FcPattern *aFont,
                                                  nsACString *aFullname)
{
    FcChar8 *family;
    if (FcPatternGetString(aFont, FC_FAMILY, 0, &family) != FcResultMatch)
        return false;

    aFullname->Truncate();
    aFullname->Append(ToCString(family));

    FcChar8 *style;
    if (FcPatternGetString(aFont, FC_STYLE, 0, &style) == FcResultMatch &&
        strcmp(ToCString(style), "Regular") != 0) {
        aFullname->Append(' ');
        aFullname->Append(ToCString(style));
    }

    return true;
}

bool
gfxFontconfigUtils::FontsByFullnameEntry::KeyEquals(KeyTypePointer aKey) const
{
    const FcChar8 *key = mKey;
    
    
    nsAutoCString fullname;
    if (!key) {
        NS_ASSERTION(mFonts.Length(), "No font in FontsByFullnameEntry!");
        GetFullnameFromFamilyAndStyle(mFonts[0], &fullname);

        key = ToFcChar8(fullname);
    }

    return FcStrCmpIgnoreCase(aKey, key) == 0;
}

void
gfxFontconfigUtils::AddFullnameEntries()
{
    
    FcFontSet *fontSets[] = {
        FcConfigGetFonts(nullptr, FcSetSystem)
#ifdef MOZ_BUNDLED_FONTS
        , FcConfigGetFonts(nullptr, FcSetApplication)
#endif
    };

    for (unsigned fs = 0; fs < ArrayLength(fontSets); ++fs) {
        FcFontSet *fontSet = fontSets[fs];
        if (!fontSet) {
            continue;
        }
        
        for (int f = 0; f < fontSet->nfont; ++f) {
            FcPattern *font = fontSet->fonts[f];

            int v = 0;
            FcChar8 *fullname;
            while (FcPatternGetString(font,
                          FC_FULLNAME, v, &fullname) == FcResultMatch) {
                FontsByFullnameEntry *entry =
                    mFontsByFullname.PutEntry(fullname);
                if (entry) {
                    
                    
                    
                    bool added = entry->AddFont(font);
                    
                    
                    
                    
                    
                    if (!entry->mKey && added) {
                        entry->mKey = fullname;
                    }
                }

                ++v;
            }

            
            if (v == 0) {
                nsAutoCString name;
                if (!GetFullnameFromFamilyAndStyle(font, &name))
                    continue;

                FontsByFullnameEntry *entry =
                    mFontsByFullname.PutEntry(ToFcChar8(name));
                if (entry) {
                    entry->AddFont(font);
                    
                    
                    
                }
            }
        }
    }
}

const nsTArray< nsCountedRef<FcPattern> >&
gfxFontconfigUtils::GetFontsForFullname(const FcChar8 *aFullname)
{
    if (mFontsByFullname.Count() == 0) {
        AddFullnameEntries();
    }

    FontsByFullnameEntry *entry = mFontsByFullname.GetEntry(aFullname);

    if (!entry)
        return mEmptyPatternArray;

    return entry->GetFonts();
}

static FcLangResult
CompareLangString(const FcChar8 *aLangA, const FcChar8 *aLangB) {
    FcLangResult result = FcLangDifferentLang;
    for (uint32_t i = 0; ; ++i) {
        FcChar8 a = FcToLower(aLangA[i]);
        FcChar8 b = FcToLower(aLangB[i]);

        if (a != b) {
            if ((a == '\0' && b == '-') || (a == '-' && b == '\0'))
                return FcLangDifferentCountry;

            return result;
        }
        if (a == '\0')
            return FcLangEqual;

        if (a == '-') {
            result = FcLangDifferentCountry;
        }
    }
}


FcLangResult
gfxFontconfigUtils::GetLangSupport(FcPattern *aFont, const FcChar8 *aLang)
{
    
    
    
    
    
    
    
    
    
    
    
    
    FcValue value;
    FcLangResult best = FcLangDifferentLang;
    for (int v = 0;
         FcPatternGet(aFont, FC_LANG, v, &value) == FcResultMatch;
         ++v) {

        FcLangResult support;
        switch (value.type) {
            case FcTypeLangSet:
                support = FcLangSetHasLang(value.u.l, aLang);
                break;
            case FcTypeString:
                support = CompareLangString(value.u.s, aLang);
                break;
            default:
                
                continue;
        }

        if (support < best) { 
            if (support == FcLangEqual)
                return support;
            best = support;
        }        
    }

    return best;
}

gfxFontconfigUtils::LangSupportEntry *
gfxFontconfigUtils::GetLangSupportEntry(const FcChar8 *aLang, bool aWithFonts)
{
    
    
    
    

    LangSupportEntry *entry = mLangSupportTable.PutEntry(aLang);
    if (!entry)
        return nullptr;

    FcLangResult best = FcLangDifferentLang;

    if (!entry->IsKeyInitialized()) {
        entry->InitKey(aLang);
    } else {
        
        if (!aWithFonts)
            return entry;

        best = entry->mSupport;
        
        
        if (best == FcLangDifferentLang || entry->mFonts.Length() > 0)
            return entry;
    }

    
    FcFontSet *fontSets[] = {
        FcConfigGetFonts(nullptr, FcSetSystem)
#ifdef MOZ_BUNDLED_FONTS
        , FcConfigGetFonts(nullptr, FcSetApplication)
#endif
    };

    nsAutoTArray<FcPattern*,100> fonts;

    for (unsigned fs = 0; fs < ArrayLength(fontSets); ++fs) {
        FcFontSet *fontSet = fontSets[fs];
        if (!fontSet) {
            continue;
        }
        for (int f = 0; f < fontSet->nfont; ++f) {
            FcPattern *font = fontSet->fonts[f];

            FcLangResult support = GetLangSupport(font, aLang);

            if (support < best) { 
                best = support;
                if (aWithFonts) {
                    fonts.Clear();
                } else if (best == FcLangEqual) {
                    break;
                }
            }

            
            
            
            
            
            if (aWithFonts && support != FcLangDifferentLang &&
                support == best) {
                fonts.AppendElement(font);
            }
        }
    }

    entry->mSupport = best;
    if (aWithFonts) {
        if (fonts.Length() != 0) {
            entry->mFonts.AppendElements(fonts.Elements(), fonts.Length());
        } else if (best != FcLangDifferentLang) {
            
            
            
            
            
            mLastConfig = nullptr; 
            UpdateFontListInternal(true);
            return GetLangSupportEntry(aLang, aWithFonts);
        }
    }

    return entry;
}

FcLangResult
gfxFontconfigUtils::GetBestLangSupport(const FcChar8 *aLang)
{
    UpdateFontListInternal();

    LangSupportEntry *entry = GetLangSupportEntry(aLang, false);
    if (!entry)
        return FcLangEqual;

    return entry->mSupport;
}

const nsTArray< nsCountedRef<FcPattern> >&
gfxFontconfigUtils::GetFontsForLang(const FcChar8 *aLang)
{
    LangSupportEntry *entry = GetLangSupportEntry(aLang, true);
    if (!entry)
        return mEmptyPatternArray;

    return entry->mFonts;
}

#ifdef MOZ_BUNDLED_FONTS

void
gfxFontconfigUtils::ActivateBundledFonts()
{
    if (!mBundledFontsInitialized) {
        mBundledFontsInitialized = true;
        nsCOMPtr<nsIFile> localDir;
        nsresult rv = NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(localDir));
        if (NS_FAILED(rv)) {
            return;
        }
        if (NS_FAILED(localDir->Append(NS_LITERAL_STRING("fonts")))) {
            return;
        }
        bool isDir;
        if (NS_FAILED(localDir->IsDirectory(&isDir)) || !isDir) {
            return;
        }
        if (NS_FAILED(localDir->GetNativePath(mBundledFontsPath))) {
            return;
        }
    }
    if (!mBundledFontsPath.IsEmpty()) {
        FcConfigAppFontAddDir(nullptr, (const FcChar8*)mBundledFontsPath.get());
    }
}

#endif
