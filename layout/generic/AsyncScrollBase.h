




#ifndef mozilla_layout_AsyncScrollBase_h_
#define mozilla_layout_AsyncScrollBase_h_

#include "mozilla/TimeStamp.h"
#include "nsPoint.h"
#include "nsSMILKeySpline.h"

namespace mozilla {



class AsyncScrollBase
{
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  explicit AsyncScrollBase(nsPoint aStartPos);

  void Update(TimeStamp aTime,
              nsPoint aDestination,
              const nsSize& aCurrentVelocity);

  
  nsSize VelocityAt(TimeStamp aTime) const;

  
  
  nsPoint PositionAt(TimeStamp aTime) const;

  bool IsFinished(TimeStamp aTime) {
    return aTime > mStartTime + mDuration;
  }

protected:
  double ProgressAt(TimeStamp aTime) const {
    return clamped((aTime - mStartTime) / mDuration, 0.0, 1.0);
  }

  nscoord VelocityComponent(double aTimeProgress,
                            const nsSMILKeySpline& aTimingFunction,
                            nscoord aStart, nscoord aDestination) const;

  
  
  
  TimeDuration ComputeDuration(TimeStamp aTime);

  
  void InitializeHistory(TimeStamp aTime);

  
  
  void InitTimingFunction(nsSMILKeySpline& aTimingFunction,
                          nscoord aCurrentPos, nscoord aCurrentVelocity,
                          nscoord aDestination);

  
  
  
  
  
  TimeStamp mPrevEventTime[3];
  bool mIsFirstIteration;

  TimeStamp mStartTime;

  
  
  
  
  
  
  int32_t mOriginMinMS;
  int32_t mOriginMaxMS;
  double mIntervalRatio;

  nsPoint mStartPos;
  TimeDuration mDuration;
  nsPoint mDestination;
  nsSMILKeySpline mTimingFunctionX;
  nsSMILKeySpline mTimingFunctionY;
};

}

#endif 
