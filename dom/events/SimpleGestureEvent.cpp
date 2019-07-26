




#include "mozilla/dom/SimpleGestureEvent.h"
#include "mozilla/TouchEvents.h"
#include "prtime.h"

namespace mozilla {
namespace dom {

SimpleGestureEvent::SimpleGestureEvent(EventTarget* aOwner,
                                       nsPresContext* aPresContext,
                                       WidgetSimpleGestureEvent* aEvent)
  : nsDOMMouseEvent(aOwner, aPresContext,
                    aEvent ? aEvent :
                             new WidgetSimpleGestureEvent(false, 0, nullptr))
{
  NS_ASSERTION(mEvent->eventStructType == NS_SIMPLE_GESTURE_EVENT, "event type mismatch");

  if (aEvent) {
    mEventIsInternal = false;
  } else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
    mEvent->refPoint.x = mEvent->refPoint.y = 0;
    static_cast<WidgetMouseEventBase*>(mEvent)->inputSource =
      nsIDOMMouseEvent::MOZ_SOURCE_UNKNOWN;
  }
}

NS_IMPL_ADDREF_INHERITED(SimpleGestureEvent, nsDOMUIEvent)
NS_IMPL_RELEASE_INHERITED(SimpleGestureEvent, nsDOMUIEvent)

NS_INTERFACE_MAP_BEGIN(SimpleGestureEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSimpleGestureEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMMouseEvent)


uint32_t
SimpleGestureEvent::AllowedDirections()
{
  return mEvent->AsSimpleGestureEvent()->allowedDirections;
}

NS_IMETHODIMP
SimpleGestureEvent::GetAllowedDirections(uint32_t* aAllowedDirections)
{
  NS_ENSURE_ARG_POINTER(aAllowedDirections);
  *aAllowedDirections = AllowedDirections();
  return NS_OK;
}

NS_IMETHODIMP
SimpleGestureEvent::SetAllowedDirections(uint32_t aAllowedDirections)
{
  mEvent->AsSimpleGestureEvent()->allowedDirections = aAllowedDirections;
  return NS_OK;
}


uint32_t
SimpleGestureEvent::Direction()
{
  return mEvent->AsSimpleGestureEvent()->direction;
}

NS_IMETHODIMP
SimpleGestureEvent::GetDirection(uint32_t* aDirection)
{
  NS_ENSURE_ARG_POINTER(aDirection);
  *aDirection = Direction();
  return NS_OK;
}


double
SimpleGestureEvent::Delta()
{
  return mEvent->AsSimpleGestureEvent()->delta;
}

NS_IMETHODIMP
SimpleGestureEvent::GetDelta(double* aDelta)
{
  NS_ENSURE_ARG_POINTER(aDelta);
  *aDelta = Delta();
  return NS_OK;
}


uint32_t
SimpleGestureEvent::ClickCount()
{
  return mEvent->AsSimpleGestureEvent()->clickCount;
}

NS_IMETHODIMP
SimpleGestureEvent::GetClickCount(uint32_t* aClickCount)
{
  NS_ENSURE_ARG_POINTER(aClickCount);
  *aClickCount = ClickCount();
  return NS_OK;
}

NS_IMETHODIMP
SimpleGestureEvent::InitSimpleGestureEvent(const nsAString& aTypeArg,
                                           bool aCanBubbleArg,
                                           bool aCancelableArg,
                                           nsIDOMWindow* aViewArg,
                                           int32_t aDetailArg,
                                           int32_t aScreenX, 
                                           int32_t aScreenY,
                                           int32_t aClientX,
                                           int32_t aClientY,
                                           bool aCtrlKeyArg,
                                           bool aAltKeyArg,
                                           bool aShiftKeyArg,
                                           bool aMetaKeyArg,
                                           uint16_t aButton,
                                           nsIDOMEventTarget* aRelatedTarget,
                                           uint32_t aAllowedDirectionsArg,
                                           uint32_t aDirectionArg,
                                           double aDeltaArg,
                                           uint32_t aClickCountArg)
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

  WidgetSimpleGestureEvent* simpleGestureEvent = mEvent->AsSimpleGestureEvent();
  simpleGestureEvent->allowedDirections = aAllowedDirectionsArg;
  simpleGestureEvent->direction = aDirectionArg;
  simpleGestureEvent->delta = aDeltaArg;
  simpleGestureEvent->clickCount = aClickCountArg;

  return NS_OK;
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMSimpleGestureEvent(nsIDOMEvent** aInstancePtrResult,
                            EventTarget* aOwner,
                            nsPresContext* aPresContext,
                            WidgetSimpleGestureEvent* aEvent)
{
  SimpleGestureEvent* it = new SimpleGestureEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
