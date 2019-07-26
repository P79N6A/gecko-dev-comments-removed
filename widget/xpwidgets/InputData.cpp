




#include "InputData.h"

#include "nsGUIEvent.h"
#include "mozilla/dom/Touch.h"
#include "nsDebug.h"
#include "nsThreadUtils.h"

namespace mozilla {

using namespace dom;

MultiTouchInput::MultiTouchInput(const nsTouchEvent& aTouchEvent)
  : InputData(MULTITOUCH_INPUT, aTouchEvent.time)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(),
                    "Can only copy from nsTouchEvent on main thread");

  switch (aTouchEvent.message) {
    case NS_TOUCH_START:
      mType = MULTITOUCH_START;
      break;
    case NS_TOUCH_MOVE:
      mType = MULTITOUCH_MOVE;
      break;
    case NS_TOUCH_END:
      mType = MULTITOUCH_END;
      break;
    case NS_TOUCH_ENTER:
      mType = MULTITOUCH_ENTER;
      break;
    case NS_TOUCH_LEAVE:
      mType = MULTITOUCH_LEAVE;
      break;
    case NS_TOUCH_CANCEL:
      mType = MULTITOUCH_CANCEL;
      break;
    default:
      NS_WARNING("Did not assign a type to a MultiTouchInput");
      break;
  }

  for (size_t i = 0; i < aTouchEvent.touches.Length(); i++) {
    const Touch* domTouch = aTouchEvent.touches[i];

    
    int32_t identifier = domTouch->Identifier();
    int32_t radiusX = domTouch->RadiusX();
    int32_t radiusY = domTouch->RadiusY();
    float rotationAngle = domTouch->RotationAngle();
    float force = domTouch->Force();

    SingleTouchData data(identifier,
                         ScreenIntPoint::FromUnknownPoint(
                           gfx::IntPoint(domTouch->mRefPoint.x,
                                         domTouch->mRefPoint.y)),
                         ScreenSize(radiusX, radiusY),
                         rotationAngle,
                         force);

    mTouches.AppendElement(data);
  }
}







MultiTouchInput::MultiTouchInput(const nsMouseEvent& aMouseEvent)
  : InputData(MULTITOUCH_INPUT, aMouseEvent.time)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(),
                    "Can only copy from nsMouseEvent on main thread");
  switch (aMouseEvent.message) {
  case NS_MOUSE_BUTTON_DOWN:
    mType = MULTITOUCH_START;
    break;
  case NS_MOUSE_MOVE:
    mType = MULTITOUCH_MOVE;
    break;
  case NS_MOUSE_BUTTON_UP:
    mType = MULTITOUCH_END;
    break;
  
  
  
  
  case NS_MOUSE_EXIT:
    mType = MULTITOUCH_CANCEL;
    break;
  default:
    NS_WARNING("Did not assign a type to a MultiTouchInput");
    break;
  }

  mTouches.AppendElement(SingleTouchData(0,
                                         ScreenIntPoint::FromUnknownPoint(
                                           gfx::IntPoint(aMouseEvent.refPoint.x,
                                                         aMouseEvent.refPoint.y)),
                                         ScreenSize(1, 1),
                                         180.0f,
                                         1.0f));
}
}
