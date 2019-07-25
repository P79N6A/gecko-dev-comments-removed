





#include "VideoFrameContainer.h"

#include "nsHTMLMediaElement.h"
#include "nsIFrame.h"
#include "nsDisplayList.h"
#include "nsSVGEffects.h"

using namespace mozilla::layers;

namespace mozilla {

void VideoFrameContainer::SetCurrentFrame(const gfxIntSize& aIntrinsicSize,
                                          Image* aImage,
                                          TimeStamp aTargetTime)
{
  MutexAutoLock lock(mMutex);

  if (aIntrinsicSize != mIntrinsicSize) {
    mIntrinsicSize = aIntrinsicSize;
    mIntrinsicSizeChanged = true;
  }

  gfxIntSize oldFrameSize = mImageContainer->GetCurrentSize();
  TimeStamp lastPaintTime = mImageContainer->GetPaintTime();
  if (!lastPaintTime.IsNull() && !mPaintTarget.IsNull()) {
    mPaintDelay = lastPaintTime - mPaintTarget;
  }
  mImageContainer->SetCurrentImage(aImage);
  gfxIntSize newFrameSize = mImageContainer->GetCurrentSize();
  if (oldFrameSize != newFrameSize) {
    mImageSizeChanged = true;
  }

  mPaintTarget = aTargetTime;
}

double VideoFrameContainer::GetFrameDelay()
{
  MutexAutoLock lock(mMutex);
  return mPaintDelay.ToSeconds();
}

void VideoFrameContainer::Invalidate()
{
  NS_ASSERTION(NS_IsMainThread(), "Must call on main thread");

  if (!mNeedInvalidation) {
    return;
  }
  if (mImageContainer && mImageContainer->IsAsync()) {
    mNeedInvalidation = false;
  }

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

  if (frame) {
    nsRect contentRect = frame->GetContentRect() - frame->GetPosition();
    if (invalidateFrame) {
      frame->Invalidate(contentRect);
    } else {
      frame->InvalidateLayer(contentRect, nsDisplayItem::TYPE_VIDEO);
    }
  }

  nsSVGEffects::InvalidateDirectRenderingObservers(mElement);
}

}
