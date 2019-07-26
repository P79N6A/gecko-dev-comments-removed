




#include "mozilla/dom/Touch.h"

#include "mozilla/dom/TouchBinding.h"
#include "mozilla/Preferences.h"
#include "nsContentUtils.h"
#include "nsDOMTouchEvent.h"
#include "nsGUIEvent.h"
#include "nsPresContext.h"

namespace mozilla {
namespace dom {

 bool
Touch::PrefEnabled()
{
  return nsDOMTouchEvent::PrefEnabled();
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(Touch, mTarget)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Touch)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(Touch)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Touch)

EventTarget*
Touch::Target() const
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mTarget);
  if (content && content->ChromeOnlyAccess() &&
      !nsContentUtils::CanAccessNativeAnon()) {
    return content->FindFirstNonChromeOnlyAccessContent();
  }

  return mTarget;
}

void
Touch::InitializePoints(nsPresContext* aPresContext, nsEvent* aEvent)
{
  if (mPointsInitialized) {
    return;
  }
  mClientPoint = nsDOMEvent::GetClientCoords(
    aPresContext, aEvent, LayoutDeviceIntPoint::FromUntyped(mRefPoint),
    mClientPoint);
  mPagePoint = nsDOMEvent::GetPageCoords(
    aPresContext, aEvent, LayoutDeviceIntPoint::FromUntyped(mRefPoint),
    mClientPoint);
  mScreenPoint = nsDOMEvent::GetScreenCoords(aPresContext, aEvent,
    LayoutDeviceIntPoint::FromUntyped(mRefPoint));
  mPointsInitialized = true;
}

bool
Touch::Equals(Touch* aTouch)
{
  return mRefPoint == aTouch->mRefPoint &&
         mForce == aTouch->Force() &&
         mRotationAngle == aTouch->RotationAngle() &&
         mRadius.x == aTouch->RadiusX() &&
         mRadius.y == aTouch->RadiusY();
}

 JSObject*
Touch::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return TouchBinding::Wrap(aCx, aScope, this);
}

} 
} 
