






#include "PointerEvent.h"
#include "mozilla/MouseEvents.h"
#include "prtime.h"

namespace mozilla {
namespace dom {

PointerEvent::PointerEvent(EventTarget* aOwner,
                           nsPresContext* aPresContext,
                           WidgetPointerEvent* aEvent)
  : nsDOMMouseEvent(aOwner, aPresContext, aEvent ? aEvent : new WidgetPointerEvent(false, 0, nullptr))
{
  NS_ASSERTION(mEvent->eventStructType == NS_POINTER_EVENT, "event type mismatch NS_POINTER_EVENT");

  WidgetMouseEvent* mouseEvent = mEvent->AsMouseEvent();
  if (aEvent) {
    mEventIsInternal = false;
  } else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
    mEvent->refPoint.x = mEvent->refPoint.y = 0;
    mouseEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_UNKNOWN;
  }
}

static uint16_t
ConvertStringToPointerType(const nsAString& aPointerTypeArg)
{
  if (aPointerTypeArg.EqualsLiteral("mouse")) {
    return nsIDOMMouseEvent::MOZ_SOURCE_MOUSE;
  }
  if (aPointerTypeArg.EqualsLiteral("pen")) {
    return nsIDOMMouseEvent::MOZ_SOURCE_PEN;
  }
  if (aPointerTypeArg.EqualsLiteral("touch")) {
    return nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  }

  return nsIDOMMouseEvent::MOZ_SOURCE_UNKNOWN;
}


already_AddRefed<PointerEvent>
PointerEvent::Constructor(const mozilla::dom::GlobalObject& aGlobal,
                          const nsAString& aType,
                          const mozilla::dom::PointerEventInit& aParam,
                          mozilla::ErrorResult& aRv)
{
  nsCOMPtr<mozilla::dom::EventTarget> t = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<PointerEvent> e = new PointerEvent(t, nullptr, nullptr);
  bool trusted = e->Init(t);

  aRv = e->InitMouseEvent(aType, aParam.mBubbles, aParam.mCancelable,
                          aParam.mView, aParam.mDetail, aParam.mScreenX,
                          aParam.mScreenY, aParam.mClientX, aParam.mClientY,
                          aParam.mCtrlKey, aParam.mAltKey, aParam.mShiftKey,
                          aParam.mMetaKey, aParam.mButton, aParam.mRelatedTarget);
  if (aRv.Failed()) {
    return nullptr;
  }

  WidgetPointerEvent* widgetEvent = e->mEvent->AsPointerEvent();
  widgetEvent->pointerId = aParam.mPointerId;
  widgetEvent->width = aParam.mWidth;
  widgetEvent->height = aParam.mHeight;
  widgetEvent->pressure = aParam.mPressure;
  widgetEvent->tiltX = aParam.mTiltX;
  widgetEvent->tiltY = aParam.mTiltY;
  widgetEvent->inputSource = ConvertStringToPointerType(aParam.mPointerType);
  widgetEvent->isPrimary = aParam.mIsPrimary;
  widgetEvent->buttons = aParam.mButtons;

  e->SetTrusted(trusted);
  return e.forget();
}

void
PointerEvent::GetPointerType(nsAString& aPointerType)
{
  switch (mEvent->AsPointerEvent()->inputSource) {
    case nsIDOMMouseEvent::MOZ_SOURCE_MOUSE:
      aPointerType.AssignLiteral("mouse");
      break;
    case nsIDOMMouseEvent::MOZ_SOURCE_PEN:
      aPointerType.AssignLiteral("pen");
      break;
    case nsIDOMMouseEvent::MOZ_SOURCE_TOUCH:
      aPointerType.AssignLiteral("touch");
      break;
    case nsIDOMMouseEvent::MOZ_SOURCE_UNKNOWN:
      aPointerType.AssignLiteral("");
      break;
  }
}

int32_t PointerEvent::PointerId()
{
  return mEvent->AsPointerEvent()->pointerId;
}

int32_t PointerEvent::Width()
{
  return mEvent->AsPointerEvent()->width;
}

int32_t PointerEvent::Height()
{
  return mEvent->AsPointerEvent()->height;
}

int32_t PointerEvent::Pressure()
{
  return mEvent->AsPointerEvent()->pressure;
}

int32_t PointerEvent::TiltX()
{
  return mEvent->AsPointerEvent()->tiltX;
}

int32_t PointerEvent::TiltY()
{
  return mEvent->AsPointerEvent()->tiltY;
}

bool PointerEvent::IsPrimary()
{
  return mEvent->AsPointerEvent()->isPrimary;
}

} 
} 

using namespace mozilla;

nsresult NS_NewDOMPointerEvent(nsIDOMEvent** aInstancePtrResult,
                               dom::EventTarget* aOwner,
                               nsPresContext* aPresContext,
                               WidgetPointerEvent *aEvent)
{
  dom::PointerEvent *it = new dom::PointerEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
