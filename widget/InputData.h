




#ifndef InputData_h__
#define InputData_h__

#include "nsGUIEvent.h"
#include "nsDOMTouchEvent.h"
#include "nsDebug.h"

namespace mozilla {


enum InputType
{
  MULTITOUCH_INPUT,
  PINCHGESTURE_INPUT,
  TAPGESTURE_INPUT
};

class MultiTouchInput;
class PinchGestureInput;
class TapGestureInput;





#define INPUTDATA_AS_CHILD_TYPE(type, enumID) \
  const type& As##type() const \
  { \
    NS_ABORT_IF_FALSE(mInputType == enumID, "Invalid cast of InputData."); \
    return (const type&) *this; \
  }


class InputData
{
public:
  InputType mInputType;
  
  
  
  
  uint32_t mTime;

  INPUTDATA_AS_CHILD_TYPE(MultiTouchInput, MULTITOUCH_INPUT)
  INPUTDATA_AS_CHILD_TYPE(PinchGestureInput, PINCHGESTURE_INPUT)
  INPUTDATA_AS_CHILD_TYPE(TapGestureInput, TAPGESTURE_INPUT)

protected:
  InputData(InputType aInputType, uint32_t aTime)
    : mInputType(aInputType),
      mTime(aTime)
  {


  }
};
















class SingleTouchData
{
public:
  SingleTouchData(int32_t aIdentifier,
                  nsIntPoint aScreenPoint,
                  nsIntPoint aRadius,
                  float aRotationAngle,
                  float aForce)
    : mIdentifier(aIdentifier),
      mScreenPoint(aScreenPoint),
      mRadius(aRadius),
      mRotationAngle(aRotationAngle),
      mForce(aForce)
  {


  }

  
  
  int32_t mIdentifier;

  
  
  nsIntPoint mScreenPoint;

  
  
  
  
  
  nsIntPoint mRadius;

  float mRotationAngle;

  
  float mForce;
};










class MultiTouchInput : public InputData
{
public:
  enum MultiTouchType
  {
    MULTITOUCH_START,
    MULTITOUCH_MOVE,
    MULTITOUCH_END,
    MULTITOUCH_ENTER,
    MULTITOUCH_LEAVE,
    MULTITOUCH_CANCEL
  };

  MultiTouchInput(MultiTouchType aType, uint32_t aTime)
    : InputData(MULTITOUCH_INPUT, aTime),
      mType(aType)
  {


  }

  MultiTouchInput(const nsTouchEvent& aTouchEvent)
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

  
  
  
  
  
  
  MultiTouchInput(const nsMouseEvent& aMouseEvent)
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

  MultiTouchType mType;
  nsTArray<SingleTouchData> mTouches;
};






class PinchGestureInput : public InputData
{
public:
  enum PinchGestureType
  {
    PINCHGESTURE_START,
    PINCHGESTURE_SCALE,
    PINCHGESTURE_END
  };

  PinchGestureInput(PinchGestureType aType,
                    uint32_t aTime,
                    const nsIntPoint& aFocusPoint,
                    float aCurrentSpan,
                    float aPreviousSpan)
    : InputData(PINCHGESTURE_INPUT, aTime),
      mType(aType),
      mFocusPoint(aFocusPoint),
      mCurrentSpan(aCurrentSpan),
      mPreviousSpan(aPreviousSpan)
  {


  }

  PinchGestureType mType;

  
  
  
  
  
  nsIntPoint mFocusPoint;

  
  
  
  float mCurrentSpan;

  
  
  
  float mPreviousSpan;
};






class TapGestureInput : public InputData
{
public:
  enum TapGestureType
  {
    TAPGESTURE_LONG,
    TAPGESTURE_UP,
    TAPGESTURE_CONFIRMED,
    TAPGESTURE_DOUBLE,
    TAPGESTURE_CANCEL
  };

  TapGestureInput(TapGestureType aType, uint32_t aTime, const nsIntPoint& aPoint)
    : InputData(TAPGESTURE_INPUT, aTime),
      mType(aType),
      mPoint(aPoint)
  {


  }

  TapGestureType mType;
  nsIntPoint mPoint;
};

}

#endif
