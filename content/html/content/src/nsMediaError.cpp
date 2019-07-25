




#include "nsMediaError.h"
#include "nsDOMClassInfoID.h"

NS_IMPL_ADDREF(nsMediaError)
NS_IMPL_RELEASE(nsMediaError)

DOMCI_DATA(MediaError, nsMediaError)

NS_INTERFACE_MAP_BEGIN(nsMediaError)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMediaError)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MediaError)
NS_INTERFACE_MAP_END

nsMediaError::nsMediaError(uint16_t aCode) :
  mCode(aCode)
{
}

NS_IMETHODIMP nsMediaError::GetCode(uint16_t* aCode)
{
  if (aCode)
    *aCode = mCode;

  return NS_OK;
}
