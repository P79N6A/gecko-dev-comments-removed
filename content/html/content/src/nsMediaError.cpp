




































#include "nsMediaError.h"
#include "nsContentUtils.h"

NS_IMPL_ADDREF(nsMediaError)
NS_IMPL_RELEASE(nsMediaError)

DOMCI_DATA(MediaError, nsMediaError)

NS_INTERFACE_MAP_BEGIN(nsMediaError)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMediaError)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MediaError)
NS_INTERFACE_MAP_END

nsMediaError::nsMediaError(PRUint16 aCode) :
  mCode(aCode)
{
}

NS_IMETHODIMP nsMediaError::GetCode(PRUint16* aCode)
{
  if (aCode)
    *aCode = mCode;

  return NS_OK;
}
