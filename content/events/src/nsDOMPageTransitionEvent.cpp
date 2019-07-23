





































#include "nsDOMPageTransitionEvent.h"
#include "nsContentUtils.h"

nsDOMPageTransitionEvent::nsDOMPageTransitionEvent(nsPresContext* aPresContext,
                                                   nsPageTransitionEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent ? aEvent :
               new nsPageTransitionEvent(PR_FALSE, 0, PR_FALSE))
{  
  if ( aEvent ) {
    mEventIsInternal = PR_FALSE;
  }
  else
  {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
  }
}

NS_INTERFACE_MAP_BEGIN(nsDOMPageTransitionEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPageTransitionEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(PageTransitionEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMPageTransitionEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMPageTransitionEvent, nsDOMEvent)

NS_IMETHODIMP
nsDOMPageTransitionEvent::GetPersisted(PRBool* aPersisted)
{
  *aPersisted = static_cast<nsPageTransitionEvent*>(mEvent)->persisted;
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

  static_cast<nsPageTransitionEvent*>(mEvent)->persisted = aPersisted;
  return NS_OK;
}

nsresult NS_NewDOMPageTransitionEvent(nsIDOMEvent** aInstancePtrResult,
                                      nsPresContext* aPresContext,
                                      nsPageTransitionEvent *aEvent) 
{
  nsDOMPageTransitionEvent* it =
    new nsDOMPageTransitionEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
