






































#include "nsAtomService.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsAtomService, nsIAtomService)

nsAtomService::nsAtomService()
{
}

nsresult
nsAtomService::GetAtom(const PRUnichar *aString, nsIAtom ** aResult)
{
  *aResult = NS_NewAtom(aString);

  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  
  return NS_OK;
}

nsresult
nsAtomService::GetPermanentAtom(const PRUnichar *aString, nsIAtom ** aResult)
{
  *aResult = NS_NewPermanentAtom(aString);

  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  
  return NS_OK;
}

NS_IMETHODIMP
nsAtomService::GetAtomUTF8(const char *aValue, nsIAtom* *aResult)
{
    *aResult = NS_NewAtom(aValue);

    if (!*aResult)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

NS_IMETHODIMP
nsAtomService::GetPermanentAtomUTF8(const char *aValue, nsIAtom* *aResult)
{
    *aResult = NS_NewPermanentAtom(aValue);

    if (!*aResult)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}
