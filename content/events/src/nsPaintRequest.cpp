




#include "nsPaintRequest.h"

#include "nsIFrame.h"
#include "nsContentUtils.h"
#include "mozilla/dom/PaintRequestBinding.h"
#include "mozilla/dom/PaintRequestListBinding.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(nsPaintRequest, mParent)

NS_INTERFACE_TABLE_HEAD(nsPaintRequest)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_TABLE1(nsPaintRequest, nsIDOMPaintRequest)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsPaintRequest)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsPaintRequest)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsPaintRequest)

 JSObject*
nsPaintRequest::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return PaintRequestBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

already_AddRefed<nsClientRect>
nsPaintRequest::ClientRect()
{
  nsRefPtr<nsClientRect> clientRect = new nsClientRect();
  clientRect->SetLayoutRect(mRequest.mRect);
  return clientRect.forget();
}

NS_IMETHODIMP
nsPaintRequest::GetClientRect(nsIDOMClientRect** aResult)
{
  nsRefPtr<nsClientRect> clientRect = ClientRect();
  clientRect.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsPaintRequest::GetXPCOMReason(nsAString& aResult)
{
  GetReason(aResult);
  return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(nsPaintRequestList, mParent)

NS_INTERFACE_TABLE_HEAD(nsPaintRequestList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_TABLE1(nsPaintRequestList, nsIDOMPaintRequestList)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsPaintRequestList)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsPaintRequestList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsPaintRequestList)

JSObject*
nsPaintRequestList::WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap)
{
  return mozilla::dom::PaintRequestListBinding::Wrap(cx, scope, this,
                                                     triedToWrap);
}

NS_IMETHODIMP    
nsPaintRequestList::GetLength(uint32_t* aLength)
{
  *aLength = Length();
  return NS_OK;
}

NS_IMETHODIMP    
nsPaintRequestList::Item(uint32_t aIndex, nsIDOMPaintRequest** aReturn)
{
  NS_IF_ADDREF(*aReturn = Item(aIndex));
  return NS_OK;
}
