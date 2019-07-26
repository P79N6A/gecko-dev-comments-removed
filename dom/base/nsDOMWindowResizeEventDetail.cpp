



#include "nsDOMWindowResizeEventDetail.h"
#include "nsDOMClassInfoID.h" 

DOMCI_DATA(DOMWindowResizeEventDetail, nsDOMWindowResizeEventDetail)

NS_INTERFACE_MAP_BEGIN(nsDOMWindowResizeEventDetail)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMWindowResizeEventDetail)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DOMWindowResizeEventDetail)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDOMWindowResizeEventDetail)
NS_IMPL_RELEASE(nsDOMWindowResizeEventDetail)

NS_IMETHODIMP
nsDOMWindowResizeEventDetail::GetWidth(int32_t* aOut)
{
  *aOut = mSize.width;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWindowResizeEventDetail::GetHeight(int32_t* aOut)
{
  *aOut = mSize.height;
  return NS_OK;
}
