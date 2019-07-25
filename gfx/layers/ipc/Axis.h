





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

  




  void UpdateWithTouchAtDevicePoint(PRInt32 aPos, const TimeDuration& aTimeDelta);

  



  void StartTouch(PRInt32 aPos);

  



  void EndTouch();

  





  void CancelTouch();

  





  void LockPanning();

  









  float GetDisplacementForDuration(float aScale, const TimeDuration& aDelta);

  




  float PanDistance();

  





  bool FlingApplyFrictionOrCancel(const TimeDuration& aDelta);

  


  Overscroll GetOverscroll();

  







  float GetExcess();

  


  float GetVelocity();

  




  Overscroll DisplacementWillOverscroll(PRInt32 aDisplacement);

  



  float DisplacementWillOverscrollAmount(PRInt32 aDisplacement);

  








  Overscroll ScaleWillOverscroll(float aScale, PRInt32 aFocus);

  







  float ScaleWillOverscrollAmount(float aScale, PRInt32 aFocus);

  






  bool ScaleWillOverscrollBothSides(float aScale);

  float GetOrigin();
  float GetViewportLength();
  float GetPageStart();
  float GetPageLength();
  float GetViewportEnd();
  float GetPageEnd();

  virtual float GetPointOffset(const gfx::Point& aPoint) = 0;
  virtual float GetRectLength(const gfx::Rect& aRect) = 0;
  virtual float GetRectOffset(const gfx::Rect& aRect) = 0;

protected:
  PRInt32 mPos;
  PRInt32 mStartPos;
  float mVelocity;
  
  
  
  
  
  PRInt32 mAcceleration;
  nsRefPtr<AsyncPanZoomController> mAsyncPanZoomController;
  bool mLockPanning;
};

class AxisX : public Axis {
public:
  AxisX(AsyncPanZoomController* mAsyncPanZoomController);
  virtual float GetPointOffset(const gfx::Point& aPoint);
  virtual float GetRectLength(const gfx::Rect& aRect);
  virtual float GetRectOffset(const gfx::Rect& aRect);
};

class AxisY : public Axis {
public:
  AxisY(AsyncPanZoomController* mAsyncPanZoomController);
  virtual float GetPointOffset(const gfx::Point& aPoint);
  virtual float GetRectLength(const gfx::Rect& aRect);
  virtual float GetRectOffset(const gfx::Rect& aRect);
};

}
}

#endif
