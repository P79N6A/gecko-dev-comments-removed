





#include "mozilla/dom/MediaError.h"
#include "nsDOMClassInfoID.h"

DOMCI_DATA(MediaError, mozilla::dom::MediaError)

namespace mozilla {
namespace dom {

NS_IMPL_ADDREF(MediaError)
NS_IMPL_RELEASE(MediaError)

NS_INTERFACE_MAP_BEGIN(MediaError)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMediaError)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MediaError)
NS_INTERFACE_MAP_END

MediaError::MediaError(uint16_t aCode)
: mCode(aCode)
{
}

NS_IMETHODIMP MediaError::GetCode(uint16_t* aCode)
{
  if (aCode)
    *aCode = mCode;

  return NS_OK;
}

} 
} 
