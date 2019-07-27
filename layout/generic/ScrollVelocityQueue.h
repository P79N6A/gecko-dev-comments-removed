





#ifndef ScrollVelocityQueue_h_
#define ScrollVelocityQueue_h_

#include "nsTArray.h"
#include "nsPoint.h"
#include "mozilla/TimeStamp.h"

class nsPresContext;

namespace mozilla {
namespace layout {































class ScrollVelocityQueue MOZ_FINAL {
public:
  explicit ScrollVelocityQueue(nsPresContext *aPresContext)
    : mPresContext(aPresContext) {}

  
  
  void Sample(const nsPoint& aScrollPosition);

  
  
  void Reset();

  
  nsPoint GetVelocity();
private:
  
  
  
  nsTArray<std::pair<uint32_t, nsPoint> > mQueue;

  
  
  nsPoint mAccumulator;

  
  TimeStamp mSampleTime;

  
  
  nsPoint mLastPosition;

  
  nsPresContext* mPresContext;

  
  
  void TrimQueue();
};

} 
} 

#endif  
