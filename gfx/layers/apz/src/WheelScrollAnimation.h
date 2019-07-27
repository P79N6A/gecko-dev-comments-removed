





#ifndef mozilla_layers_WheelScrollAnimation_h_
#define mozilla_layers_WheelScrollAnimation_h_

#include "AsyncPanZoomAnimation.h"
#include "AsyncScrollBase.h"

namespace mozilla {
namespace layers {

class WheelScrollAnimation
  : public AsyncPanZoomAnimation,
    public AsyncScrollBase
{
public:
  WheelScrollAnimation(AsyncPanZoomController& aApzc, const nsPoint& aInitialPosition);

  bool DoSample(FrameMetrics& aFrameMetrics, const TimeDuration& aDelta) override;
  void Update(TimeStamp aTime, nsPoint aDelta, const nsSize& aCurrentVelocity);

  WheelScrollAnimation* AsWheelScrollAnimation() override {
    return this;
  }

private:
  void InitPreferences(TimeStamp aTime);

private:
  AsyncPanZoomController& mApzc;
  nsPoint mFinalDestination;
};

} 
} 

#endif 
