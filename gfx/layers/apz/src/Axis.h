





#ifndef mozilla_layers_Axis_h
#define mozilla_layers_Axis_h

#include <sys/types.h>                  
#include "Units.h"                      
#include "mozilla/TimeStamp.h"          
#include "nsTArray.h"                   

namespace mozilla {
namespace layers {

const float EPSILON = 0.0001f;







const float COORDINATE_EPSILON = 0.01f;

struct FrameMetrics;
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

  



  void UpdateWithTouchAtDevicePoint(int32_t aPos, uint32_t aTimestampMs);

  



  void StartTouch(int32_t aPos, uint32_t aTimestampMs);

  



  void EndTouch(uint32_t aTimestampMs);

  





  void CancelTouch();

  









  bool AdjustDisplacement(float aDisplacement,
                          float& aDisplacementOut,
                          float& aOverscrollAmountOut);

  



  void OverscrollBy(float aOverscroll);

  


  float GetOverscroll() const;

  



  bool SampleSnapBack(const TimeDuration& aDelta);

  


  bool IsOverscrolled() const;

  


  void ClearOverscroll();

  




  float PanDistance();

  



  float PanDistance(float aPos);

  








  bool FlingApplyFrictionOrCancel(const TimeDuration& aDelta,
                                  float aFriction,
                                  float aThreshold);

  


  bool CanScroll() const;

  



  bool CanScrollNow() const;

  void SetAxisLocked(bool aAxisLocked) { mAxisLocked = aAxisLocked; }

  


  float GetVelocity();

  







  void SetVelocity(float aVelocity);

  




  Overscroll DisplacementWillOverscroll(float aDisplacement);

  



  float DisplacementWillOverscrollAmount(float aDisplacement);

  







  float ScaleWillOverscrollAmount(float aScale, float aFocus);

  






  bool ScaleWillOverscrollBothSides(float aScale);

  float GetOrigin() const;
  float GetCompositionLength() const;
  float GetPageStart() const;
  float GetPageLength() const;
  float GetCompositionEnd() const;
  float GetPageEnd() const;

  int32_t GetPos() const { return mPos; }

  virtual float GetPointOffset(const CSSPoint& aPoint) const = 0;
  virtual float GetRectLength(const CSSRect& aRect) const = 0;
  virtual float GetRectOffset(const CSSRect& aRect) const = 0;

protected:
  int32_t mPos;
  uint32_t mPosTimeMs;
  int32_t mStartPos;
  float mVelocity;
  bool mAxisLocked;     
  AsyncPanZoomController* mAsyncPanZoomController;
  
  
  
  
  
  
  float mOverscroll;
  
  
  
  
  nsTArray<std::pair<uint32_t, float> > mVelocityQueue;

  const FrameMetrics& GetFrameMetrics() const;

  
  
  float ApplyResistance(float aOverscroll) const;
};

class AxisX : public Axis {
public:
  AxisX(AsyncPanZoomController* mAsyncPanZoomController);
  virtual float GetPointOffset(const CSSPoint& aPoint) const;
  virtual float GetRectLength(const CSSRect& aRect) const;
  virtual float GetRectOffset(const CSSRect& aRect) const;
};

class AxisY : public Axis {
public:
  AxisY(AsyncPanZoomController* mAsyncPanZoomController);
  virtual float GetPointOffset(const CSSPoint& aPoint) const;
  virtual float GetRectLength(const CSSRect& aRect) const;
  virtual float GetRectOffset(const CSSRect& aRect) const;
};

}
}

#endif
