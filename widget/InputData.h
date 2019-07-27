




#ifndef InputData_h__
#define InputData_h__

#include "nsIDOMWheelEvent.h"
#include "nsDebug.h"
#include "nsPoint.h"
#include "nsTArray.h"
#include "Units.h"
#include "mozilla/EventForwards.h"
#include "mozilla/TimeStamp.h"

template<class E> struct already_AddRefed;
class nsIWidget;

namespace mozilla {

namespace dom {
class Touch;
}

namespace gfx {
class Matrix4x4;
}

enum InputType
{
  MULTITOUCH_INPUT,
  PANGESTURE_INPUT,
  PINCHGESTURE_INPUT,
  TAPGESTURE_INPUT,
  SCROLLWHEEL_INPUT
};

class MultiTouchInput;
class PanGestureInput;
class PinchGestureInput;
class TapGestureInput;
class ScrollWheelInput;





#define INPUTDATA_AS_CHILD_TYPE(type, enumID) \
  const type& As##type() const \
  { \
    MOZ_ASSERT(mInputType == enumID, "Invalid cast of InputData."); \
    return (const type&) *this; \
  } \
  type& As##type() \
  { \
    MOZ_ASSERT(mInputType == enumID, "Invalid cast of InputData."); \
    return (type&) *this; \
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
  INPUTDATA_AS_CHILD_TYPE(ScrollWheelInput, SCROLLWHEEL_INPUT)

  explicit InputData(InputType aInputType)
    : mInputType(aInputType),
      mTime(0),
      modifiers(0)
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

  
  
  
  
  SingleTouchData(int32_t aIdentifier,
                  ParentLayerPoint aLocalScreenPoint,
                  ScreenSize aRadius,
                  float aRotationAngle,
                  float aForce)
    : mIdentifier(aIdentifier),
      mLocalScreenPoint(aLocalScreenPoint),
      mRadius(aRadius),
      mRotationAngle(aRotationAngle),
      mForce(aForce)
  {
  }

  SingleTouchData()
  {
  }

  already_AddRefed<dom::Touch> ToNewDOMTouch() const;

  
  
  int32_t mIdentifier;

  
  
  ScreenIntPoint mScreenPoint;

  
  
  ParentLayerPoint mLocalScreenPoint;

  
  
  
  
  
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
    MULTITOUCH_CANCEL
  };

  MultiTouchInput(MultiTouchType aType, uint32_t aTime, TimeStamp aTimeStamp,
                  Modifiers aModifiers)
    : InputData(MULTITOUCH_INPUT, aTime, aTimeStamp, aModifiers),
      mType(aType)
  {
  }

  MultiTouchInput()
    : InputData(MULTITOUCH_INPUT)
  {
  }

  MultiTouchInput(const MultiTouchInput& aOther)
    : InputData(MULTITOUCH_INPUT, aOther.mTime,
                aOther.mTimeStamp, aOther.modifiers)
    , mType(aOther.mType)
  {
    mTouches.AppendElements(aOther.mTouches);
  }

  explicit MultiTouchInput(const WidgetTouchEvent& aTouchEvent);
  WidgetTouchEvent ToWidgetTouchEvent(nsIWidget* aWidget) const;
  WidgetMouseEvent ToWidgetMouseEvent(nsIWidget* aWidget) const;

  
  
  int32_t IndexOfTouch(int32_t aTouchIdentifier);

  
  
  
  
  
  
  explicit MultiTouchInput(const WidgetMouseEvent& aMouseEvent);

  void TransformToLocal(const gfx::Matrix4x4& aTransform);

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

  void TransformToLocal(const gfx::Matrix4x4& aTransform);

  PanGestureType mType;
  ScreenPoint mPanStartPoint;

  
  ScreenPoint mPanDisplacement;

  
  
  ParentLayerPoint mLocalPanStartPoint;
  ParentLayerPoint mLocalPanDisplacement;
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

  
  
  PinchGestureInput(PinchGestureType aType,
                    uint32_t aTime,
                    TimeStamp aTimeStamp,
                    const ParentLayerPoint& aLocalFocusPoint,
                    float aCurrentSpan,
                    float aPreviousSpan,
                    Modifiers aModifiers)
    : InputData(PINCHGESTURE_INPUT, aTime, aTimeStamp, aModifiers),
      mType(aType),
      mLocalFocusPoint(aLocalFocusPoint),
      mCurrentSpan(aCurrentSpan),
      mPreviousSpan(aPreviousSpan)
  {
  }

  void TransformToLocal(const gfx::Matrix4x4& aTransform);

  PinchGestureType mType;

  
  
  
  
  
  ScreenPoint mFocusPoint;

  
  
  ParentLayerPoint mLocalFocusPoint;

  
  
  
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

  
  
  TapGestureInput(TapGestureType aType,
                  uint32_t aTime,
                  TimeStamp aTimeStamp,
                  const ParentLayerPoint& aLocalPoint,
                  Modifiers aModifiers)
    : InputData(TAPGESTURE_INPUT, aTime, aTimeStamp, aModifiers),
      mType(aType),
      mLocalPoint(aLocalPoint)
  {
  }

  void TransformToLocal(const gfx::Matrix4x4& aTransform);

  TapGestureType mType;

  
  ScreenIntPoint mPoint;

  
  
  ParentLayerPoint mLocalPoint;
};




class ScrollWheelInput : public InputData
{
public:
  enum ScrollDeltaType
  {
    
    
    SCROLLDELTA_LINE,
    SCROLLDELTA_PIXEL
  };

  static ScrollDeltaType
  DeltaTypeForDeltaMode(uint32_t aDeltaMode)
  {
    switch (aDeltaMode) {
      case nsIDOMWheelEvent::DOM_DELTA_LINE:
        return SCROLLDELTA_LINE;
      case nsIDOMWheelEvent::DOM_DELTA_PIXEL:
        return SCROLLDELTA_PIXEL;
      default:
        MOZ_CRASH();
    }
    return SCROLLDELTA_LINE;
  }

  enum ScrollMode
  {
    SCROLLMODE_INSTANT,
    SCROLLMODE_SMOOTH
  };

  ScrollWheelInput(uint32_t aTime,
                   TimeStamp aTimeStamp,
                   Modifiers aModifiers,
                   ScrollMode aScrollMode,
                   ScrollDeltaType aDeltaType,
                   const ScreenPoint& aOrigin,
                   double aDeltaX,
                   double aDeltaY)
   : InputData(SCROLLWHEEL_INPUT, aTime, aTimeStamp, aModifiers),
     mDeltaType(aDeltaType),
     mScrollMode(aScrollMode),
     mOrigin(aOrigin),
     mDeltaX(aDeltaX),
     mDeltaY(aDeltaY)
  {}

  void TransformToLocal(const gfx::Matrix4x4& aTransform);

  ScrollDeltaType mDeltaType;
  ScrollMode mScrollMode;
  ScreenPoint mOrigin;

  
  
  
  
  
  
  
  double mDeltaX;
  double mDeltaY;

  
  
  ParentLayerPoint mLocalOrigin;
};

}

#endif
