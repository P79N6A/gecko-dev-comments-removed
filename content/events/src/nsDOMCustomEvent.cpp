




































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
                                  bool aCanBubble,
                                  bool aCancelable,
                                  nsIVariant* aDetail)
{
  nsresult rv = nsDOMEvent::InitEvent(aType, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);

  mDetail = aDetail;
  return NS_OK;
}

nsresult
nsDOMCustomEvent::InitFromCtor(const nsAString& aType, nsISupports* aDict)
{
  nsCOMPtr<nsICustomEventInit> eventInit = do_QueryInterface(aDict);
  bool bubbles = false;
  bool cancelable = false;
  nsCOMPtr<nsIVariant> detail;
  if (eventInit) {
    nsresult rv = eventInit->GetBubbles(&bubbles);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = eventInit->GetCancelable(&cancelable);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = eventInit->GetDetail(getter_AddRefs(detail));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return InitCustomEvent(aType, bubbles, cancelable, detail);
}

nsresult
NS_NewDOMCustomEvent(nsIDOMEvent** aInstancePtrResult,
                     nsPresContext* aPresContext,
                     nsEvent* aEvent) 
{
  nsDOMCustomEvent* e = new nsDOMCustomEvent(aPresContext, aEvent);
  return CallQueryInterface(e, aInstancePtrResult);
}
