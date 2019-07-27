




#include "FrameAnimator.h"
#include "FrameBlender.h"

#include "imgIContainer.h"

namespace mozilla {
namespace image {

FrameAnimator::FrameAnimator(FrameBlender& aFrameBlender,
                             uint16_t aAnimationMode)
  : mCurrentAnimationFrameIndex(0)
  , mLoopCounter(-1)
  , mFrameBlender(aFrameBlender)
  , mAnimationMode(aAnimationMode)
  , mDoneDecoding(false)
{
}

int32_t
FrameAnimator::GetSingleLoopTime() const
{
  
  if (!mDoneDecoding) {
    return -1;
  }

  
  if (mAnimationMode != imgIContainer::kNormalAnimMode) {
    return -1;
  }

  uint32_t looptime = 0;
  for (uint32_t i = 0; i < mFrameBlender.GetNumFrames(); ++i) {
    int32_t timeout = mFrameBlender.GetTimeoutForFrame(i);
    if (timeout >= 0) {
      looptime += static_cast<uint32_t>(timeout);
    } else {
      
      
      NS_WARNING("Negative frame timeout - how did this happen?");
      return -1;
    }
  }

  return looptime;
}

TimeStamp
FrameAnimator::GetCurrentImgFrameEndTime() const
{
  TimeStamp currentFrameTime = mCurrentAnimationFrameTime;
  int32_t timeout = mFrameBlender.GetTimeoutForFrame(mCurrentAnimationFrameIndex);

  if (timeout < 0) {
    
    
    
    
    
    
    return TimeStamp::NowLoRes() +
           TimeDuration::FromMilliseconds(31536000.0);
  }

  TimeDuration durationOfTimeout =
    TimeDuration::FromMilliseconds(static_cast<double>(timeout));
  TimeStamp currentFrameEndTime = currentFrameTime + durationOfTimeout;

  return currentFrameEndTime;
}

FrameAnimator::RefreshResult
FrameAnimator::AdvanceFrame(TimeStamp aTime)
{
  NS_ASSERTION(aTime <= TimeStamp::Now(),
               "Given time appears to be in the future");

  uint32_t currentFrameIndex = mCurrentAnimationFrameIndex;
  uint32_t nextFrameIndex = currentFrameIndex + 1;
  int32_t timeout = 0;

  RefreshResult ret;
  nsRefPtr<imgFrame> nextFrame = mFrameBlender.RawGetFrame(nextFrameIndex);

  
  
  
  bool canDisplay = mDoneDecoding || (nextFrame && nextFrame->ImageComplete());

  if (!canDisplay) {
    
    
    return ret;
  }

  
  
  if (mFrameBlender.GetNumFrames() == nextFrameIndex) {
    

    
    if (mLoopCounter < 0 && mFrameBlender.GetLoopCount() >= 0) {
      mLoopCounter = mFrameBlender.GetLoopCount();
    }

    
    if (mAnimationMode == imgIContainer::kLoopOnceAnimMode || mLoopCounter == 0) {
      ret.animationFinished = true;
    }

    nextFrameIndex = 0;

    if (mLoopCounter > 0) {
      mLoopCounter--;
    }

    
    if (ret.animationFinished) {
      return ret;
    }
  }

  timeout = mFrameBlender.GetTimeoutForFrame(nextFrameIndex);

  
  if (timeout < 0) {
    ret.animationFinished = true;
    ret.error = true;
  }

  if (nextFrameIndex == 0) {
    ret.dirtyRect = mFirstFrameRefreshArea;
  } else {
    
    if (nextFrameIndex != currentFrameIndex + 1) {
      nextFrame = mFrameBlender.RawGetFrame(nextFrameIndex);
    }

    if (!mFrameBlender.DoBlend(&ret.dirtyRect, currentFrameIndex, nextFrameIndex)) {
      
      NS_WARNING("FrameAnimator::AdvanceFrame(): Compositing of frame failed");
      nextFrame->SetCompositingFailed(true);
      mCurrentAnimationFrameTime = GetCurrentImgFrameEndTime();
      mCurrentAnimationFrameIndex = nextFrameIndex;

      ret.error = true;
      return ret;
    }

    nextFrame->SetCompositingFailed(false);
  }

  mCurrentAnimationFrameTime = GetCurrentImgFrameEndTime();

  
  
  uint32_t loopTime = GetSingleLoopTime();
  if (loopTime > 0) {
    TimeDuration delay = aTime - mCurrentAnimationFrameTime;
    if (delay.ToMilliseconds() > loopTime) {
      
      
      uint32_t loops = static_cast<uint32_t>(delay.ToMilliseconds()) / loopTime;
      mCurrentAnimationFrameTime += TimeDuration::FromMilliseconds(loops * loopTime);
    }
  }

  
  mCurrentAnimationFrameIndex = nextFrameIndex;

  
  ret.frameAdvanced = true;

  return ret;
}

FrameAnimator::RefreshResult
FrameAnimator::RequestRefresh(const TimeStamp& aTime)
{
  
  
  TimeStamp currentFrameEndTime = GetCurrentImgFrameEndTime();

  
  RefreshResult ret;

  while (currentFrameEndTime <= aTime) {
    TimeStamp oldFrameEndTime = currentFrameEndTime;

    RefreshResult frameRes = AdvanceFrame(aTime);

    
    ret.Accumulate(frameRes);

    currentFrameEndTime = GetCurrentImgFrameEndTime();

    
    
    
    if (!frameRes.frameAdvanced && (currentFrameEndTime == oldFrameEndTime)) {
      break;
    }
  }

  return ret;
}

void
FrameAnimator::ResetAnimation()
{
  mCurrentAnimationFrameIndex = 0;
}

void
FrameAnimator::SetDoneDecoding(bool aDone)
{
  mDoneDecoding = aDone;
}

void
FrameAnimator::SetAnimationMode(uint16_t aAnimationMode)
{
  mAnimationMode = aAnimationMode;
}

void
FrameAnimator::InitAnimationFrameTimeIfNecessary()
{
  if (mCurrentAnimationFrameTime.IsNull()) {
    mCurrentAnimationFrameTime = TimeStamp::Now();
  }
}

void
FrameAnimator::SetAnimationFrameTime(const TimeStamp& aTime)
{
  mCurrentAnimationFrameTime = aTime;
}

void
FrameAnimator::SetFirstFrameRefreshArea(const nsIntRect& aRect)
{
  mFirstFrameRefreshArea = aRect;
}

void
FrameAnimator::UnionFirstFrameRefreshArea(const nsIntRect& aRect)
{
  mFirstFrameRefreshArea.UnionRect(mFirstFrameRefreshArea, aRect);
}

uint32_t
FrameAnimator::GetCurrentAnimationFrameIndex() const
{
  return mCurrentAnimationFrameIndex;
}

nsIntRect
FrameAnimator::GetFirstFrameRefreshArea() const
{
  return mFirstFrameRefreshArea;
}

} 
} 
