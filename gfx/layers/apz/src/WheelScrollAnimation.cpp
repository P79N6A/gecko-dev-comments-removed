





#include "WheelScrollAnimation.h"

namespace mozilla {
namespace layers {

WheelScrollAnimation::WheelScrollAnimation(AsyncPanZoomController& aApzc, const nsPoint& aInitialPosition)
  : AsyncPanZoomAnimation(TimeDuration::FromMilliseconds(gfxPrefs::APZSmoothScrollRepaintInterval()))
  , AsyncScrollBase(aInitialPosition)
  , mApzc(aApzc)
  , mFinalDestination(aInitialPosition)
{
}

void
WheelScrollAnimation::Update(TimeStamp aTime, nsPoint aDelta, const nsSize& aCurrentVelocity)
{
  InitPreferences(aTime);

  mFinalDestination += aDelta;

  
  CSSPoint clamped = CSSPoint::FromAppUnits(mFinalDestination);
  clamped.x = mApzc.mX.ClampOriginToScrollableRect(clamped.x);
  clamped.y = mApzc.mY.ClampOriginToScrollableRect(clamped.y);
  mFinalDestination = CSSPoint::ToAppUnits(clamped);

  AsyncScrollBase::Update(aTime, mFinalDestination, aCurrentVelocity);
}

bool
WheelScrollAnimation::DoSample(FrameMetrics& aFrameMetrics, const TimeDuration& aDelta)
{
  TimeStamp now = AsyncPanZoomController::GetFrameTime();
  CSSToParentLayerScale2D zoom = aFrameMetrics.GetZoom();

  
  
  
  bool finished = IsFinished(now);
  nsPoint sampledDest = finished
                        ? mDestination
                        : PositionAt(now);
  ParentLayerPoint displacement =
    (CSSPoint::FromAppUnits(sampledDest) - aFrameMetrics.GetScrollOffset()) * zoom;

  
  ParentLayerPoint adjustedOffset, overscroll;
  mApzc.mX.AdjustDisplacement(displacement.x, adjustedOffset.x, overscroll.x);
  mApzc.mY.AdjustDisplacement(displacement.y, adjustedOffset.y, overscroll.y,
                              !aFrameMetrics.AllowVerticalScrollWithWheel());

  
  
  
  
  if (!IsZero(displacement) && IsZero(adjustedOffset)) {
    
    return false;
  }

  aFrameMetrics.ScrollBy(adjustedOffset / zoom);
  return !finished;
}

void
WheelScrollAnimation::InitPreferences(TimeStamp aTime)
{
  if (!mIsFirstIteration) {
    return;
  }

  mOriginMaxMS = clamped(gfxPrefs::WheelSmoothScrollMaxDurationMs(), 0, 10000);
  mOriginMinMS = clamped(gfxPrefs::WheelSmoothScrollMinDurationMs(), 0, mOriginMaxMS);

  mIntervalRatio = (gfxPrefs::SmoothScrollDurationToIntervalRatio() * 100) / 100.0;
  mIntervalRatio = std::max(1.0, mIntervalRatio);

  InitializeHistory(aTime);
}

} 
} 
