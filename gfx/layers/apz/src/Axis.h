





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

  enum Overscroll {
    
    OVERSCROLL_NONE = 0,
    
    
    OVERSCROLL_MINUS,
    
    
    OVERSCROLL_PLUS,
    
    
    OVERSCROLL_BOTH
  };

  



  void UpdateWithTouchAtDevicePoint(ScreenCoord aPos, uint32_t aTimestampMs);

  



  void StartTouch(ScreenCoord aPos, uint32_t aTimestampMs);

  



  void EndTouch(uint32_t aTimestampMs);

  





  void CancelTouch();

  









  bool AdjustDisplacement(CSSCoord aDisplacement,
                           float& aDisplacementOut,
                           float& aOverscrollAmountOut);

  



  void OverscrollBy(CSSCoord aOverscroll);

  


  CSSCoord GetOverscroll() const;

  



  bool SampleSnapBack(const TimeDuration& aDelta);

  


  bool IsOverscrolled() const;

  


  void ClearOverscroll();

  




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

  




  Overscroll DisplacementWillOverscroll(CSSCoord aDisplacement);

  



  CSSCoord DisplacementWillOverscrollAmount(CSSCoord aDisplacement);

  







  CSSCoord ScaleWillOverscrollAmount(float aScale, CSSCoord aFocus);

  






  bool ScaleWillOverscrollBothSides(float aScale);

  CSSCoord GetOrigin() const;
  CSSCoord GetCompositionLength() const;
  CSSCoord GetPageStart() const;
  CSSCoord GetPageLength() const;
  CSSCoord GetCompositionEnd() const;
  CSSCoord GetPageEnd() const;

  ScreenCoord GetPos() const { return mPos; }

  virtual CSSCoord GetPointOffset(const CSSPoint& aPoint) const = 0;
  virtual CSSCoord GetRectLength(const CSSRect& aRect) const = 0;
  virtual CSSCoord GetRectOffset(const CSSRect& aRect) const = 0;

  virtual ScreenPoint MakePoint(ScreenCoord aCoord) const = 0;

protected:
  ScreenCoord mPos;
  uint32_t mPosTimeMs;
  ScreenCoord mStartPos;
  float mVelocity;
  bool mAxisLocked;     
  AsyncPanZoomController* mAsyncPanZoomController;
  
  
  
  
  
  
  CSSCoord mOverscroll;
  
  
  
  
  nsTArray<std::pair<uint32_t, float> > mVelocityQueue;

  const FrameMetrics& GetFrameMetrics() const;

  
  
  CSSCoord ApplyResistance(CSSCoord aOverscroll) const;
};

class AxisX : public Axis {
public:
  explicit AxisX(AsyncPanZoomController* mAsyncPanZoomController);
  virtual CSSCoord GetPointOffset(const CSSPoint& aPoint) const;
  virtual CSSCoord GetRectLength(const CSSRect& aRect) const;
  virtual CSSCoord GetRectOffset(const CSSRect& aRect) const;
  virtual ScreenPoint MakePoint(ScreenCoord aCoord) const;
};

class AxisY : public Axis {
public:
  explicit AxisY(AsyncPanZoomController* mAsyncPanZoomController);
  virtual CSSCoord GetPointOffset(const CSSPoint& aPoint) const;
  virtual CSSCoord GetRectLength(const CSSRect& aRect) const;
  virtual CSSCoord GetRectOffset(const CSSRect& aRect) const;
  virtual ScreenPoint MakePoint(ScreenCoord aCoord) const;
};

}
}

#endif
