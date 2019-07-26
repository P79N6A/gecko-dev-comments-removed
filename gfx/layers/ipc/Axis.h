





#ifndef mozilla_layers_Axis_h
#define mozilla_layers_Axis_h

#include "nsGUIEvent.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/gfx/2D.h"
#include "nsTArray.h"
#include "Units.h"

namespace mozilla {
namespace layers {

class AsyncPanZoomController;






class Axis {
public:
  Axis(AsyncPanZoomController* aAsyncPanZoomController);

  enum Overscroll {
    
    OVERSCROLL_NONE = 0,
    
    
    OVERSCROLL_MINUS,
    
    
    OVERSCROLL_PLUS,
    
    
    OVERSCROLL_BOTH
  };

  




  void UpdateWithTouchAtDevicePoint(int32_t aPos, const TimeDuration& aTimeDelta);

  



  void StartTouch(int32_t aPos);

  



  void EndTouch();

  





  void CancelTouch();

  









  float GetDisplacementForDuration(float aScale, const TimeDuration& aDelta);

  




  float PanDistance();

  





  bool FlingApplyFrictionOrCancel(const TimeDuration& aDelta);

  


  Overscroll GetOverscroll();

  







  float GetExcess();

  



  float GetAccelerationFactor();

  


  float GetVelocity();

  




  Overscroll DisplacementWillOverscroll(float aDisplacement);

  



  float DisplacementWillOverscrollAmount(float aDisplacement);

  








  Overscroll ScaleWillOverscroll(float aScale, float aFocus);

  







  float ScaleWillOverscrollAmount(float aScale, float aFocus);

  






  bool ScaleWillOverscrollBothSides(float aScale);

  float GetOrigin();
  float GetCompositionLength();
  float GetPageStart();
  float GetPageLength();
  float GetCompositionEnd();
  float GetPageEnd();

  virtual float GetPointOffset(const CSSPoint& aPoint) = 0;
  virtual float GetRectLength(const CSSRect& aRect) = 0;
  virtual float GetRectOffset(const CSSRect& aRect) = 0;

protected:
  int32_t mPos;
  int32_t mStartPos;
  float mVelocity;
  
  
  
  
  
  int32_t mAcceleration;
  AsyncPanZoomController* mAsyncPanZoomController;
  nsTArray<float> mVelocityQueue;
};

class AxisX : public Axis {
public:
  AxisX(AsyncPanZoomController* mAsyncPanZoomController);
  virtual float GetPointOffset(const CSSPoint& aPoint);
  virtual float GetRectLength(const CSSRect& aRect);
  virtual float GetRectOffset(const CSSRect& aRect);
};

class AxisY : public Axis {
public:
  AxisY(AsyncPanZoomController* mAsyncPanZoomController);
  virtual float GetPointOffset(const CSSPoint& aPoint);
  virtual float GetRectLength(const CSSRect& aRect);
  virtual float GetRectOffset(const CSSRect& aRect);
};

}
}

#endif
