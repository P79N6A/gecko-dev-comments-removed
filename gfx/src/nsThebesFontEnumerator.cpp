





































#include "nsThebesFontEnumerator.h"

#include "nsMemory.h"

#include "gfxPlatform.h"
#include "nsTArray.h"
#include "nsIAtom.h"

NS_IMPL_ISUPPORTS1(nsThebesFontEnumerator, nsIFontEnumerator)

nsThebesFontEnumerator::nsThebesFontEnumerator()
{
}

NS_IMETHODIMP
nsThebesFontEnumerator::EnumerateAllFonts(PRUint32 *aCount,
                                          PRUnichar ***aResult)
{
    return EnumerateFonts (nsnull, nsnull, aCount, aResult);
}

NS_IMETHODIMP
nsThebesFontEnumerator::EnumerateFonts(const char *aLangGroup,
                                       const char *aGeneric,
                                       PRUint32 *aCount,
                                       PRUnichar ***aResult)
{
    NS_ENSURE_ARG_POINTER(aCount);
    NS_ENSURE_ARG_POINTER(aResult);

    nsTArray<nsString> fontList;

    nsCAutoString generic;
    if (aGeneric)
        generic.Assign(aGeneric);
    else
        generic.SetIsVoid(PR_TRUE);

    nsCOMPtr<nsIAtom> langGroupAtom;
    if (aLangGroup) {
        nsCAutoString lowered;
        lowered.Assign(aLangGroup);
        ToLowerCase(lowered);
        langGroupAtom = do_GetAtom(lowered);
    }

    nsresult rv = gfxPlatform::GetPlatform()->GetFontList(langGroupAtom, generic, fontList);

    if (NS_FAILED(rv)) {
        *aCount = 0;
        *aResult = nsnull;
        
        return NS_OK;
    }

    PRUnichar **fs = static_cast<PRUnichar **>
                                (nsMemory::Alloc(fontList.Length() * sizeof(PRUnichar*)));
    for (PRUint32 i = 0; i < fontList.Length(); i++) {
        fs[i] = ToNewUnicode(fontList[i]);
    }

    *aResult = fs;
    *aCount = fontList.Length();

    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontEnumerator::HaveFontFor(const char *aLangGroup,
                                    bool *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);

    *aResult = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontEnumerator::GetDefaultFont(const char *aLangGroup,
                                       const char *aGeneric,
                                       PRUnichar **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontEnumerator::UpdateFontList(bool *_retval)
{
    gfxPlatform::GetPlatform()->UpdateFontList();
    *_retval = PR_FALSE; 
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontEnumerator::GetStandardFamilyName(const PRUnichar *aName,
                                              PRUnichar **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aName);

    nsAutoString name(aName);
    if (name.IsEmpty()) {
        *aResult = nsnull;
        return NS_OK;
    }

    nsAutoString family;
    nsresult rv = gfxPlatform::GetPlatform()->
        GetStandardFamilyName(nsDependentString(aName), family);
    if (NS_FAILED(rv) || family.IsEmpty()) {
        *aResult = nsnull;
        return NS_OK;
    }
    *aResult = ToNewUnicode(family);
    return NS_OK;
}
