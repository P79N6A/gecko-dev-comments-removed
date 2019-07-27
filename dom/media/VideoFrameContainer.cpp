





#include "VideoFrameContainer.h"

#include "mozilla/dom/HTMLMediaElement.h"
#include "nsIFrame.h"
#include "nsDisplayList.h"
#include "nsSVGEffects.h"
#include "ImageContainer.h"

using namespace mozilla::layers;

namespace mozilla {

VideoFrameContainer::VideoFrameContainer(dom::HTMLMediaElement* aElement,
                                         already_AddRefed<ImageContainer> aContainer)
  : mElement(aElement),
    mImageContainer(aContainer), mMutex("nsVideoFrameContainer"),
    mIntrinsicSizeChanged(false), mImageSizeChanged(false)
{
  NS_ASSERTION(aElement, "aElement must not be null");
  NS_ASSERTION(mImageContainer, "aContainer must not be null");
}

VideoFrameContainer::~VideoFrameContainer()
{}

void VideoFrameContainer::SetCurrentFrame(const gfxIntSize& aIntrinsicSize,
                                          Image* aImage,
                                          TimeStamp aTargetTime)
{
  if (!aImage->IsValid()) {
    return;
  }

  MutexAutoLock lock(mMutex);

  if (aIntrinsicSize != mIntrinsicSize) {
    mIntrinsicSize = aIntrinsicSize;
    mIntrinsicSizeChanged = true;
  }

  gfx::IntSize oldFrameSize = mImageContainer->GetCurrentSize();
  TimeStamp lastPaintTime = mImageContainer->GetPaintTime();
  if (!lastPaintTime.IsNull() && !mPaintTarget.IsNull()) {
    mPaintDelay = lastPaintTime - mPaintTarget;
  }

  
  
  
  
  
  
  nsRefPtr<Image> kungFuDeathGrip;
  kungFuDeathGrip = mImageContainer->LockCurrentImage();
  mImageContainer->UnlockCurrentImage();

  mImageContainer->SetCurrentImage(aImage);
  gfx::IntSize newFrameSize = mImageContainer->GetCurrentSize();
  if (oldFrameSize != newFrameSize) {
    mImageSizeChanged = true;
  }

  mPaintTarget = aTargetTime;
}

void VideoFrameContainer::Reset()
{
  ClearCurrentFrame(true);
  Invalidate();
  mIntrinsicSize = gfxIntSize(-1, -1);
  mPaintDelay = mozilla::TimeDuration();
  mPaintTarget = mozilla::TimeStamp();
  mImageContainer->ResetPaintCount();
}

void VideoFrameContainer::ClearCurrentFrame(bool aResetSize)
{
  MutexAutoLock lock(mMutex);

  
  
  nsRefPtr<Image> kungFuDeathGrip;
  kungFuDeathGrip = mImageContainer->LockCurrentImage();
  mImageContainer->UnlockCurrentImage();

  mImageContainer->ClearAllImages();
  mImageSizeChanged = aResetSize;
}

ImageContainer* VideoFrameContainer::GetImageContainer() {
  return mImageContainer;
}


double VideoFrameContainer::GetFrameDelay()
{
  MutexAutoLock lock(mMutex);
  return mPaintDelay.ToSeconds();
}

void VideoFrameContainer::InvalidateWithFlags(uint32_t aFlags)
{
  NS_ASSERTION(NS_IsMainThread(), "Must call on main thread");

  if (!mElement) {
    
    return;
  }

  nsIFrame* frame = mElement->GetPrimaryFrame();
  bool invalidateFrame = false;

  {
    MutexAutoLock lock(mMutex);

    
    invalidateFrame = mImageSizeChanged;
    mImageSizeChanged = false;

    if (mIntrinsicSizeChanged) {
      mElement->UpdateMediaSize(mIntrinsicSize);
      mIntrinsicSizeChanged = false;

      if (frame) {
        nsPresContext* presContext = frame->PresContext();
        nsIPresShell *presShell = presContext->PresShell();
        presShell->FrameNeedsReflow(frame,
                                    nsIPresShell::eStyleChange,
                                    NS_FRAME_IS_DIRTY);
      }
    }
  }

  bool asyncInvalidate = mImageContainer &&
                         mImageContainer->IsAsync() &&
                         !(aFlags & INVALIDATE_FORCE);

  if (frame) {
    if (invalidateFrame) {
      frame->InvalidateFrame();
    } else {
      frame->InvalidateLayer(nsDisplayItem::TYPE_VIDEO, nullptr, nullptr,
                             asyncInvalidate ? nsIFrame::UPDATE_IS_ASYNC : 0);
    }
  }

  nsSVGEffects::InvalidateDirectRenderingObservers(mElement);
}

}
