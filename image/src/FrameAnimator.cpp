




#include "FrameAnimator.h"
#include "FrameBlender.h"

#include "imgIContainer.h"

using namespace mozilla::image;
using namespace mozilla;

FrameAnimator::FrameAnimator(FrameBlender& aFrameBlender)
  : mCurrentAnimationFrameIndex(0)
  , mLoopCount(-1)
  , mFrameBlender(aFrameBlender)
  , mAnimationMode(imgIContainer::kNormalAnimMode)
  , mDoneDecoding(false)
{
}

uint32_t
FrameAnimator::GetSingleLoopTime() const
{
  
  if (!mDoneDecoding) {
    return 0;
  }

  
  if (mAnimationMode != imgIContainer::kNormalAnimMode) {
    return 0;
  }

  uint32_t looptime = 0;
  for (uint32_t i = 0; i < mFrameBlender.GetNumFrames(); ++i) {
    int32_t timeout = mFrameBlender.RawGetFrame(i)->GetTimeout();
    if (timeout > 0) {
      looptime += static_cast<uint32_t>(timeout);
    } else {
      
      
      NS_WARNING("Negative frame timeout - how did this happen?");
      return 0;
    }
  }

  return looptime;
}

TimeStamp
FrameAnimator::GetCurrentImgFrameEndTime() const
{
  imgFrame* currentFrame = mFrameBlender.RawGetFrame(mCurrentAnimationFrameIndex);
  TimeStamp currentFrameTime = mCurrentAnimationFrameTime;
  int64_t timeout = currentFrame->GetTimeout();

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
  uint32_t timeout = 0;

  RefreshResult ret;

  
  
  
  bool needToWait = !mDoneDecoding &&
                    mFrameBlender.RawGetFrame(nextFrameIndex) &&
                    !mFrameBlender.RawGetFrame(nextFrameIndex)->ImageComplete();

  if (needToWait) {
    
    
    return ret;
  } else {
    
    
    if (mFrameBlender.GetNumFrames() == nextFrameIndex) {
      

      
      if (mAnimationMode == imgIContainer::kLoopOnceAnimMode || mLoopCount == 0) {
        ret.animationFinished = true;
      }

      nextFrameIndex = 0;

      if (mLoopCount > 0) {
        mLoopCount--;
      }

      
      if (ret.animationFinished) {
        return ret;
      }
    }

    timeout = mFrameBlender.GetFrame(nextFrameIndex)->GetTimeout();
  }

  
  if (!(timeout > 0)) {
    ret.animationFinished = true;
    ret.error = true;
  }

  if (nextFrameIndex == 0) {
    ret.dirtyRect = mFirstFrameRefreshArea;
  } else {
    
    if (!mFrameBlender.DoBlend(&ret.dirtyRect, currentFrameIndex, nextFrameIndex)) {
      
      NS_WARNING("FrameAnimator::AdvanceFrame(): Compositing of frame failed");
      mFrameBlender.RawGetFrame(nextFrameIndex)->SetCompositingFailed(true);
      mCurrentAnimationFrameTime = GetCurrentImgFrameEndTime();
      mCurrentAnimationFrameIndex = nextFrameIndex;

      ret.error = true;
      return ret;
    }

    mFrameBlender.RawGetFrame(nextFrameIndex)->SetCompositingFailed(false);
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
FrameAnimator::RequestRefresh(const mozilla::TimeStamp& aTime)
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

void
FrameAnimator::SetLoopCount(int loopcount)
{
  mLoopCount = loopcount;
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


