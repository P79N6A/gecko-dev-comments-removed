





































#include "nsDOMPageTransitionEvent.h"
#include "nsContentUtils.h"

DOMCI_DATA(PageTransitionEvent, nsDOMPageTransitionEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMPageTransitionEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPageTransitionEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(PageTransitionEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMPageTransitionEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMPageTransitionEvent, nsDOMEvent)

NS_IMETHODIMP
nsDOMPageTransitionEvent::GetPersisted(bool* aPersisted)
{
  *aPersisted = mPersisted;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMPageTransitionEvent::InitPageTransitionEvent(const nsAString &aTypeArg,
                                                  bool aCanBubbleArg,
                                                  bool aCancelableArg,
                                                  bool aPersisted)
{
  nsresult rv = nsDOMEvent::InitEvent(aTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  mPersisted = aPersisted;
  return NS_OK;
}

nsresult
nsDOMPageTransitionEvent::InitFromCtor(const nsAString& aType, nsISupports* aDict,
                                       JSContext* aCx, JSObject* aObj)
{
  nsCOMPtr<nsIPageTransitionEventInit> eventInit = do_QueryInterface(aDict);
  bool bubbles = false;
  bool cancelable = false;
  bool persisted = false;
  if (eventInit) {
    nsresult rv = eventInit->GetBubbles(&bubbles);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = eventInit->GetCancelable(&cancelable);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = eventInit->GetPersisted(&persisted);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return InitPageTransitionEvent(aType, bubbles, cancelable, persisted);
}

nsresult NS_NewDOMPageTransitionEvent(nsIDOMEvent** aInstancePtrResult,
                                      nsPresContext* aPresContext,
                                      nsEvent *aEvent) 
{
  nsDOMPageTransitionEvent* it =
    new nsDOMPageTransitionEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
