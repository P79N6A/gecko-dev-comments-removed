




#ifndef InputData_h__
#define InputData_h__

#include "nsDebug.h"
#include "nsPoint.h"
#include "nsTArray.h"

class nsTouchEvent;
class nsMouseEvent;
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

  InputData()
  {
  }

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

  SingleTouchData()
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

  MultiTouchInput()
  {
  }

  MultiTouchInput(const nsTouchEvent& aTouchEvent);

  
  
  
  
  
  
  MultiTouchInput(const nsMouseEvent& aMouseEvent);

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
