




#include "FrameAnimator.h"

#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
#include "imgIContainer.h"
#include "MainThreadUtils.h"
#include "RasterImage.h"

#include "pixman.h"

namespace mozilla {

using namespace gfx;

namespace image {

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
  for (uint32_t i = 0; i < mImage->GetNumFrames(); ++i) {
    int32_t timeout = GetTimeoutForFrame(i);
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
  int32_t timeout =
    GetTimeoutForFrame(mCurrentAnimationFrameIndex);

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
  RawAccessFrameRef nextFrame = GetRawFrame(nextFrameIndex);

  
  
  
  bool canDisplay = mDoneDecoding || (nextFrame && nextFrame->ImageComplete());

  if (!canDisplay) {
    
    
    return ret;
  }

  
  
  if (mImage->GetNumFrames() == nextFrameIndex) {
    

    
    if (mLoopRemainingCount < 0 && LoopCount() >= 0) {
      mLoopRemainingCount = LoopCount();
    }

    
    
    if (mAnimationMode == imgIContainer::kLoopOnceAnimMode ||
        mLoopRemainingCount == 0) {
      ret.animationFinished = true;
    }

    nextFrameIndex = 0;

    if (mLoopRemainingCount > 0) {
      mLoopRemainingCount--;
    }

    
    if (ret.animationFinished) {
      return ret;
    }
  }

  timeout = GetTimeoutForFrame(nextFrameIndex);

  
  if (timeout < 0) {
    ret.animationFinished = true;
    ret.error = true;
  }

  if (nextFrameIndex == 0) {
    ret.dirtyRect = mFirstFrameRefreshArea;
  } else {
    
    if (nextFrameIndex != currentFrameIndex + 1) {
      nextFrame = GetRawFrame(nextFrameIndex);
    }

    if (!DoBlend(&ret.dirtyRect, currentFrameIndex,
                               nextFrameIndex)) {
      
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
      mCurrentAnimationFrameTime +=
        TimeDuration::FromMilliseconds(loops * loopTime);
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
  mLastCompositedFrameIndex = -1;
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

DrawableFrameRef
FrameAnimator::GetCompositedFrame(uint32_t aFrameNum)
{
  MOZ_ASSERT(aFrameNum != 0, "First frame is never composited");

  
  if (mLastCompositedFrameIndex == int32_t(aFrameNum)) {
    return mCompositingFrame->DrawableRef();
  }

  
  
  DrawableFrameRef ref =
    SurfaceCache::Lookup(ImageKey(mImage),
                         RasterSurfaceKey(mSize,
                                          0,  
                                          aFrameNum));
  MOZ_ASSERT(!ref || !ref->GetIsPaletted(), "About to return a paletted frame");
  return ref;
}

int32_t
FrameAnimator::GetTimeoutForFrame(uint32_t aFrameNum) const
{
  RawAccessFrameRef frame = GetRawFrame(aFrameNum);
  const int32_t timeout = frame->GetRawTimeout();

  
  
  
  
  
  
  
  
  
  
  
  
  
  if (timeout >= 0 && timeout <= 10 && mLoopCount != 0) {
    return 100;
  }

  return timeout;
}

size_t
FrameAnimator::SizeOfCompositingFrames(gfxMemoryLocation aLocation,
                                       MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;

  if (mCompositingFrame) {
    n += mCompositingFrame->SizeOfExcludingThis(aLocation, aMallocSizeOf);
  }
  if (mCompositingPrevFrame) {
    n += mCompositingPrevFrame->SizeOfExcludingThis(aLocation, aMallocSizeOf);
  }

  return n;
}

RawAccessFrameRef
FrameAnimator::GetRawFrame(uint32_t aFrameNum) const
{
  DrawableFrameRef ref =
    SurfaceCache::Lookup(ImageKey(mImage),
                         RasterSurfaceKey(mSize,
                                          0,  
                                          aFrameNum));
  return ref ? ref->RawAccessRef()
             : RawAccessFrameRef();
}




bool
FrameAnimator::DoBlend(nsIntRect* aDirtyRect,
                       uint32_t aPrevFrameIndex,
                       uint32_t aNextFrameIndex)
{
  RawAccessFrameRef prevFrame = GetRawFrame(aPrevFrameIndex);
  RawAccessFrameRef nextFrame = GetRawFrame(aNextFrameIndex);

  MOZ_ASSERT(prevFrame && nextFrame, "Should have frames here");

  DisposalMethod prevFrameDisposalMethod = prevFrame->GetDisposalMethod();
  if (prevFrameDisposalMethod == DisposalMethod::RESTORE_PREVIOUS &&
      !mCompositingPrevFrame) {
    prevFrameDisposalMethod = DisposalMethod::CLEAR;
  }

  nsIntRect prevFrameRect = prevFrame->GetRect();
  bool isFullPrevFrame = (prevFrameRect.x == 0 && prevFrameRect.y == 0 &&
                          prevFrameRect.width == mSize.width &&
                          prevFrameRect.height == mSize.height);

  
  
  if (isFullPrevFrame &&
      (prevFrameDisposalMethod == DisposalMethod::CLEAR)) {
    prevFrameDisposalMethod = DisposalMethod::CLEAR_ALL;
  }

  DisposalMethod nextFrameDisposalMethod = nextFrame->GetDisposalMethod();
  nsIntRect nextFrameRect = nextFrame->GetRect();
  bool isFullNextFrame = (nextFrameRect.x == 0 && nextFrameRect.y == 0 &&
                          nextFrameRect.width == mSize.width &&
                          nextFrameRect.height == mSize.height);

  if (!nextFrame->GetIsPaletted()) {
    
    
    if (prevFrameDisposalMethod == DisposalMethod::CLEAR_ALL) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      return true;
    }

    
    
    if (isFullNextFrame &&
        (nextFrameDisposalMethod != DisposalMethod::RESTORE_PREVIOUS) &&
        !nextFrame->GetHasAlpha()) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      return true;
    }
  }

  
  switch (prevFrameDisposalMethod) {
    default:
    case DisposalMethod::NOT_SPECIFIED:
    case DisposalMethod::KEEP:
      *aDirtyRect = nextFrameRect;
      break;

    case DisposalMethod::CLEAR_ALL:
      
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;

    case DisposalMethod::CLEAR:
      
      
      
      
      
      
      
      aDirtyRect->UnionRect(nextFrameRect, prevFrameRect);
      break;

    case DisposalMethod::RESTORE_PREVIOUS:
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;
  }

  
  
  
  
  
  
  if (mLastCompositedFrameIndex == int32_t(aNextFrameIndex)) {
    return true;
  }

  bool needToBlankComposite = false;

  
  if (!mCompositingFrame) {
    nsRefPtr<imgFrame> newFrame = new imgFrame;
    nsresult rv = newFrame->InitForDecoder(ThebesIntSize(mSize),
                                           SurfaceFormat::B8G8R8A8);
    if (NS_FAILED(rv)) {
      mCompositingFrame.reset();
      return false;
    }
    mCompositingFrame = newFrame->RawAccessRef();
    needToBlankComposite = true;
  } else if (int32_t(aNextFrameIndex) != mLastCompositedFrameIndex+1) {

    
    
    needToBlankComposite = true;
  }

  
  
  
  
  
  
  bool doDisposal = true;
  if (!nextFrame->GetHasAlpha() &&
      nextFrameDisposalMethod != DisposalMethod::RESTORE_PREVIOUS) {
    if (isFullNextFrame) {
      
      
      doDisposal = false;
      
      needToBlankComposite = false;
    } else {
      if ((prevFrameRect.x >= nextFrameRect.x) &&
          (prevFrameRect.y >= nextFrameRect.y) &&
          (prevFrameRect.x + prevFrameRect.width <=
              nextFrameRect.x + nextFrameRect.width) &&
          (prevFrameRect.y + prevFrameRect.height <=
              nextFrameRect.y + nextFrameRect.height)) {
        
        
        doDisposal = false;
      }
    }
  }

  if (doDisposal) {
    
    switch (prevFrameDisposalMethod) {
      case DisposalMethod::CLEAR:
        if (needToBlankComposite) {
          
          
          ClearFrame(mCompositingFrame->GetRawData(),
                     mCompositingFrame->GetRect());
        } else {
          
          ClearFrame(mCompositingFrame->GetRawData(),
                     mCompositingFrame->GetRect(),
                     prevFrameRect);
        }
        break;

      case DisposalMethod::CLEAR_ALL:
        ClearFrame(mCompositingFrame->GetRawData(),
                   mCompositingFrame->GetRect());
        break;

      case DisposalMethod::RESTORE_PREVIOUS:
        
        
        if (mCompositingPrevFrame) {
          CopyFrameImage(mCompositingPrevFrame->GetRawData(),
                         mCompositingPrevFrame->GetRect(),
                         mCompositingFrame->GetRawData(),
                         mCompositingFrame->GetRect());

          
          if (nextFrameDisposalMethod !=
              DisposalMethod::RESTORE_PREVIOUS) {
            mCompositingPrevFrame.reset();
          }
        } else {
          ClearFrame(mCompositingFrame->GetRawData(),
                     mCompositingFrame->GetRect());
        }
        break;

      default:
        
        
        
        
        
        
        
        if (mLastCompositedFrameIndex != int32_t(aNextFrameIndex - 1)) {
          if (isFullPrevFrame && !prevFrame->GetIsPaletted()) {
            
            CopyFrameImage(prevFrame->GetRawData(),
                           prevFrame->GetRect(),
                           mCompositingFrame->GetRawData(),
                           mCompositingFrame->GetRect());
          } else {
            if (needToBlankComposite) {
              
              if (prevFrame->GetHasAlpha() || !isFullPrevFrame) {
                ClearFrame(mCompositingFrame->GetRawData(),
                           mCompositingFrame->GetRect());
              }
            }
            DrawFrameTo(prevFrame->GetRawData(), prevFrameRect,
                        prevFrame->PaletteDataLength(),
                        prevFrame->GetHasAlpha(),
                        mCompositingFrame->GetRawData(),
                        mCompositingFrame->GetRect(),
                        prevFrame->GetBlendMethod());
          }
        }
    }
  } else if (needToBlankComposite) {
    
    
    ClearFrame(mCompositingFrame->GetRawData(),
               mCompositingFrame->GetRect());
  }

  
  
  
  if ((nextFrameDisposalMethod == DisposalMethod::RESTORE_PREVIOUS) &&
      (prevFrameDisposalMethod != DisposalMethod::RESTORE_PREVIOUS)) {
    
    
    
    if (!mCompositingPrevFrame) {
      nsRefPtr<imgFrame> newFrame = new imgFrame;
      nsresult rv = newFrame->InitForDecoder(ThebesIntSize(mSize),
                                             SurfaceFormat::B8G8R8A8);
      if (NS_FAILED(rv)) {
        mCompositingPrevFrame.reset();
        return false;
      }

      mCompositingPrevFrame = newFrame->RawAccessRef();
    }

    CopyFrameImage(mCompositingFrame->GetRawData(),
                   mCompositingFrame->GetRect(),
                   mCompositingPrevFrame->GetRawData(),
                   mCompositingPrevFrame->GetRect());
  }

  
  DrawFrameTo(nextFrame->GetRawData(), nextFrameRect,
              nextFrame->PaletteDataLength(),
              nextFrame->GetHasAlpha(),
              mCompositingFrame->GetRawData(),
              mCompositingFrame->GetRect(),
              nextFrame->GetBlendMethod());

  
  
  int32_t timeout = nextFrame->GetRawTimeout();
  mCompositingFrame->SetRawTimeout(timeout);

  
  nsresult rv =
    mCompositingFrame->ImageUpdated(mCompositingFrame->GetRect());
  if (NS_FAILED(rv)) {
    return false;
  }

  mLastCompositedFrameIndex = int32_t(aNextFrameIndex);

  return true;
}



void
FrameAnimator::ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect)
{
  if (!aFrameData) {
    return;
  }

  memset(aFrameData, 0, aFrameRect.width * aFrameRect.height * 4);
}


void
FrameAnimator::ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect,
                          const nsIntRect& aRectToClear)
{
  if (!aFrameData || aFrameRect.width <= 0 || aFrameRect.height <= 0 ||
      aRectToClear.width <= 0 || aRectToClear.height <= 0) {
    return;
  }

  nsIntRect toClear = aFrameRect.Intersect(aRectToClear);
  if (toClear.IsEmpty()) {
    return;
  }

  uint32_t bytesPerRow = aFrameRect.width * 4;
  for (int row = toClear.y; row < toClear.y + toClear.height; ++row) {
    memset(aFrameData + toClear.x * 4 + row * bytesPerRow, 0,
           toClear.width * 4);
  }
}




bool
FrameAnimator::CopyFrameImage(const uint8_t* aDataSrc, const nsIntRect& aRectSrc,
                              uint8_t* aDataDest, const nsIntRect& aRectDest)
{
  uint32_t dataLengthSrc = aRectSrc.width * aRectSrc.height * 4;
  uint32_t dataLengthDest = aRectDest.width * aRectDest.height * 4;

  if (!aDataDest || !aDataSrc || dataLengthSrc != dataLengthDest) {
    return false;
  }

  memcpy(aDataDest, aDataSrc, dataLengthDest);

  return true;
}

nsresult
FrameAnimator::DrawFrameTo(const uint8_t* aSrcData, const nsIntRect& aSrcRect,
                           uint32_t aSrcPaletteLength, bool aSrcHasAlpha,
                           uint8_t* aDstPixels, const nsIntRect& aDstRect,
                           BlendMethod aBlendMethod)
{
  NS_ENSURE_ARG_POINTER(aSrcData);
  NS_ENSURE_ARG_POINTER(aDstPixels);

  
  if (aSrcRect.x < 0 || aSrcRect.y < 0) {
    NS_WARNING("FrameAnimator::DrawFrameTo: negative offsets not allowed");
    return NS_ERROR_FAILURE;
  }
  
  if ((aSrcRect.x > aDstRect.width) || (aSrcRect.y > aDstRect.height)) {
    return NS_OK;
  }

  if (aSrcPaletteLength) {
    
    int32_t width = std::min(aSrcRect.width, aDstRect.width - aSrcRect.x);
    int32_t height = std::min(aSrcRect.height, aDstRect.height - aSrcRect.y);

    
    NS_ASSERTION((aSrcRect.x >= 0) && (aSrcRect.y >= 0) &&
                 (aSrcRect.x + width <= aDstRect.width) &&
                 (aSrcRect.y + height <= aDstRect.height),
                "FrameAnimator::DrawFrameTo: Invalid aSrcRect");

    
    NS_ASSERTION((width <= aSrcRect.width) && (height <= aSrcRect.height),
                 "FrameAnimator::DrawFrameTo: source must be smaller than dest");

    
    const uint8_t* srcPixels = aSrcData + aSrcPaletteLength;
    uint32_t* dstPixels = reinterpret_cast<uint32_t*>(aDstPixels);
    const uint32_t* colormap = reinterpret_cast<const uint32_t*>(aSrcData);

    
    dstPixels += aSrcRect.x + (aSrcRect.y * aDstRect.width);
    if (!aSrcHasAlpha) {
      for (int32_t r = height; r > 0; --r) {
        for (int32_t c = 0; c < width; c++) {
          dstPixels[c] = colormap[srcPixels[c]];
        }
        
        srcPixels += aSrcRect.width;
        dstPixels += aDstRect.width;
      }
    } else {
      for (int32_t r = height; r > 0; --r) {
        for (int32_t c = 0; c < width; c++) {
          const uint32_t color = colormap[srcPixels[c]];
          if (color) {
            dstPixels[c] = color;
          }
        }
        
        srcPixels += aSrcRect.width;
        dstPixels += aDstRect.width;
      }
    }
  } else {
    pixman_image_t* src =
      pixman_image_create_bits(
          aSrcHasAlpha ? PIXMAN_a8r8g8b8 : PIXMAN_x8r8g8b8,
          aSrcRect.width, aSrcRect.height,
          reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(aSrcData)),
          aSrcRect.width * 4);
    pixman_image_t* dst =
      pixman_image_create_bits(PIXMAN_a8r8g8b8,
                               aDstRect.width,
                               aDstRect.height,
                               reinterpret_cast<uint32_t*>(aDstPixels),
                               aDstRect.width * 4);

    auto op = aBlendMethod == BlendMethod::SOURCE ? PIXMAN_OP_SRC
                                                  : PIXMAN_OP_OVER;
    pixman_image_composite32(op,
                             src,
                             nullptr,
                             dst,
                             0, 0,
                             0, 0,
                             aSrcRect.x, aSrcRect.y,
                             aSrcRect.width, aSrcRect.height);

    pixman_image_unref(src);
    pixman_image_unref(dst);
  }

  return NS_OK;
}

} 
} 
