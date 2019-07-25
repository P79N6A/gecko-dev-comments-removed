






































#include "nsDOMCompositionEvent.h"
#include "nsDOMClassInfo.h"

nsDOMCompositionEvent::nsDOMCompositionEvent(nsPresContext* aPresContext,
                                             nsCompositionEvent* aEvent)
  : nsDOMUIEvent(aPresContext, aEvent ? aEvent :
                 new nsCompositionEvent(PR_FALSE, 0, nsnull))
{
  NS_ASSERTION(mEvent->eventStructType == NS_COMPOSITION_EVENT,
               "event type mismatch");

  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  } else {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();

    
    
    
    mEvent->flags |= NS_EVENT_FLAG_CANT_CANCEL;
  }

  mData = static_cast<nsCompositionEvent*>(mEvent)->data;
  
}

nsDOMCompositionEvent::~nsDOMCompositionEvent()
{
  if (mEventIsInternal) {
    delete static_cast<nsCompositionEvent*>(mEvent);
    mEvent = nsnull;
  }
}

NS_IMPL_ADDREF_INHERITED(nsDOMCompositionEvent, nsDOMUIEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMCompositionEvent, nsDOMUIEvent)

DOMCI_DATA(CompositionEvent, nsDOMCompositionEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMCompositionEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCompositionEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CompositionEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMUIEvent)

NS_IMETHODIMP
nsDOMCompositionEvent::GetData(nsAString& aData)
{
  aData = mData;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMCompositionEvent::GetLocale(nsAString& aLocale)
{
  aLocale = mLocale;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMCompositionEvent::InitCompositionEvent(const nsAString& aType,
                                            bool aCanBubble,
                                            bool aCancelable,
                                            nsIDOMWindow* aView,
                                            const nsAString& aData,
                                            const nsAString& aLocale)
{
  nsresult rv =
    nsDOMUIEvent::InitUIEvent(aType, aCanBubble, aCancelable, aView, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  mData = aData;
  mLocale = aLocale;
  return NS_OK;
}

nsresult
NS_NewDOMCompositionEvent(nsIDOMEvent** aInstancePtrResult,
                          nsPresContext* aPresContext,
                          nsCompositionEvent *aEvent)
{
  nsDOMCompositionEvent* event =
    new nsDOMCompositionEvent(aPresContext, aEvent);
  return CallQueryInterface(event, aInstancePtrResult);
}
