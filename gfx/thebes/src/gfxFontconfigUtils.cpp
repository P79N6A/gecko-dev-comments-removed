





































#include "gfxFontconfigUtils.h"

#include <fontconfig/fontconfig.h>

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsServiceManagerUtils.h"

#include "nsIAtom.h"
#include "nsCRT.h"

 gfxFontconfigUtils* gfxFontconfigUtils::sUtils = nsnull;

gfxFontconfigUtils::gfxFontconfigUtils()
{
    mAliasTable.Init(50);
    UpdateFontListInternal(PR_TRUE);
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

    PRInt32 serif = 0, sansSerif = 0, monospace = 0, nGenerics;

    
    
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
    nGenerics = serif + sansSerif + monospace;

    if (serif)
        aListOfFonts.AppendString(NS_LITERAL_STRING("serif"));
    if (sansSerif)
        aListOfFonts.AppendString(NS_LITERAL_STRING("sans-serif"));
    if (monospace)
        aListOfFonts.AppendString(NS_LITERAL_STRING("monospace"));

    aListOfFonts.Sort();

    return NS_OK;
}

struct MozGtkLangGroup {
    const char    *mozLangGroup;
    const FcChar8 *Lang;
};

const MozGtkLangGroup MozGtkLangGroups[] = {
    { "x-western",      (const FcChar8 *)"en" },
    { "x-central-euro", (const FcChar8 *)"pl" },
    { "x-cyrillic",     (const FcChar8 *)"ru" },
    { "x-baltic",       (const FcChar8 *)"lv" },
    { "x-devanagari",   (const FcChar8 *)"hi" },
    { "x-tamil",        (const FcChar8 *)"ta" },
    { "x-armn",         (const FcChar8 *)"hy" },
    { "x-beng",         (const FcChar8 *)"bn" },
    { "x-cans",         (const FcChar8 *)"iu" },
    { "x-ethi",         (const FcChar8 *)"am" },
    { "x-geor",         (const FcChar8 *)"ka" },
    { "x-gujr",         (const FcChar8 *)"gu" },
    { "x-guru",         (const FcChar8 *)"pa" },
    { "x-khmr",         (const FcChar8 *)"km" },
    { "x-mlym",         (const FcChar8 *)"ml" },
    { "x-unicode",                       0    },
    { "x-user-def",                      0    }
};

static const MozGtkLangGroup*
NS_FindFCLangGroup (nsACString &aLangGroup)
{
    for (unsigned int i=0; i < NS_ARRAY_LENGTH(MozGtkLangGroups); ++i) {
        if (aLangGroup.Equals(MozGtkLangGroups[i].mozLangGroup,
                              nsCaseInsensitiveCStringComparator())) {
            return &MozGtkLangGroups[i];
        }
    }

    return nsnull;
}

static void
NS_AddLangGroup(FcPattern *aPattern, nsIAtom *aLangGroup)
{
    
    nsCAutoString cname;
    aLangGroup->ToUTF8String(cname);

    
    
    const struct MozGtkLangGroup *langGroup;
    langGroup = NS_FindFCLangGroup(cname);

    
    
    
    
    
    if (!langGroup)
        FcPatternAddString(aPattern, FC_LANG, (FcChar8 *)cname.get());
    else if (langGroup->Lang)
        FcPatternAddString(aPattern, FC_LANG, (FcChar8 *)langGroup->Lang);
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
        nsCOMPtr<nsIAtom> langAtom = do_GetAtom(*aLangGroup);
        NS_AddLangGroup(pat, langAtom);
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
    if (!aForce && FcConfigUptoDate(NULL))
        return NS_OK;

    FcInitReinitialize();

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
    rv = prefBranch->GetCharPref("font.alias-list", getter_Copies(list));
    if (NS_FAILED(rv))
        return NS_OK;

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

    if (mAliasForMultiFonts.Count() == 0)
        return NS_OK;

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

    result = fs->nfont;
    NS_ASSERTION(result == 0 || result == 1, "What's this case?");

    if (result > 0)
        mAliasForSingleFont.AppendCString(aFontName);
    else
        mNonExistingFonts.AppendCString(aFontName);

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

