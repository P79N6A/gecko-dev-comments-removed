





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









bool FuzzyEqualsCoordinate(float aValue1, float aValue2);

struct FrameMetrics;
class AsyncPanZoomController;






class Axis {
public:
  explicit Axis(AsyncPanZoomController* aAsyncPanZoomController);

  



  void UpdateWithTouchAtDevicePoint(ParentLayerCoord aPos, uint32_t aTimestampMs);

  



  void StartTouch(ParentLayerCoord aPos, uint32_t aTimestampMs);

  



  void EndTouch(uint32_t aTimestampMs);

  





  void CancelTouch();

  









  bool AdjustDisplacement(ParentLayerCoord aDisplacement,
                           float& aDisplacementOut,
                           float& aOverscrollAmountOut,
                          bool forceOverscroll = false);

  



  void OverscrollBy(ParentLayerCoord aOverscroll);

  








  ParentLayerCoord GetOverscroll() const;

  



  bool SampleOverscrollAnimation(const TimeDuration& aDelta);

  


  bool IsOverscrolled() const;

  


  void ClearOverscroll();

  


  ParentLayerCoord PanStart() const;

  




  ParentLayerCoord PanDistance() const;

  



  ParentLayerCoord PanDistance(ParentLayerCoord aPos) const;

  








  bool FlingApplyFrictionOrCancel(const TimeDuration& aDelta,
                                  float aFriction,
                                  float aThreshold);

  


  bool CanScroll() const;

  


  bool CanScroll(double aDelta) const;

  



  bool CanScrollNow() const;

  



  CSSCoord ClampOriginToScrollableRect(CSSCoord aOrigin) const;

  void SetAxisLocked(bool aAxisLocked) { mAxisLocked = aAxisLocked; }

  


  float GetVelocity() const;

  







  void SetVelocity(float aVelocity);

  



  ParentLayerCoord DisplacementWillOverscrollAmount(ParentLayerCoord aDisplacement) const;

  











  CSSCoord ScaleWillOverscrollAmount(float aScale, CSSCoord aFocus) const;

  






  bool ScaleWillOverscrollBothSides(float aScale) const;

  ParentLayerCoord GetOrigin() const;
  ParentLayerCoord GetCompositionLength() const;
  ParentLayerCoord GetPageStart() const;
  ParentLayerCoord GetPageLength() const;
  ParentLayerCoord GetCompositionEnd() const;
  ParentLayerCoord GetPageEnd() const;

  ParentLayerCoord GetPos() const { return mPos; }

  virtual ParentLayerCoord GetPointOffset(const ParentLayerPoint& aPoint) const = 0;
  virtual ParentLayerCoord GetRectLength(const ParentLayerRect& aRect) const = 0;
  virtual ParentLayerCoord GetRectOffset(const ParentLayerRect& aRect) const = 0;
  virtual CSSToParentLayerScale GetScaleForAxis(const CSSToParentLayerScale2D& aScale) const = 0;

  virtual ScreenPoint MakePoint(ScreenCoord aCoord) const = 0;

  virtual const char* Name() const = 0;

protected:
  ParentLayerCoord mPos;
  uint32_t mPosTimeMs;
  ParentLayerCoord mStartPos;
  float mVelocity;      
  bool mAxisLocked;     
  AsyncPanZoomController* mAsyncPanZoomController;

  
  
  ParentLayerCoord mOverscroll;
  
  ParentLayerCoord mFirstOverscrollAnimationSample;
  
  
  
  
  
  
  
  
  
  ParentLayerCoord mLastOverscrollPeak;
  float mOverscrollScale;

  
  
  
  
  nsTArray<std::pair<uint32_t, float> > mVelocityQueue;

  const FrameMetrics& GetFrameMetrics() const;

  
  
  ParentLayerCoord ApplyResistance(ParentLayerCoord aOverscroll) const;

  
  
  void StopSamplingOverscrollAnimation();

  
  void StepOverscrollAnimation(double aStepDurationMilliseconds);

  
  float ToLocalVelocity(float aVelocityInchesPerMs) const;
};

class AxisX : public Axis {
public:
  explicit AxisX(AsyncPanZoomController* mAsyncPanZoomController);
  virtual ParentLayerCoord GetPointOffset(const ParentLayerPoint& aPoint) const override;
  virtual ParentLayerCoord GetRectLength(const ParentLayerRect& aRect) const override;
  virtual ParentLayerCoord GetRectOffset(const ParentLayerRect& aRect) const override;
  virtual CSSToParentLayerScale GetScaleForAxis(const CSSToParentLayerScale2D& aScale) const override;
  virtual ScreenPoint MakePoint(ScreenCoord aCoord) const override;
  virtual const char* Name() const override;
};

class AxisY : public Axis {
public:
  explicit AxisY(AsyncPanZoomController* mAsyncPanZoomController);
  virtual ParentLayerCoord GetPointOffset(const ParentLayerPoint& aPoint) const override;
  virtual ParentLayerCoord GetRectLength(const ParentLayerRect& aRect) const override;
  virtual ParentLayerCoord GetRectOffset(const ParentLayerRect& aRect) const override;
  virtual CSSToParentLayerScale GetScaleForAxis(const CSSToParentLayerScale2D& aScale) const override;
  virtual ScreenPoint MakePoint(ScreenCoord aCoord) const override;
  virtual const char* Name() const override;
};

}
}

#endif
