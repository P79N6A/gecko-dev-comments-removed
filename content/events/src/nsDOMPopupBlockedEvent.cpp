






































#include "nsDOMPopupBlockedEvent.h"
#include "nsIURI.h"
#include "nsContentUtils.h"

nsDOMPopupBlockedEvent::nsDOMPopupBlockedEvent(nsPresContext* aPresContext,
                                               nsPopupBlockedEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent ? aEvent :
               new nsPopupBlockedEvent(PR_FALSE, 0))
{
  NS_ASSERTION(mEvent->eventStructType == NS_POPUPBLOCKED_EVENT, "event type mismatch");

  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  }
  else {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
  }
}

nsDOMPopupBlockedEvent::~nsDOMPopupBlockedEvent() 
{
  if (mEventIsInternal) {
    if (mEvent->eventStructType == NS_POPUPBLOCKED_EVENT) {
      nsPopupBlockedEvent* event = static_cast<nsPopupBlockedEvent*>(mEvent);
      NS_IF_RELEASE(event->mPopupWindowURI);
    }
  }
}

NS_IMPL_ADDREF_INHERITED(nsDOMPopupBlockedEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMPopupBlockedEvent, nsDOMEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMPopupBlockedEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPopupBlockedEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(PopupBlockedEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP
nsDOMPopupBlockedEvent::InitPopupBlockedEvent(const nsAString & aTypeArg,
                            PRBool aCanBubbleArg, PRBool aCancelableArg,
                            nsIDOMWindow *aRequestingWindow,
                            nsIURI *aPopupWindowURI,
                            const nsAString & aPopupWindowName,
                            const nsAString & aPopupWindowFeatures)
{
  nsresult rv = nsDOMEvent::InitEvent(aTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (mEvent->eventStructType)
  {
    case NS_POPUPBLOCKED_EVENT:
    {
       nsPopupBlockedEvent* event = static_cast<nsPopupBlockedEvent*>(mEvent);
       event->mRequestingWindow = do_GetWeakReference(aRequestingWindow);
       event->mPopupWindowURI = aPopupWindowURI;
       NS_IF_ADDREF(event->mPopupWindowURI);
       event->mPopupWindowFeatures = aPopupWindowFeatures;
       event->mPopupWindowName = aPopupWindowName;
       break;
    }
    default:
       break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMPopupBlockedEvent::GetRequestingWindow(nsIDOMWindow **aRequestingWindow)
{
  if (mEvent->eventStructType == NS_POPUPBLOCKED_EVENT) {
    nsPopupBlockedEvent* event = static_cast<nsPopupBlockedEvent*>(mEvent);
    CallQueryReferent(event->mRequestingWindow.get(), aRequestingWindow);
  } else {
    *aRequestingWindow = 0;
  }

  return NS_OK;  
}

NS_IMETHODIMP
nsDOMPopupBlockedEvent::GetPopupWindowURI(nsIURI **aPopupWindowURI)
{
  NS_ENSURE_ARG_POINTER(aPopupWindowURI);
  if (mEvent->eventStructType == NS_POPUPBLOCKED_EVENT) {
    nsPopupBlockedEvent* event = static_cast<nsPopupBlockedEvent*>(mEvent);
    *aPopupWindowURI = event->mPopupWindowURI;
    NS_IF_ADDREF(*aPopupWindowURI);
    return NS_OK;
  }
  *aPopupWindowURI = 0;
  return NS_OK;  
}

NS_IMETHODIMP
nsDOMPopupBlockedEvent::GetPopupWindowFeatures(nsAString &aPopupWindowFeatures)
{
  if (mEvent->eventStructType == NS_POPUPBLOCKED_EVENT) {
    nsPopupBlockedEvent* event = static_cast<nsPopupBlockedEvent*>(mEvent);
    aPopupWindowFeatures = event->mPopupWindowFeatures;
    return NS_OK;
  }
  aPopupWindowFeatures.Truncate();
  return NS_OK;  
}

NS_IMETHODIMP
nsDOMPopupBlockedEvent::GetPopupWindowName(nsAString &aPopupWindowName)
{
  if (mEvent->eventStructType == NS_POPUPBLOCKED_EVENT) {
    nsPopupBlockedEvent* event = static_cast<nsPopupBlockedEvent*>(mEvent);
    aPopupWindowName = event->mPopupWindowName;
    return NS_OK;
  }
  aPopupWindowName.Truncate();
  return NS_OK;  
}

nsresult NS_NewDOMPopupBlockedEvent(nsIDOMEvent** aInstancePtrResult,
                                    nsPresContext* aPresContext,
                                    nsPopupBlockedEvent *aEvent) 
{
  nsDOMPopupBlockedEvent* it = new nsDOMPopupBlockedEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
