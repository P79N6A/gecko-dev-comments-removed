





#ifndef mozilla_layers_Axis_h
#define mozilla_layers_Axis_h

#include "nsGUIEvent.h"
#include "mozilla/TimeStamp.h"

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

  




  void UpdateWithTouchAtDevicePoint(PRInt32 aPos, PRInt32 aTimeDelta);

  



  void StartTouch(PRInt32 aPos);

  




  void StopTouch();

  









  PRInt32 UpdateAndGetDisplacement(float aScale);

  




  float PanDistance();

  





  bool FlingApplyFrictionOrCancel(const TimeDuration& aDelta);

  


  Overscroll GetOverscroll();

  







  PRInt32 GetExcess();

  


  float GetVelocity();

  




  Overscroll DisplacementWillOverscroll(PRInt32 aDisplacement);

  



  PRInt32 DisplacementWillOverscrollAmount(PRInt32 aDisplacement);

  








  Overscroll ScaleWillOverscroll(float aScale, PRInt32 aFocus);

  







  PRInt32 ScaleWillOverscrollAmount(float aScale, PRInt32 aFocus);

  






  bool ScaleWillOverscrollBothSides(float aScale);

  PRInt32 GetOrigin();
  PRInt32 GetViewportLength();
  PRInt32 GetPageStart();
  PRInt32 GetPageLength();
  PRInt32 GetViewportEnd();
  PRInt32 GetPageEnd();

  virtual PRInt32 GetPointOffset(const nsIntPoint& aPoint) = 0;
  virtual PRInt32 GetRectLength(const nsIntRect& aRect) = 0;
  virtual PRInt32 GetRectOffset(const nsIntRect& aRect) = 0;

protected:
  PRInt32 mPos;
  PRInt32 mStartPos;
  float mVelocity;
  nsRefPtr<AsyncPanZoomController> mAsyncPanZoomController;
};

class AxisX : public Axis {
public:
  AxisX(AsyncPanZoomController* mAsyncPanZoomController);
  virtual PRInt32 GetPointOffset(const nsIntPoint& aPoint);
  virtual PRInt32 GetRectLength(const nsIntRect& aRect);
  virtual PRInt32 GetRectOffset(const nsIntRect& aRect);
};

class AxisY : public Axis {
public:
  AxisY(AsyncPanZoomController* mAsyncPanZoomController);
  virtual PRInt32 GetPointOffset(const nsIntPoint& aPoint);
  virtual PRInt32 GetRectLength(const nsIntRect& aRect);
  virtual PRInt32 GetRectOffset(const nsIntRect& aRect);
};

}
}

#endif
