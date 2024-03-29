





#include "mozilla/dom/WheelEvent.h"
#include "mozilla/MouseEvents.h"
#include "prtime.h"

namespace mozilla {
namespace dom {

WheelEvent::WheelEvent(EventTarget* aOwner,
                       nsPresContext* aPresContext,
                       WidgetWheelEvent* aWheelEvent)
  : MouseEvent(aOwner, aPresContext,
               aWheelEvent ? aWheelEvent :
                             new WidgetWheelEvent(false, 0, nullptr))
  , mAppUnitsPerDevPixel(0)
{
  if (aWheelEvent) {
    mEventIsInternal = false;
    
    
    
    
    if (aWheelEvent->deltaMode == nsIDOMWheelEvent::DOM_DELTA_PIXEL) {
      mAppUnitsPerDevPixel = aPresContext->AppUnitsPerDevPixel();
    }
  } else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
    mEvent->refPoint.x = mEvent->refPoint.y = 0;
    mEvent->AsWheelEvent()->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_UNKNOWN;
  }
}

NS_IMPL_ADDREF_INHERITED(WheelEvent, MouseEvent)
NS_IMPL_RELEASE_INHERITED(WheelEvent, MouseEvent)

NS_INTERFACE_MAP_BEGIN(WheelEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMWheelEvent)
NS_INTERFACE_MAP_END_INHERITING(MouseEvent)

NS_IMETHODIMP
WheelEvent::InitWheelEvent(const nsAString& aType,
                           bool aCanBubble,
                           bool aCancelable,
                           nsIDOMWindow* aView,
                           int32_t aDetail,
                           int32_t aScreenX,
                           int32_t aScreenY,
                           int32_t aClientX,
                           int32_t aClientY, 
                           uint16_t aButton,
                           nsIDOMEventTarget* aRelatedTarget,
                           const nsAString& aModifiersList,
                           double aDeltaX,
                           double aDeltaY,
                           double aDeltaZ,
                           uint32_t aDeltaMode)
{
  nsresult rv =
    MouseEvent::InitMouseEvent(aType, aCanBubble, aCancelable, aView, aDetail,
                               aScreenX, aScreenY, aClientX, aClientY, aButton,
                               aRelatedTarget, aModifiersList);
  NS_ENSURE_SUCCESS(rv, rv);

  WidgetWheelEvent* wheelEvent = mEvent->AsWheelEvent();
  wheelEvent->deltaX = aDeltaX;
  wheelEvent->deltaY = aDeltaY;
  wheelEvent->deltaZ = aDeltaZ;
  wheelEvent->deltaMode = aDeltaMode;

  return NS_OK;
}

double
WheelEvent::DeltaX()
{
  if (!mAppUnitsPerDevPixel) {
    return mEvent->AsWheelEvent()->deltaX;
  }
  return mEvent->AsWheelEvent()->deltaX *
    mAppUnitsPerDevPixel / nsPresContext::AppUnitsPerCSSPixel();
}

NS_IMETHODIMP
WheelEvent::GetDeltaX(double* aDeltaX)
{
  NS_ENSURE_ARG_POINTER(aDeltaX);

  *aDeltaX = DeltaX();
  return NS_OK;
}

double
WheelEvent::DeltaY()
{
  if (!mAppUnitsPerDevPixel) {
    return mEvent->AsWheelEvent()->deltaY;
  }
  return mEvent->AsWheelEvent()->deltaY *
    mAppUnitsPerDevPixel / nsPresContext::AppUnitsPerCSSPixel();
}

NS_IMETHODIMP
WheelEvent::GetDeltaY(double* aDeltaY)
{
  NS_ENSURE_ARG_POINTER(aDeltaY);

  *aDeltaY = DeltaY();
  return NS_OK;
}

double
WheelEvent::DeltaZ()
{
  if (!mAppUnitsPerDevPixel) {
    return mEvent->AsWheelEvent()->deltaZ;
  }
  return mEvent->AsWheelEvent()->deltaZ *
    mAppUnitsPerDevPixel / nsPresContext::AppUnitsPerCSSPixel();
}

NS_IMETHODIMP
WheelEvent::GetDeltaZ(double* aDeltaZ)
{
  NS_ENSURE_ARG_POINTER(aDeltaZ);

  *aDeltaZ = DeltaZ();
  return NS_OK;
}

uint32_t
WheelEvent::DeltaMode()
{
  return mEvent->AsWheelEvent()->deltaMode;
}

NS_IMETHODIMP
WheelEvent::GetDeltaMode(uint32_t* aDeltaMode)
{
  NS_ENSURE_ARG_POINTER(aDeltaMode);

  *aDeltaMode = DeltaMode();
  return NS_OK;
}

already_AddRefed<WheelEvent>
WheelEvent::Constructor(const GlobalObject& aGlobal,
                        const nsAString& aType,
                        const WheelEventInit& aParam,
                        ErrorResult& aRv)
{
  nsCOMPtr<EventTarget> t = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<WheelEvent> e = new WheelEvent(t, nullptr, nullptr);
  bool trusted = e->Init(t);
  aRv = e->InitWheelEvent(aType, aParam.mBubbles, aParam.mCancelable,
                          aParam.mView, aParam.mDetail,
                          aParam.mScreenX, aParam.mScreenY,
                          aParam.mClientX, aParam.mClientY,
                          aParam.mButton, aParam.mRelatedTarget,
                          EmptyString(), aParam.mDeltaX,
                          aParam.mDeltaY, aParam.mDeltaZ, aParam.mDeltaMode);
  e->InitializeExtraMouseEventDictionaryMembers(aParam);
  e->SetTrusted(trusted);
  return e.forget();
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMWheelEvent(nsIDOMEvent** aInstancePtrResult,
                    EventTarget* aOwner,
                    nsPresContext* aPresContext,
                    WidgetWheelEvent* aEvent)
{
  WheelEvent* it = new WheelEvent(aOwner, aPresContext, aEvent);
  NS_ADDREF(it);
  *aInstancePtrResult = static_cast<Event*>(it);
  return NS_OK;
}
