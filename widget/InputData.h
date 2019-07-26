




#ifndef InputData_h__
#define InputData_h__

#include "nsDebug.h"
#include "nsPoint.h"
#include "nsTArray.h"
#include "Units.h"
#include "mozilla/EventForwards.h"
#include "mozilla/TimeStamp.h"

template<class E> struct already_AddRefed;

namespace mozilla {

namespace dom {
class Touch;
}

enum InputType
{
  MULTITOUCH_INPUT,
  PANGESTURE_INPUT,
  PINCHGESTURE_INPUT,
  TAPGESTURE_INPUT
};

class MultiTouchInput;
class PanGestureInput;
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
  
  
  TimeStamp mTimeStamp;

  Modifiers modifiers;

  INPUTDATA_AS_CHILD_TYPE(MultiTouchInput, MULTITOUCH_INPUT)
  INPUTDATA_AS_CHILD_TYPE(PanGestureInput, PANGESTURE_INPUT)
  INPUTDATA_AS_CHILD_TYPE(PinchGestureInput, PINCHGESTURE_INPUT)
  INPUTDATA_AS_CHILD_TYPE(TapGestureInput, TAPGESTURE_INPUT)

  InputData()
  {
  }

protected:
  InputData(InputType aInputType, uint32_t aTime, TimeStamp aTimeStamp,
            Modifiers aModifiers)
    : mInputType(aInputType),
      mTime(aTime),
      mTimeStamp(aTimeStamp),
      modifiers(aModifiers)
  {


  }
};
















class SingleTouchData
{
public:
  SingleTouchData(int32_t aIdentifier,
                  ScreenIntPoint aScreenPoint,
                  ScreenSize aRadius,
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

  already_AddRefed<dom::Touch> ToNewDOMTouch();

  
  
  int32_t mIdentifier;

  
  
  ScreenIntPoint mScreenPoint;

  
  
  
  
  
  ScreenSize mRadius;

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

  MultiTouchInput(MultiTouchType aType, uint32_t aTime, TimeStamp aTimeStamp,
                  Modifiers aModifiers)
    : InputData(MULTITOUCH_INPUT, aTime, aTimeStamp, aModifiers),
      mType(aType)
  {
  }

  MultiTouchInput()
  {
  }

  MultiTouchInput(const MultiTouchInput& aOther)
    : InputData(MULTITOUCH_INPUT, aOther.mTime,
                aOther.mTimeStamp, aOther.modifiers)
    , mType(aOther.mType)
  {
    mTouches.AppendElements(aOther.mTouches);
  }

  MultiTouchInput(const WidgetTouchEvent& aTouchEvent);

  
  
  
  
  
  
  MultiTouchInput(const WidgetMouseEvent& aMouseEvent);

  MultiTouchType mType;
  nsTArray<SingleTouchData> mTouches;
};





class PanGestureInput : public InputData
{
public:
  enum PanGestureType
  {
    
    
    
    
    
    PANGESTURE_MAYSTART,

    
    
    
    PANGESTURE_CANCELLED,

    
    
    
    PANGESTURE_START,

    
    PANGESTURE_PAN,

    
    
    
    PANGESTURE_END,

    
    
    
    

    
    
    
    PANGESTURE_MOMENTUMSTART,

    
    PANGESTURE_MOMENTUMPAN,

    
    
    
    PANGESTURE_MOMENTUMEND
  };

  PanGestureInput(PanGestureType aType,
                  uint32_t aTime,
                  TimeStamp aTimeStamp,
                  const ScreenPoint& aPanStartPoint,
                  const ScreenPoint& aPanDisplacement,
                  Modifiers aModifiers)
    : InputData(PANGESTURE_INPUT, aTime, aTimeStamp, aModifiers),
      mType(aType),
      mPanStartPoint(aPanStartPoint),
      mPanDisplacement(aPanDisplacement)
  {
  }

  PanGestureType mType;
  ScreenPoint mPanStartPoint;

  
  ScreenPoint mPanDisplacement;
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
                    TimeStamp aTimeStamp,
                    const ScreenPoint& aFocusPoint,
                    float aCurrentSpan,
                    float aPreviousSpan,
                    Modifiers aModifiers)
    : InputData(PINCHGESTURE_INPUT, aTime, aTimeStamp, aModifiers),
      mType(aType),
      mFocusPoint(aFocusPoint),
      mCurrentSpan(aCurrentSpan),
      mPreviousSpan(aPreviousSpan)
  {


  }

  PinchGestureType mType;

  
  
  
  
  
  ScreenPoint mFocusPoint;

  
  
  
  float mCurrentSpan;

  
  
  
  float mPreviousSpan;
};






class TapGestureInput : public InputData
{
public:
  enum TapGestureType
  {
    TAPGESTURE_LONG,
    TAPGESTURE_LONG_UP,
    TAPGESTURE_UP,
    TAPGESTURE_CONFIRMED,
    TAPGESTURE_DOUBLE,
    TAPGESTURE_CANCEL
  };

  TapGestureInput(TapGestureType aType,
                  uint32_t aTime,
                  TimeStamp aTimeStamp,
                  const ScreenIntPoint& aPoint,
                  Modifiers aModifiers)
    : InputData(TAPGESTURE_INPUT, aTime, aTimeStamp, aModifiers),
      mType(aType),
      mPoint(aPoint)
  {


  }

  TapGestureType mType;
  ScreenIntPoint mPoint;
};

}

#endif
