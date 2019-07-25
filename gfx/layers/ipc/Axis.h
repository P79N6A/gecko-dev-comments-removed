





#ifndef mozilla_layers_Axis_h
#define mozilla_layers_Axis_h

#include "nsGUIEvent.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/gfx/2D.h"

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

  





  void LockPanning();

  









  int32_t GetDisplacementForDuration(float aScale, const TimeDuration& aDelta);

  




  float PanDistance();

  





  bool FlingApplyFrictionOrCancel(const TimeDuration& aDelta);

  


  Overscroll GetOverscroll();

  







  int32_t GetExcess();

  


  float GetVelocity();

  




  Overscroll DisplacementWillOverscroll(int32_t aDisplacement);

  



  int32_t DisplacementWillOverscrollAmount(int32_t aDisplacement);

  








  Overscroll ScaleWillOverscroll(float aScale, int32_t aFocus);

  







  int32_t ScaleWillOverscrollAmount(float aScale, int32_t aFocus);

  






  bool ScaleWillOverscrollBothSides(float aScale);

  int32_t GetOrigin();
  int32_t GetViewportLength();
  int32_t GetPageStart();
  int32_t GetPageLength();
  int32_t GetViewportEnd();
  int32_t GetPageEnd();

  virtual int32_t GetPointOffset(const nsIntPoint& aPoint) = 0;
  virtual int32_t GetRectLength(const gfx::Rect& aRect) = 0;
  virtual int32_t GetRectOffset(const gfx::Rect& aRect) = 0;

protected:
  int32_t mPos;
  int32_t mStartPos;
  float mVelocity;
  
  
  
  
  
  int32_t mAcceleration;
  nsRefPtr<AsyncPanZoomController> mAsyncPanZoomController;
  bool mLockPanning;
};

class AxisX : public Axis {
public:
  AxisX(AsyncPanZoomController* mAsyncPanZoomController);
  virtual int32_t GetPointOffset(const nsIntPoint& aPoint);
  virtual int32_t GetRectLength(const gfx::Rect& aRect);
  virtual int32_t GetRectOffset(const gfx::Rect& aRect);
};

class AxisY : public Axis {
public:
  AxisY(AsyncPanZoomController* mAsyncPanZoomController);
  virtual int32_t GetPointOffset(const nsIntPoint& aPoint);
  virtual int32_t GetRectLength(const gfx::Rect& aRect);
  virtual int32_t GetRectOffset(const gfx::Rect& aRect);
};

}
}

#endif
