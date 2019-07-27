




#include "nsThebesFontEnumerator.h"
#include <stdint.h>                     
#include "gfxPlatform.h"                
#include "mozilla/Assertions.h"         
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsError.h"                    
#include "nsIAtom.h"                    
#include "nsID.h"
#include "nsMemory.h"                   
#include "nsString.h"               
#include "nsTArray.h"                   
#include "nscore.h"                     

NS_IMPL_ISUPPORTS(nsThebesFontEnumerator, nsIFontEnumerator)

nsThebesFontEnumerator::nsThebesFontEnumerator()
{
}

NS_IMETHODIMP
nsThebesFontEnumerator::EnumerateAllFonts(uint32_t *aCount,
                                          char16_t ***aResult)
{
    return EnumerateFonts (nullptr, nullptr, aCount, aResult);
}

NS_IMETHODIMP
nsThebesFontEnumerator::EnumerateFonts(const char *aLangGroup,
                                       const char *aGeneric,
                                       uint32_t *aCount,
                                       char16_t ***aResult)
{
    NS_ENSURE_ARG_POINTER(aCount);
    NS_ENSURE_ARG_POINTER(aResult);

    nsTArray<nsString> fontList;

    nsAutoCString generic;
    if (aGeneric)
        generic.Assign(aGeneric);
    else
        generic.SetIsVoid(true);

    nsCOMPtr<nsIAtom> langGroupAtom;
    if (aLangGroup) {
        nsAutoCString lowered;
        lowered.Assign(aLangGroup);
        ToLowerCase(lowered);
        langGroupAtom = do_GetAtom(lowered);
    }

    nsresult rv = gfxPlatform::GetPlatform()->GetFontList(langGroupAtom, generic, fontList);

    if (NS_FAILED(rv)) {
        *aCount = 0;
        *aResult = nullptr;
        
        return NS_OK;
    }

    char16_t **fs = static_cast<char16_t **>
                                (moz_xmalloc(fontList.Length() * sizeof(char16_t*)));
    for (uint32_t i = 0; i < fontList.Length(); i++) {
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

    *aResult = true;
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontEnumerator::GetDefaultFont(const char *aLangGroup,
                                       const char *aGeneric,
                                       char16_t **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontEnumerator::UpdateFontList(bool *_retval)
{
    gfxPlatform::GetPlatform()->UpdateFontList();
    *_retval = false; 
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontEnumerator::GetStandardFamilyName(const char16_t *aName,
                                              char16_t **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aName);

    nsAutoString name(aName);
    if (name.IsEmpty()) {
        *aResult = nullptr;
        return NS_OK;
    }

    nsAutoString family;
    nsresult rv = gfxPlatform::GetPlatform()->
        GetStandardFamilyName(nsDependentString(aName), family);
    if (NS_FAILED(rv) || family.IsEmpty()) {
        *aResult = nullptr;
        return NS_OK;
    }
    *aResult = ToNewUnicode(family);
    return NS_OK;
}
