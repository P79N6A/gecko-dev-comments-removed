






































#include "gfxFontconfigUtils.h"
#include "gfxFont.h"
#include "gfxAtoms.h"

#include <locale.h>
#include <fontconfig/fontconfig.h>

#include "nsServiceManagerUtils.h"
#include "nsILanguageAtomService.h"
#include "nsTArray.h"
#include "mozilla/Preferences.h"

#include "nsIAtom.h"
#include "nsCRT.h"

using namespace mozilla;

 gfxFontconfigUtils* gfxFontconfigUtils::sUtils = nsnull;
static nsILanguageAtomService* gLangService = nsnull;

 void
gfxFontconfigUtils::Shutdown() {
    if (sUtils) {
        delete sUtils;
        sUtils = nsnull;
    }
    NS_IF_RELEASE(gLangService);
}

 PRUint8
gfxFontconfigUtils::FcSlantToThebesStyle(int aFcSlant)
{
    switch (aFcSlant) {
        case FC_SLANT_ITALIC:
            return FONT_STYLE_ITALIC;
        case FC_SLANT_OBLIQUE:
            return FONT_STYLE_OBLIQUE;
        default:
            return FONT_STYLE_NORMAL;
    }
}

 PRUint8
gfxFontconfigUtils::GetThebesStyle(FcPattern *aPattern)
{
    int slant;
    if (FcPatternGetInteger(aPattern, FC_SLANT, 0, &slant) != FcResultMatch) {
        return FONT_STYLE_NORMAL;
    }

    return FcSlantToThebesStyle(slant);
}

 int
gfxFontconfigUtils::GetFcSlant(const gfxFontStyle& aFontStyle)
{
    if (aFontStyle.style == FONT_STYLE_ITALIC)
        return FC_SLANT_ITALIC;
    if (aFontStyle.style == FONT_STYLE_OBLIQUE)
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

 PRUint16
gfxFontconfigUtils::GetThebesWeight(FcPattern *aPattern)
{
    int weight;
    if (FcPatternGetInteger(aPattern, FC_WEIGHT, 0, &weight) != FcResultMatch)
        return FONT_WEIGHT_NORMAL;

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
gfxFontconfigUtils::FcWeightForBaseWeight(PRInt8 aBaseWeight)
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




 int
GuessFcWeight(const gfxFontStyle& aFontStyle)
{
    







    PRInt8 weight = aFontStyle.ComputeWeight();

    
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
    
    nsCAutoString lang;
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

    if (aLang) {
        AddString(pattern, FC_LANG, aLang);
    }

    PRBool useWeakBinding = PR_FALSE;
    for (PRUint32 i = 0; i < aFamilies.Length(); ++i) {
        NS_ConvertUTF16toUTF8 family(aFamilies[i]);
        if (!useWeakBinding) {
            AddString(pattern, FC_FAMILY, family.get());

            
            
            
            
            
            
            for (PRUint32 g = 0;
                 g < NS_ARRAY_LENGTH(sFontconfigGenerics);
                 ++g) {
                if (FcStrCmpIgnoreCase(ToFcChar8(sFontconfigGenerics[g]),
                                       ToFcChar8(family.get()))) {
                    useWeakBinding = PR_TRUE;
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
    : mLastConfig(NULL)
{
    mFontsByFamily.Init(50);
    mFontsByFullname.Init(50);
    mLangSupportTable.Init(20);
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

    for (PRUint32 i = 0; i < fonts.Length(); ++i) {
        aListOfFonts.AppendElement(NS_ConvertUTF8toUTF16(fonts[i]));
    }

    aListOfFonts.Sort();

    PRInt32 serif = 0, sansSerif = 0, monospace = 0;

    
    
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
    { gfxAtoms::x_western,      "en" },
    { gfxAtoms::x_central_euro, "pl" },
    { gfxAtoms::x_cyrillic,     "ru" },
    { gfxAtoms::x_baltic,       "lv" },
    { gfxAtoms::x_devanagari,   "hi" },
    { gfxAtoms::x_tamil,        "ta" },
    { gfxAtoms::x_armn,         "hy" },
    { gfxAtoms::x_beng,         "bn" },
    { gfxAtoms::x_cans,         "iu" },
    { gfxAtoms::x_ethi,         "am" },
    { gfxAtoms::x_geor,         "ka" },
    { gfxAtoms::x_gujr,         "gu" },
    { gfxAtoms::x_guru,         "pa" },
    { gfxAtoms::x_khmr,         "km" },
    { gfxAtoms::x_knda,         "kn" },
    { gfxAtoms::x_mlym,         "ml" },
    { gfxAtoms::x_orya,         "or" },
    { gfxAtoms::x_sinh,         "si" },
    { gfxAtoms::x_telu,         "te" },
    { gfxAtoms::x_tibt,         "bo" },
    { gfxAtoms::x_unicode,      0    },
    { gfxAtoms::x_user_def,     0    }
};

static PRBool
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
    NS_PRECONDITION(aFcLang != nsnull, "aFcLang must not be NULL");

    const MozLangGroupData *langGroup = nsnull;

    for (unsigned int i = 0; i < NS_ARRAY_LENGTH(MozLangGroups); ++i) {
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

            for (const char *pos = languages; PR_TRUE; ++pos) {
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
        const char *ctype = setlocale(LC_CTYPE, NULL);
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
    FcPattern *pat = NULL;
    FcObjectSet *os = NULL;
    FcFontSet *fs = NULL;
    nsresult rv = NS_ERROR_FAILURE;

    aListOfFonts.Clear();

    pat = FcPatternCreate();
    if (!pat)
        goto end;

    os = FcObjectSetBuild(FC_FAMILY, NULL);
    if (!os)
        goto end;

    
    if (aLangGroup) {
        AddLangGroup(pat, aLangGroup);
    }

    fs = FcFontList(NULL, pat, os);
    if (!fs)
        goto end;

    for (int i = 0; i < fs->nfont; i++) {
        char *family;

        if (FcPatternGetString(fs->fonts[i], FC_FAMILY, 0,
                               (FcChar8 **) &family) != FcResultMatch)
        {
            continue;
        }

        
        nsCAutoString strFamily(family);
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
    return UpdateFontListInternal(PR_TRUE);
}

nsresult
gfxFontconfigUtils::UpdateFontListInternal(PRBool aForce)
{
    if (!aForce) {
        
        
        FcInitBringUptoDate();
    } else if (!FcConfigUptoDate(NULL)) { 
        mLastConfig = NULL;
        FcInitReinitialize();
    }

    
    
    
    
    
    FcConfig *currentConfig = FcConfigGetCurrent();
    if (currentConfig == mLastConfig)
        return NS_OK;

    
    FcFontSet *fontSet = FcConfigGetFonts(currentConfig, FcSetSystem);

    mFontsByFamily.Clear();
    mFontsByFullname.Clear();
    mLangSupportTable.Clear();
    mAliasForMultiFonts.Clear();

    
    for (int f = 0; f < fontSet->nfont; ++f) {
        FcPattern *font = fontSet->fonts[f];

        FcChar8 *family;
        for (int v = 0;
             FcPatternGetString(font, FC_FAMILY, v, &family) == FcResultMatch;
             ++v) {
            FontsByFcStrEntry *entry = mFontsByFamily.PutEntry(family);
            if (entry) {
                PRBool added = entry->AddFont(font);

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

    
    
    
    
    
    NS_ENSURE_TRUE(Preferences::GetRootBranch(), NS_ERROR_FAILURE);
    nsAdoptingCString list = Preferences::GetCString("font.alias-list");

    if (!list.IsEmpty()) {
        const char kComma = ',';
        const char *p, *p_end;
        list.BeginReading(p);
        list.EndReading(p_end);
        while (p < p_end) {
            while (nsCRT::IsAsciiSpace(*p)) {
                if (++p == p_end)
                    break;
            }
            if (p == p_end)
                break;
            const char *start = p;
            while (++p != p_end && *p != kComma)
                 ;
            nsCAutoString name(Substring(start, p));
            name.CompressWhitespace(PR_FALSE, PR_TRUE);
            mAliasForMultiFonts.AppendElement(name);
            p++;
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

    FcPattern *pat = NULL;
    FcObjectSet *os = NULL;
    FcFontSet *givenFS = NULL;
    nsTArray<nsCString> candidates;
    FcFontSet *candidateFS = NULL;
    rv = NS_ERROR_FAILURE;

    pat = FcPatternCreate();
    if (!pat)
        goto end;

    FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)fontname.get());

    os = FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_INDEX, NULL);
    if (!os)
        goto end;

    givenFS = FcFontList(NULL, pat, os);
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

    
    
    for (PRUint32 j = 0; j < candidates.Length(); ++j) {
        FcPatternDel(pat, FC_FAMILY);
        FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)candidates[j].get());

        candidateFS = FcFontList(NULL, pat, os);
        if (!candidateFS)
            goto end;

        if (candidateFS->nfont != givenFS->nfont)
            continue;

        PRBool equal = PR_TRUE;
        for (int i = 0; i < givenFS->nfont; ++i) {
            if (!FcPatternEqual(candidateFS->fonts[i], givenFS->fonts[i])) {
                equal = PR_FALSE;
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

nsresult
gfxFontconfigUtils::ResolveFontName(const nsAString& aFontName,
                                    gfxPlatform::FontResolverCallback aCallback,
                                    void *aClosure,
                                    PRBool& aAborted)
{
    aAborted = PR_FALSE;

    nsresult rv = UpdateFontListInternal();
    if (NS_FAILED(rv))
        return rv;

    NS_ConvertUTF16toUTF8 fontname(aFontName);
    
    
    
    
    
    
    
    
    
    
    
    
    if (IsExistingFamily(fontname) ||
        mAliasForMultiFonts.Contains(fontname, gfxIgnoreCaseCStringComparator()))
        aAborted = !(*aCallback)(aFontName, aClosure);

    return NS_OK;
}

PRBool
gfxFontconfigUtils::IsExistingFamily(const nsCString& aFamilyName)
{
    return mFontsByFamily.GetEntry(ToFcChar8(aFamilyName)) != nsnull;
}

const nsTArray< nsCountedRef<FcPattern> >&
gfxFontconfigUtils::GetFontsForFamily(const FcChar8 *aFamilyName)
{
    FontsByFcStrEntry *entry = mFontsByFamily.GetEntry(aFamilyName);

    if (!entry)
        return mEmptyPatternArray;

    return entry->GetFonts();
}






PRBool
gfxFontconfigUtils::GetFullnameFromFamilyAndStyle(FcPattern *aFont,
                                                  nsACString *aFullname)
{
    FcChar8 *family;
    if (FcPatternGetString(aFont, FC_FAMILY, 0, &family) != FcResultMatch)
        return PR_FALSE;

    aFullname->Truncate();
    aFullname->Append(ToCString(family));

    FcChar8 *style;
    if (FcPatternGetString(aFont, FC_STYLE, 0, &style) == FcResultMatch &&
        strcmp(ToCString(style), "Regular") != 0) {
        aFullname->Append(' ');
        aFullname->Append(ToCString(style));
    }

    return PR_TRUE;
}

PRBool
gfxFontconfigUtils::FontsByFullnameEntry::KeyEquals(KeyTypePointer aKey) const
{
    const FcChar8 *key = mKey;
    
    nsCAutoString fullname;
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
    
    FcFontSet *fontSet = FcConfigGetFonts(NULL, FcSetSystem);

    
    for (int f = 0; f < fontSet->nfont; ++f) {
        FcPattern *font = fontSet->fonts[f];

        int v = 0;
        FcChar8 *fullname;
        while (FcPatternGetString(font,
                                  FC_FULLNAME, v, &fullname) == FcResultMatch) {
            FontsByFullnameEntry *entry = mFontsByFullname.PutEntry(fullname);
            if (entry) {
                
                
                
                PRBool added = entry->AddFont(font);
                
                
                
                
                
                if (!entry->mKey && added) {
                    entry->mKey = fullname;
                }
            }

            ++v;
        }

        
        if (v == 0) {
            nsCAutoString name;
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
    for (PRUint32 i = 0; ; ++i) {
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
gfxFontconfigUtils::GetLangSupportEntry(const FcChar8 *aLang, PRBool aWithFonts)
{
    
    
    
    

    LangSupportEntry *entry = mLangSupportTable.PutEntry(aLang);
    if (!entry)
        return nsnull;

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

    
    FcFontSet *fontSet = FcConfigGetFonts(NULL, FcSetSystem);

    nsAutoTArray<FcPattern*,100> fonts;

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

        
        
        
        
        
        if (aWithFonts && support != FcLangDifferentLang && support == best) {
            fonts.AppendElement(font);
        }
    }

    entry->mSupport = best;
    if (aWithFonts) {
        if (fonts.Length() != 0) {
            entry->mFonts.AppendElements(fonts.Elements(), fonts.Length());
        } else if (best != FcLangDifferentLang) {
            
            
            
            
            
            mLastConfig = NULL; 
            UpdateFontListInternal(PR_TRUE);
            return GetLangSupportEntry(aLang, aWithFonts);
        }
    }

    return entry;
}

FcLangResult
gfxFontconfigUtils::GetBestLangSupport(const FcChar8 *aLang)
{
    UpdateFontListInternal();

    LangSupportEntry *entry = GetLangSupportEntry(aLang, PR_FALSE);
    if (!entry)
        return FcLangEqual;

    return entry->mSupport;
}

const nsTArray< nsCountedRef<FcPattern> >&
gfxFontconfigUtils::GetFontsForLang(const FcChar8 *aLang)
{
    LangSupportEntry *entry = GetLangSupportEntry(aLang, PR_TRUE);
    if (!entry)
        return mEmptyPatternArray;

    return entry->mFonts;
}

PRBool
gfxFontNameList::Exists(nsAString& aName) {
    for (PRUint32 i = 0; i < Length(); i++) {
        if (aName.Equals(ElementAt(i)))
            return PR_TRUE;
    }
    return PR_FALSE;
}
