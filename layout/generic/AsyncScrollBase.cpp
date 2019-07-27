




#include "AsyncScrollBase.h"

using namespace mozilla;

AsyncScrollBase::AsyncScrollBase(nsPoint aStartPos)
 : mIsFirstIteration(true)
 , mStartPos(aStartPos)
{
}

void
AsyncScrollBase::Update(TimeStamp aTime,
                        nsPoint aDestination,
                        const nsSize& aCurrentVelocity)
{
  TimeDuration duration = ComputeDuration(aTime);
  nsSize currentVelocity = aCurrentVelocity;

  if (!mIsFirstIteration) {
    
    
    
    if (aDestination == mDestination &&
        aTime + duration > mStartTime + mDuration)
    {
      return;
    }

    currentVelocity = VelocityAt(aTime);
    mStartPos = PositionAt(aTime);
  }

  mStartTime = aTime;
  mDuration = duration;
  mDestination = aDestination;
  InitTimingFunction(mTimingFunctionX, mStartPos.x, currentVelocity.width,
                     aDestination.x);
  InitTimingFunction(mTimingFunctionY, mStartPos.y, currentVelocity.height,
                     aDestination.y);
  mIsFirstIteration = false;
}

TimeDuration
AsyncScrollBase::ComputeDuration(TimeStamp aTime)
{
  
  int32_t eventsDeltaMs = (aTime - mPrevEventTime[2]).ToMilliseconds() / 3;
  mPrevEventTime[2] = mPrevEventTime[1];
  mPrevEventTime[1] = mPrevEventTime[0];
  mPrevEventTime[0] = aTime;

  
  
  
  
  
  int32_t durationMS = clamped<int32_t>(eventsDeltaMs * mIntervalRatio, mOriginMinMS, mOriginMaxMS);

  return TimeDuration::FromMilliseconds(durationMS);
}

void
AsyncScrollBase::InitializeHistory(TimeStamp aTime)
{
  
  

  
  TimeDuration maxDelta = TimeDuration::FromMilliseconds(mOriginMaxMS / mIntervalRatio);
  mPrevEventTime[0] = aTime              - maxDelta;
  mPrevEventTime[1] = mPrevEventTime[0]  - maxDelta;
  mPrevEventTime[2] = mPrevEventTime[1]  - maxDelta;
}

const double kCurrentVelocityWeighting = 0.25;
const double kStopDecelerationWeighting = 0.4;

void
AsyncScrollBase::InitTimingFunction(nsSMILKeySpline& aTimingFunction,
                                    nscoord aCurrentPos,
                                    nscoord aCurrentVelocity,
                                    nscoord aDestination)
{
  if (aDestination == aCurrentPos || kCurrentVelocityWeighting == 0) {
    aTimingFunction.Init(0, 0, 1 - kStopDecelerationWeighting, 1);
    return;
  }

  const TimeDuration oneSecond = TimeDuration::FromSeconds(1);
  double slope = aCurrentVelocity * (mDuration / oneSecond) / (aDestination - aCurrentPos);
  double normalization = sqrt(1.0 + slope * slope);
  double dt = 1.0 / normalization * kCurrentVelocityWeighting;
  double dxy = slope / normalization * kCurrentVelocityWeighting;
  aTimingFunction.Init(dt, dxy, 1 - kStopDecelerationWeighting, 1);
}

nsPoint
AsyncScrollBase::PositionAt(TimeStamp aTime) const
{
  double progressX = mTimingFunctionX.GetSplineValue(ProgressAt(aTime));
  double progressY = mTimingFunctionY.GetSplineValue(ProgressAt(aTime));
  return nsPoint(NSToCoordRound((1 - progressX) * mStartPos.x + progressX * mDestination.x),
                 NSToCoordRound((1 - progressY) * mStartPos.y + progressY * mDestination.y));
}

nsSize
AsyncScrollBase::VelocityAt(TimeStamp aTime) const
{
  double timeProgress = ProgressAt(aTime);
  return nsSize(VelocityComponent(timeProgress, mTimingFunctionX,
                                  mStartPos.x, mDestination.x),
                VelocityComponent(timeProgress, mTimingFunctionY,
                                  mStartPos.y, mDestination.y));
}

nscoord
AsyncScrollBase::VelocityComponent(double aTimeProgress,
                                   const nsSMILKeySpline& aTimingFunction,
                                   nscoord aStart,
                                   nscoord aDestination) const
{
  double dt, dxy;
  aTimingFunction.GetSplineDerivativeValues(aTimeProgress, dt, dxy);
  if (dt == 0)
    return dxy >= 0 ? nscoord_MAX : nscoord_MIN;

  const TimeDuration oneSecond = TimeDuration::FromSeconds(1);
  double slope = dxy / dt;
  return NSToCoordRound(slope * (aDestination - aStart) / (mDuration / oneSecond));
}
