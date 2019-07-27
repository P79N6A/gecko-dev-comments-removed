





#include "ScrollVelocityQueue.h"

#include "gfxPrefs.h"
#include "nsPresContext.h"
#include "nsRefreshDriver.h"

namespace mozilla {
namespace layout {

void
ScrollVelocityQueue::Sample(const nsPoint& aScrollPosition)
{
  float flingSensitivity = gfxPrefs::ScrollSnapPredictionSensitivity();
  int maxVelocity = gfxPrefs::ScrollSnapPredictionMaxVelocity();
  maxVelocity = nsPresContext::CSSPixelsToAppUnits(maxVelocity);
  int maxOffset = maxVelocity * flingSensitivity;
  TimeStamp currentRefreshTime = mPresContext->RefreshDriver()->MostRecentRefresh();
  if (mSampleTime.IsNull()) {
    mAccumulator = nsPoint();
  } else {
    uint32_t durationMs = (currentRefreshTime - mSampleTime).ToMilliseconds();
    if (durationMs > gfxPrefs::APZVelocityRelevanceTime()) {
      mAccumulator = nsPoint();
      mQueue.Clear();
    } else if (durationMs == 0) {
      mAccumulator += aScrollPosition - mLastPosition;
    } else {
      nsPoint velocity = mAccumulator * 1000 / durationMs;
      velocity.Clamp(maxVelocity);
      mQueue.AppendElement(std::make_pair(durationMs, velocity));
      mAccumulator = aScrollPosition - mLastPosition;
    }
  }
  mAccumulator.Clamp(maxOffset);
  mSampleTime = currentRefreshTime;
  mLastPosition = aScrollPosition;
  TrimQueue();
}

void
ScrollVelocityQueue::TrimQueue()
{
  if (mSampleTime.IsNull()) {
    
    return;
  }

  TimeStamp currentRefreshTime = mPresContext->RefreshDriver()->MostRecentRefresh();
  nsPoint velocity;
  uint32_t timeDelta = (currentRefreshTime - mSampleTime).ToMilliseconds();
  for (int i = mQueue.Length() - 1; i >= 0; i--) {
    timeDelta += mQueue[i].first;
    if (timeDelta >= gfxPrefs::APZVelocityRelevanceTime()) {
      
      for (; i >= 0; i--) {
        mQueue.RemoveElementAt(0);
      }
    }
  }
}

void
ScrollVelocityQueue::Reset()
{
  mAccumulator = nsPoint();
  mSampleTime = TimeStamp();
  mQueue.Clear();
}




nsPoint
ScrollVelocityQueue::GetVelocity()
{
  TrimQueue();
  if (mQueue.Length() == 0) {
    
    
    return nsPoint();
  }
  nsPoint velocity;
  for (int i = mQueue.Length() - 1; i >= 0; i--) {
    velocity += mQueue[i].second;
  }
  return velocity / mQueue.Length();;
}

} 
} 

