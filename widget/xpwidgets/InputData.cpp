




#include "InputData.h"

#include "nsGUIEvent.h"
#include "nsDOMTouchEvent.h"
#include "nsDebug.h"

namespace mozilla {

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
    nsDOMTouch* domTouch = (nsDOMTouch*)(aTouchEvent.touches[i].get());

    
    int32_t identifier, radiusX, radiusY;
    float rotationAngle, force;
    domTouch->GetIdentifier(&identifier);
    domTouch->GetRadiusX(&radiusX);
    domTouch->GetRadiusY(&radiusY);
    domTouch->GetRotationAngle(&rotationAngle);
    domTouch->GetForce(&force);

    SingleTouchData data(identifier,
                         domTouch->mRefPoint,
                         nsIntPoint(radiusX, radiusY),
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
                                         aMouseEvent.refPoint,
                                         nsIntPoint(1, 1),
                                         180.0f,
                                         1.0f));
}
}
