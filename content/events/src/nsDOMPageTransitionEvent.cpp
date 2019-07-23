





































#include "nsDOMPageTransitionEvent.h"
#include "nsContentUtils.h"

NS_INTERFACE_MAP_BEGIN(nsDOMPageTransitionEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPageTransitionEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(PageTransitionEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMPageTransitionEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMPageTransitionEvent, nsDOMEvent)

NS_IMETHODIMP
nsDOMPageTransitionEvent::GetPersisted(PRBool* aPersisted)
{
  *aPersisted = mPersisted;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMPageTransitionEvent::InitPageTransitionEvent(const nsAString &aTypeArg,
                                                  PRBool aCanBubbleArg,
                                                  PRBool aCancelableArg,
                                                  PRBool aPersisted)
{
  nsresult rv = nsDOMEvent::InitEvent(aTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  mPersisted = aPersisted;
  return NS_OK;
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
