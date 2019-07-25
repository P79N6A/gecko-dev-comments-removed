




































#include "nsDOMSimpleGestureEvent.h"
#include "nsGUIEvent.h"
#include "nsContentUtils.h"


nsDOMSimpleGestureEvent::nsDOMSimpleGestureEvent(nsPresContext* aPresContext, nsSimpleGestureEvent* aEvent)
  : nsDOMMouseEvent(aPresContext, aEvent ? aEvent : new nsSimpleGestureEvent(PR_FALSE, 0, nsnull, 0, 0.0))
{
  NS_ASSERTION(mEvent->eventStructType == NS_SIMPLE_GESTURE_EVENT, "event type mismatch");

  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  } else {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
    mEvent->refPoint.x = mEvent->refPoint.y = 0;
    static_cast<nsMouseEvent*>(mEvent)->inputSource = nsIDOMNSMouseEvent::MOZ_SOURCE_UNKNOWN;
  }
}

nsDOMSimpleGestureEvent::~nsDOMSimpleGestureEvent()
{
  if (mEventIsInternal) {
    delete static_cast<nsSimpleGestureEvent*>(mEvent);
    mEvent = nsnull;
  }
}

NS_IMPL_ADDREF_INHERITED(nsDOMSimpleGestureEvent, nsDOMUIEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMSimpleGestureEvent, nsDOMUIEvent)

DOMCI_DATA(SimpleGestureEvent, nsDOMSimpleGestureEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMSimpleGestureEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSimpleGestureEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SimpleGestureEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMMouseEvent)


NS_IMETHODIMP
nsDOMSimpleGestureEvent::GetDirection(PRUint32 *aDirection)
{
  NS_ENSURE_ARG_POINTER(aDirection);
  *aDirection = static_cast<nsSimpleGestureEvent*>(mEvent)->direction;
  return NS_OK;
}


NS_IMETHODIMP
nsDOMSimpleGestureEvent::GetDelta(PRFloat64 *aDelta)
{
  NS_ENSURE_ARG_POINTER(aDelta);
  *aDelta = static_cast<nsSimpleGestureEvent*>(mEvent)->delta;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMSimpleGestureEvent::InitSimpleGestureEvent(const nsAString& aTypeArg,
                                                PRBool aCanBubbleArg,
                                                PRBool aCancelableArg,
                                                nsIDOMAbstractView* aViewArg,
                                                PRInt32 aDetailArg,
                                                PRInt32 aScreenX, 
                                                PRInt32 aScreenY,
                                                PRInt32 aClientX,
                                                PRInt32 aClientY,
                                                PRBool aCtrlKeyArg,
                                                PRBool aAltKeyArg,
                                                PRBool aShiftKeyArg,
                                                PRBool aMetaKeyArg,
                                                PRUint16 aButton,
                                                nsIDOMEventTarget* aRelatedTarget,
                                                PRUint32 aDirectionArg,
                                                PRFloat64 aDeltaArg)
{
  nsresult rv = nsDOMMouseEvent::InitMouseEvent(aTypeArg,
                                                aCanBubbleArg,
                                                aCancelableArg,
                                                aViewArg,
                                                aDetailArg,
                                                aScreenX, 
                                                aScreenY,
                                                aClientX,
                                                aClientY,
                                                aCtrlKeyArg,
                                                aAltKeyArg,
                                                aShiftKeyArg,
                                                aMetaKeyArg,
                                                aButton,
                                                aRelatedTarget);
  NS_ENSURE_SUCCESS(rv, rv);

  nsSimpleGestureEvent* simpleGestureEvent = static_cast<nsSimpleGestureEvent*>(mEvent);
  simpleGestureEvent->direction = aDirectionArg;
  simpleGestureEvent->delta = aDeltaArg;

  return NS_OK;
}

nsresult NS_NewDOMSimpleGestureEvent(nsIDOMEvent** aInstancePtrResult,
                                     nsPresContext* aPresContext,
                                     nsSimpleGestureEvent *aEvent)
{
  nsDOMSimpleGestureEvent *it = new nsDOMSimpleGestureEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return CallQueryInterface(it, aInstancePtrResult);
}
