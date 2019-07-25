




































#include "nsDOMCustomEvent.h"
#include "nsContentUtils.h"

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMCustomEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMCustomEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDetail)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMCustomEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDetail)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(CustomEvent, nsDOMCustomEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMCustomEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCustomEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CustomEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMCustomEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMCustomEvent, nsDOMEvent)

NS_IMETHODIMP
nsDOMCustomEvent::GetDetail(nsIVariant** aDetail)
{
  NS_IF_ADDREF(*aDetail = mDetail);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMCustomEvent::InitCustomEvent(const nsAString& aType,
                                  PRBool aCanBubble,
                                  PRBool aCancelable,
                                  nsIVariant* aDetail)
{
  nsresult rv = nsDOMEvent::InitEvent(aType, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);

  mDetail = aDetail;
  return NS_OK;
}

nsresult
NS_NewDOMCustomEvent(nsIDOMEvent** aInstancePtrResult,
                     nsPresContext* aPresContext,
                     nsEvent* aEvent) 
{
  nsDOMCustomEvent* e = new nsDOMCustomEvent(aPresContext, aEvent);
  return CallQueryInterface(e, aInstancePtrResult);
}
