





































#include "nsThebesFontEnumerator.h"

#include "nsMemory.h"

#include "gfxPlatform.h"

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

    nsStringArray fontList;

    nsCAutoString langGroup;
    nsCAutoString generic;

    if (aLangGroup)
        langGroup.Assign(aLangGroup);
    else
        langGroup.SetIsVoid(PR_TRUE);

    if (aGeneric)
        generic.Assign(aGeneric);
    else
        generic.SetIsVoid(PR_TRUE);

    nsresult rv = gfxPlatform::GetPlatform()->GetFontList(langGroup, generic, fontList);

    if (NS_FAILED(rv)) {
        *aCount = 0;
        *aResult = nsnull;
        
        return NS_OK;
    }

    PRUnichar **fs = NS_STATIC_CAST(PRUnichar **,
                                    nsMemory::Alloc(fontList.Count() * sizeof(PRUnichar*)));
    for (int i = 0; i < fontList.Count(); i++) {
        fs[i] = ToNewUnicode(*fontList[i]);
    }

    *aResult = fs;
    *aCount = fontList.Count();

    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontEnumerator::HaveFontFor(const char *aLangGroup,
                                    PRBool *aResult)
{
    NS_ENSURE_ARG_POINTER(*aResult);
    NS_ENSURE_ARG_POINTER(*aLangGroup);

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
nsThebesFontEnumerator::UpdateFontList(PRBool *_retval)
{
    nsresult rv = gfxPlatform::GetPlatform()->UpdateFontList();
    *_retval = PR_FALSE; 
    return NS_OK;
}
