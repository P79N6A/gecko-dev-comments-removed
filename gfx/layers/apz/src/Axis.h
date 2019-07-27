





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
  explicit Axis(AsyncPanZoomController* aAsyncPanZoomController);

  



  void UpdateWithTouchAtDevicePoint(ScreenCoord aPos, uint32_t aTimestampMs);

  



  void StartTouch(ScreenCoord aPos, uint32_t aTimestampMs);

  



  void EndTouch(uint32_t aTimestampMs);

  





  void CancelTouch();

  









  bool AdjustDisplacement(ScreenCoord aDisplacement,
                           float& aDisplacementOut,
                           float& aOverscrollAmountOut);

  



  void OverscrollBy(ScreenCoord aOverscroll);

  


  ScreenCoord GetOverscroll() const;

  



  bool SampleSnapBack(const TimeDuration& aDelta);

  


  bool IsOverscrolled() const;

  


  void ClearOverscroll();

  


  ScreenCoord PanStart() const;

  




  ScreenCoord PanDistance() const;

  



  ScreenCoord PanDistance(ScreenCoord aPos) const;

  








  bool FlingApplyFrictionOrCancel(const TimeDuration& aDelta,
                                  float aFriction,
                                  float aThreshold);

  


  bool CanScroll() const;

  



  bool CanScrollNow() const;

  void SetAxisLocked(bool aAxisLocked) { mAxisLocked = aAxisLocked; }

  


  float GetVelocity();

  







  void SetVelocity(float aVelocity);

  



  ScreenCoord DisplacementWillOverscrollAmount(ScreenCoord aDisplacement) const;

  











  CSSCoord ScaleWillOverscrollAmount(float aScale, CSSCoord aFocus) const;

  






  bool ScaleWillOverscrollBothSides(float aScale) const;

  ScreenCoord GetOrigin() const;
  ScreenCoord GetCompositionLength() const;
  ScreenCoord GetPageStart() const;
  ScreenCoord GetPageLength() const;
  ScreenCoord GetCompositionEnd() const;
  ScreenCoord GetPageEnd() const;

  ScreenCoord GetPos() const { return mPos; }

  virtual ScreenCoord GetPointOffset(const ScreenPoint& aPoint) const = 0;
  virtual ScreenCoord GetRectLength(const ScreenRect& aRect) const = 0;
  virtual ScreenCoord GetRectOffset(const ScreenRect& aRect) const = 0;

  virtual ScreenPoint MakePoint(ScreenCoord aCoord) const = 0;

protected:
  ScreenCoord mPos;
  uint32_t mPosTimeMs;
  ScreenCoord mStartPos;
  float mVelocity;      
  bool mAxisLocked;     
  AsyncPanZoomController* mAsyncPanZoomController;
  
  
  
  
  
  
  ScreenCoord mOverscroll;
  
  
  
  
  nsTArray<std::pair<uint32_t, float> > mVelocityQueue;

  const FrameMetrics& GetFrameMetrics() const;

  
  
  ScreenCoord ApplyResistance(ScreenCoord aOverscroll) const;
};

class AxisX : public Axis {
public:
  explicit AxisX(AsyncPanZoomController* mAsyncPanZoomController);
  virtual ScreenCoord GetPointOffset(const ScreenPoint& aPoint) const MOZ_OVERRIDE;
  virtual ScreenCoord GetRectLength(const ScreenRect& aRect) const MOZ_OVERRIDE;
  virtual ScreenCoord GetRectOffset(const ScreenRect& aRect) const MOZ_OVERRIDE;
  virtual ScreenPoint MakePoint(ScreenCoord aCoord) const MOZ_OVERRIDE;
};

class AxisY : public Axis {
public:
  explicit AxisY(AsyncPanZoomController* mAsyncPanZoomController);
  virtual ScreenCoord GetPointOffset(const ScreenPoint& aPoint) const MOZ_OVERRIDE;
  virtual ScreenCoord GetRectLength(const ScreenRect& aRect) const MOZ_OVERRIDE;
  virtual ScreenCoord GetRectOffset(const ScreenRect& aRect) const MOZ_OVERRIDE;
  virtual ScreenPoint MakePoint(ScreenCoord aCoord) const MOZ_OVERRIDE;
};

}
}

#endif
