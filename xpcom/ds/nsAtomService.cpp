




#include "nsAtomService.h"

NS_IMPL_ISUPPORTS1(nsAtomService, nsIAtomService)

nsAtomService::nsAtomService()
{
}

nsresult
nsAtomService::GetAtom(const nsAString& aString, nsIAtom ** aResult)
{
  *aResult = NS_NewAtom(aString).get();

  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  
  return NS_OK;
}

nsresult
nsAtomService::GetPermanentAtom(const nsAString& aString, nsIAtom ** aResult)
{
  *aResult = NS_NewPermanentAtom(aString);

  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  
  return NS_OK;
}

NS_IMETHODIMP
nsAtomService::GetAtomUTF8(const char *aValue, nsIAtom* *aResult)
{
    *aResult = NS_NewAtom(aValue).get();

    if (!*aResult)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

NS_IMETHODIMP
nsAtomService::GetPermanentAtomUTF8(const char *aValue, nsIAtom* *aResult)
{
    *aResult = NS_NewPermanentAtom(NS_ConvertUTF8toUTF16(aValue));

    if (!*aResult)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}
