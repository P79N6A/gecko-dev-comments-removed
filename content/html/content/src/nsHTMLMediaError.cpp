




































#include "nsHTMLMediaError.h"
#include "nsContentUtils.h"

NS_IMPL_ADDREF(nsHTMLMediaError)
NS_IMPL_RELEASE(nsHTMLMediaError)

NS_INTERFACE_MAP_BEGIN(nsHTMLMediaError)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLMediaError)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HTMLMediaError)
NS_INTERFACE_MAP_END

nsHTMLMediaError::nsHTMLMediaError(PRUint16 aCode) :
  mCode(aCode)
{
}

NS_IMETHODIMP nsHTMLMediaError::GetCode(PRUint16* aCode)
{
  if (aCode)
    *aCode = mCode;

  return NS_OK;
}
