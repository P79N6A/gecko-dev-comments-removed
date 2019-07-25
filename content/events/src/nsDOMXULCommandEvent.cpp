





































#include "nsDOMXULCommandEvent.h"
#include "nsContentUtils.h"

nsDOMXULCommandEvent::nsDOMXULCommandEvent(nsPresContext* aPresContext,
                                           nsInputEvent* aEvent)
  : nsDOMUIEvent(aPresContext,
                 aEvent ? aEvent : new nsInputEvent(false, 0, nsnull))
{
  if (aEvent) {
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMXULCommandEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMXULCommandEvent, nsDOMUIEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMXULCommandEvent, nsDOMUIEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMXULCommandEvent,
                                                nsDOMUIEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSourceEvent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMXULCommandEvent,
                                                  nsDOMUIEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSourceEvent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(XULCommandEvent, nsDOMXULCommandEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMXULCommandEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMXULCommandEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XULCommandEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMUIEvent)

NS_IMETHODIMP
nsDOMXULCommandEvent::GetAltKey(bool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = Event()->isAlt;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMXULCommandEvent::GetCtrlKey(bool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = Event()->isControl;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMXULCommandEvent::GetShiftKey(bool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = Event()->isShift;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMXULCommandEvent::GetMetaKey(bool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = Event()->isMeta;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMXULCommandEvent::GetSourceEvent(nsIDOMEvent** aSourceEvent)
{
  NS_ENSURE_ARG_POINTER(aSourceEvent);
  NS_IF_ADDREF(*aSourceEvent = mSourceEvent);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMXULCommandEvent::InitCommandEvent(const nsAString& aType,
                                       bool aCanBubble, bool aCancelable,
                                       nsIDOMWindow* aView,
                                       PRInt32 aDetail,
                                       bool aCtrlKey, bool aAltKey,
                                       bool aShiftKey, bool aMetaKey,
                                       nsIDOMEvent* aSourceEvent)
{
  nsresult rv = nsDOMUIEvent::InitUIEvent(aType, aCanBubble, aCancelable,
                                          aView, aDetail);
  NS_ENSURE_SUCCESS(rv, rv);

  nsInputEvent *event = Event();
  event->isControl = aCtrlKey;
  event->isAlt = aAltKey;
  event->isShift = aShiftKey;
  event->isMeta = aMetaKey;
  mSourceEvent = aSourceEvent;

  return NS_OK;
}


nsresult NS_NewDOMXULCommandEvent(nsIDOMEvent** aInstancePtrResult,
                                  nsPresContext* aPresContext,
                                  nsInputEvent *aEvent) 
{
  nsDOMXULCommandEvent* it = new nsDOMXULCommandEvent(aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
