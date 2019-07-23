






































#include "gfxFontconfigUtils.h"
#include "gfxFont.h"

#include <locale.h>
#include <fontconfig/fontconfig.h>

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsServiceManagerUtils.h"
#include "nsILanguageAtomService.h"

#include "nsIAtom.h"
#include "nsCRT.h"

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
gfxFontconfigUtils::GetThebesStyle(FcPattern *aPattern)
{
    int slant;
    if (FcPatternGetInteger(aPattern, FC_SLANT, 0, &slant) == FcResultMatch) {
        if (slant == FC_SLANT_ITALIC)
            return FONT_STYLE_ITALIC;
        if (slant == FC_SLANT_OBLIQUE)
            return FONT_STYLE_OBLIQUE;
    }

    return FONT_STYLE_NORMAL;
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

gfxFontconfigUtils::gfxFontconfigUtils()
    : mLastConfig(NULL)
{
    mAliasTable.Init(50);
}

nsresult
gfxFontconfigUtils::GetFontList(const nsACString& aLangGroup,
                                const nsACString& aGenericFamily,
                                nsStringArray& aListOfFonts)
{
    aListOfFonts.Clear();

    nsresult rv = UpdateFontListInternal();
    if (NS_FAILED(rv))
        return rv;

    nsCStringArray tmpFonts;
    nsCStringArray *fonts = &mFonts;
    if (!aLangGroup.IsEmpty() || !aGenericFamily.IsEmpty()) {
        rv = GetFontListInternal(tmpFonts, &aLangGroup);
        if (NS_FAILED(rv))
            return rv;
        fonts = &tmpFonts;
    }

    for (PRInt32 i = 0; i < fonts->Count(); ++i)
         aListOfFonts.AppendString(NS_ConvertUTF8toUTF16(*fonts->CStringAt(i)));

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
        aListOfFonts.InsertStringAt(NS_LITERAL_STRING("monospace"), 0);
    if (sansSerif)
        aListOfFonts.InsertStringAt(NS_LITERAL_STRING("sans-serif"), 0);
    if (serif)
        aListOfFonts.InsertStringAt(NS_LITERAL_STRING("serif"), 0);

    return NS_OK;
}

struct MozLangGroupData {
    const char *mozLangGroup;
    const char *defaultLang;
};

const MozLangGroupData MozLangGroups[] = {
    { "x-western",      "en" },
    { "x-central-euro", "pl" },
    { "x-cyrillic",     "ru" },
    { "x-baltic",       "lv" },
    { "x-devanagari",   "hi" },
    { "x-tamil",        "ta" },
    { "x-armn",         "hy" },
    { "x-beng",         "bn" },
    { "x-cans",         "iu" },
    { "x-ethi",         "am" },
    { "x-geor",         "ka" },
    { "x-gujr",         "gu" },
    { "x-guru",         "pa" },
    { "x-khmr",         "km" },
    { "x-knda",         "kn" },
    { "x-mlym",         "ml" },
    { "x-orya",         "or" },
    { "x-sinh",         "si" },
    { "x-telu",         "te" },
    { "x-unicode",      0    },
    { "x-user-def",     0    }
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
        gLangService->LookupLanguage(NS_ConvertUTF8toUTF16(*aFcLang));

    return atom == aLangGroup;
}

 void
gfxFontconfigUtils::GetSampleLangForGroup(const nsACString& aLangGroup,
                                          nsACString *aFcLang)
{
    NS_PRECONDITION(aFcLang != nsnull, "aFcLang must not be NULL");

    const MozLangGroupData *langGroup = nsnull;

    for (unsigned int i=0; i < NS_ARRAY_LENGTH(MozLangGroups); ++i) {
        if (aLangGroup.Equals(MozLangGroups[i].mozLangGroup,
                              nsCaseInsensitiveCStringComparator())) {
            langGroup = &MozLangGroups[i];
            break;
        }
    }

    if (!langGroup) {
        
        
        aFcLang->Assign(aLangGroup);
        return;
    }

    
    
    if (!gLangService) {
        CallGetService(NS_LANGUAGEATOMSERVICE_CONTRACTID, &gLangService);
    }

    if (gLangService) {
        nsRefPtr<nsIAtom> langGroupAtom = do_GetAtom(langGroup->mozLangGroup);

        const char *languages = getenv("LANGUAGE");
        if (languages) {
            const char separator = ':';

            for (const char *pos = languages; PR_TRUE; ++pos) {
                if (*pos == '\0' || *pos == separator) {
                    if (languages < pos &&
                        TryLangForGroup(Substring(languages, pos),
                                        langGroupAtom, aFcLang))
                        return;

                    if (*pos == '\0')
                        break;

                    languages = pos + 1;
                }
            }
        }
        const char *ctype = setlocale(LC_CTYPE, NULL);
        if (ctype &&
            TryLangForGroup(nsDependentCString(ctype), langGroupAtom, aFcLang))
            return;
    }

    if (langGroup->defaultLang) {
        aFcLang->Assign(langGroup->defaultLang);
    } else {
        aFcLang->Truncate();
    }
}

static void
AddLangGroup(FcPattern *aPattern, const nsACString& aLangGroup)
{
    
    nsCAutoString lang;
    gfxFontconfigUtils::GetSampleLangForGroup(aLangGroup, &lang);

    if (!lang.IsEmpty()) {
        
        const FcChar8 *fcString = reinterpret_cast<const FcChar8*>(lang.get());
        
        FcPatternAddString(aPattern, FC_LANG, const_cast<FcChar8*>(fcString));
    }
}


nsresult
gfxFontconfigUtils::GetFontListInternal(nsCStringArray& aListOfFonts,
                                        const nsACString *aLangGroup)
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

    
    if (aLangGroup && !aLangGroup->IsEmpty()) {
        AddLangGroup(pat, *aLangGroup);
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
        if (aListOfFonts.IndexOf(strFamily) >= 0)
            continue;

        aListOfFonts.AppendCString(strFamily);
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

    mFonts.Clear();
    mAliasForSingleFont.Clear();
    mAliasForMultiFonts.Clear();
    mNonExistingFonts.Clear();

    mAliasTable.Clear();

    nsresult rv = GetFontListInternal(mFonts);
    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    
    nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIPrefBranch> prefBranch;
    prefs->GetBranch(0, getter_AddRefs(prefBranch));
    if (!prefBranch)
        return NS_ERROR_FAILURE;

    nsXPIDLCString list;
    prefBranch->GetCharPref("font.alias-list", getter_Copies(list));

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
            mAliasForMultiFonts.AppendCString(name);
            p++;
        }
    }

    for (PRInt32 i = 0; i < mAliasForMultiFonts.Count(); i++) {
        nsRefPtr<gfxFontNameList> fonts = new gfxFontNameList;
        nsCAutoString fontname(*mAliasForMultiFonts.CStringAt(i));
        rv = GetResolvedFonts(fontname, fonts);
        if (NS_FAILED(rv))
            return rv;

        nsCAutoString key;
        ToLowerCase(fontname, key);
        mAliasTable.Put(key, fonts);
    }

    mLastConfig = currentConfig;
    return NS_OK;
}

nsresult
gfxFontconfigUtils::GetResolvedFonts(const nsACString& aName,
                                     gfxFontNameList* aResult)
{
    FcPattern *pat = NULL;
    FcFontSet *fs = NULL;
    FcResult fresult;
    aResult->Clear();
    nsresult rv = NS_ERROR_FAILURE;

    pat = FcPatternCreate();
    if (!pat)
        goto end;

    FcDefaultSubstitute(pat);
    FcPatternAddString(pat, FC_FAMILY,
                       (FcChar8 *)nsPromiseFlatCString(aName).get());
    
    FcPatternDel(pat, FC_LANG);
    FcConfigSubstitute(NULL, pat, FcMatchPattern);

    fs = FcFontSort(NULL, pat, FcTrue, NULL, &fresult);
    if (!fs)
        goto end;

    rv = NS_OK;
    for (int i = 0; i < fs->nfont; i++) {
        char *family;

        if (FcPatternGetString(fs->fonts[i], FC_FAMILY, 0,
                               (FcChar8 **) &family) != FcResultMatch ||
            mAliasForMultiFonts.IndexOfIgnoreCase(nsDependentCString(family)) >= 0 ||
            IsExistingFont(nsDependentCString(family)) == 0)
        {
            continue;
        }
        NS_ConvertUTF8toUTF16 actualName(family);
        if (aResult->Exists(actualName))
            continue;
        aResult->AppendElement(actualName);
    }

  end:
    if (pat)
        FcPatternDestroy(pat);
    if (fs)
        FcFontSetDestroy(fs);
    return rv;
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

    if (mFonts.IndexOf(fontname) >= 0) {
        aFamilyName.Assign(aFontName);
        return NS_OK;
    }

    if (mNonExistingFonts.IndexOf(fontname) >= 0)
        return NS_OK;

    FcPattern *pat = NULL;
    FcObjectSet *os = NULL;
    FcFontSet *givenFS = NULL;
    nsCStringArray candidates;
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
        if (candidates.IndexOf(first) < 0) {
            candidates.AppendCString(first);

            if (fontname.Equals(first)) {
                aFamilyName.Assign(aFontName);
                rv = NS_OK;
                goto end;
            }
        }
    }

    
    
    for (PRInt32 j = 0; j < candidates.Count(); ++j) {
        FcPatternDel(pat, FC_FAMILY);
        FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)candidates[j]->get());

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
            AppendUTF8toUTF16(*candidates[j], aFamilyName);
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
    if (mAliasForMultiFonts.IndexOfIgnoreCase(fontname) >= 0) {
        nsCAutoString key;
        ToLowerCase(fontname, key);
        nsRefPtr<gfxFontNameList> fonts;
        if (!mAliasTable.Get(key, &fonts))
            NS_ERROR("The mAliasTable was broken!");
        for (PRUint32 i = 0; i < fonts->Length(); i++) {
            aAborted = !(*aCallback)(fonts->ElementAt(i), aClosure);
            if (aAborted)
                break;
        }
    } else {
        PRInt32 result = IsExistingFont(fontname);
        if (result < 0)
            return NS_ERROR_FAILURE;

        if (result > 0)
            aAborted = !(*aCallback)(aFontName, aClosure);
    }

    return NS_OK;
}

PRInt32
gfxFontconfigUtils::IsExistingFont(const nsACString &aFontName)
{
    
    
    if (mNonExistingFonts.IndexOf(aFontName) >= 0)
        return 0;
    if (mAliasForSingleFont.IndexOf(aFontName) >= 0)
        return 1;
    if (mFonts.IndexOf(aFontName) >= 0)
        return 1;

    
    
    
    
    
    
    
    

    FcPattern *pat = NULL;
    FcObjectSet *os = NULL;
    FcFontSet *fs = NULL;
    PRInt32 result = -1;

    pat = FcPatternCreate();
    if (!pat)
        goto end;

    FcPatternAddString(pat, FC_FAMILY,
                       (FcChar8 *)nsPromiseFlatCString(aFontName).get());

    os = FcObjectSetBuild(FC_FAMILY, NULL);
    if (!os)
        goto end;

    fs = FcFontList(NULL, pat, os);
    if (!fs)
        goto end;

    
    if (fs->nfont > 0) {
        mAliasForSingleFont.AppendCString(aFontName);
        result = 1;
    } else {
        mNonExistingFonts.AppendCString(aFontName);
        result = 0;
    }

  end:
    if (pat)
        FcPatternDestroy(pat);
    if (os)
        FcObjectSetDestroy(os);
    if (fs)
        FcFontSetDestroy(fs);
    return result;
}

PRBool
gfxFontNameList::Exists(nsAString& aName) {
    for (PRUint32 i = 0; i < Length(); i++) {
        if (aName.Equals(ElementAt(i)))
            return PR_TRUE;
    }
    return PR_FALSE;
}

