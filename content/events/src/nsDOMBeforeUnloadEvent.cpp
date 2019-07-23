





































#include "nsDOMBeforeUnloadEvent.h"
#include "nsContentUtils.h"

nsDOMBeforeUnloadEvent::nsDOMBeforeUnloadEvent(nsPresContext* aPresContext,
                                               nsBeforePageUnloadEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent ? aEvent :
               new nsBeforePageUnloadEvent(PR_FALSE,
                                           NS_BEFORE_PAGE_UNLOAD_EVENT))
{
  NS_ASSERTION(mEvent->eventStructType == NS_BEFORE_PAGE_UNLOAD_EVENT,
               "event type mismatch");

  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  }
  else {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
  }
}

nsDOMBeforeUnloadEvent::~nsDOMBeforeUnloadEvent() 
{
}

NS_IMPL_ADDREF_INHERITED(nsDOMBeforeUnloadEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMBeforeUnloadEvent, nsDOMEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMBeforeUnloadEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBeforeUnloadEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(BeforeUnloadEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP
nsDOMBeforeUnloadEvent::SetReturnValue(const nsAString& aReturnValue)
{
  ((nsBeforePageUnloadEvent *)mEvent)->text = aReturnValue;

  return NS_OK;  
}

NS_IMETHODIMP
nsDOMBeforeUnloadEvent::GetReturnValue(nsAString& aReturnValue)
{
  aReturnValue = ((nsBeforePageUnloadEvent *)mEvent)->text;

  return NS_OK;  
}

nsresult NS_NewDOMBeforeUnloadEvent(nsIDOMEvent** aInstancePtrResult,
                                    nsPresContext* aPresContext,
                                    nsBeforePageUnloadEvent *aEvent) 
{
  nsDOMBeforeUnloadEvent* it =
    new nsDOMBeforeUnloadEvent(aPresContext, aEvent);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
