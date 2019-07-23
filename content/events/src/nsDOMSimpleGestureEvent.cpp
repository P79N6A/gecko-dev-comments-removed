




































#include "nsDOMSimpleGestureEvent.h"
#include "nsGUIEvent.h"
#include "nsContentUtils.h"


nsDOMSimpleGestureEvent::nsDOMSimpleGestureEvent(nsPresContext* aPresContext, nsSimpleGestureEvent* aEvent)
  : nsDOMUIEvent(aPresContext, aEvent ? aEvent : new nsSimpleGestureEvent(PR_FALSE, 0, nsnull, 0, 0.0))
{
  NS_ASSERTION(mEvent->eventStructType == NS_SIMPLE_GESTURE_EVENT, "event type mismatch");

  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  } else {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
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

NS_INTERFACE_MAP_BEGIN(nsDOMSimpleGestureEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSimpleGestureEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SimpleGestureEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMUIEvent)


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
nsDOMSimpleGestureEvent::GetAltKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = static_cast<nsInputEvent*>(mEvent)->isAlt;
  return NS_OK;
}


NS_IMETHODIMP
nsDOMSimpleGestureEvent::GetCtrlKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = static_cast<nsInputEvent*>(mEvent)->isControl;
  return NS_OK;
}


NS_IMETHODIMP
nsDOMSimpleGestureEvent::GetShiftKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = static_cast<nsInputEvent*>(mEvent)->isShift;
  return NS_OK;
}


NS_IMETHODIMP
nsDOMSimpleGestureEvent::GetMetaKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = static_cast<nsInputEvent*>(mEvent)->isMeta;
  return NS_OK;
}


NS_IMETHODIMP
nsDOMSimpleGestureEvent::InitSimpleGestureEvent(const nsAString & typeArg, PRBool canBubbleArg, PRBool cancelableArg, nsIDOMAbstractView *viewArg, PRInt32 detailArg, PRUint32 directionArg, PRFloat64 deltaArg, PRBool altKeyArg, PRBool ctrlKeyArg, PRBool shiftKeyArg, PRBool metaKeyArg)
{
  nsresult rv = nsDOMUIEvent::InitUIEvent(typeArg, canBubbleArg, cancelableArg, viewArg, detailArg);
  NS_ENSURE_SUCCESS(rv, rv);

  nsSimpleGestureEvent* simpleGestureEvent = static_cast<nsSimpleGestureEvent*>(mEvent);
  simpleGestureEvent->direction = directionArg;
  simpleGestureEvent->delta = deltaArg;
  simpleGestureEvent->isAlt = altKeyArg;
  simpleGestureEvent->isControl = ctrlKeyArg;
  simpleGestureEvent->isShift = shiftKeyArg;
  simpleGestureEvent->isMeta = metaKeyArg;

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
