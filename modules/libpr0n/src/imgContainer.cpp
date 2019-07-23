









































#include "nsComponentManagerUtils.h"
#include "imgIContainerObserver.h"
#include "nsIImage.h"
#include "imgContainer.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAutoPtr.h"

#include "gfxContext.h"

NS_IMPL_ISUPPORTS3(imgContainer, imgIContainer, nsITimerCallback, nsIProperties)


imgContainer::imgContainer() :
  mSize(0,0),
  mAnim(nsnull),
  mAnimationMode(kNormalAnimMode),
  mLoopCount(-1),
  mObserver(nsnull)
{
  mProperties = do_CreateInstance("@mozilla.org/properties;1");
}


imgContainer::~imgContainer()
{
  if (mAnim)
    delete mAnim;
}




NS_IMETHODIMP imgContainer::Init(PRInt32 aWidth, PRInt32 aHeight,
                                 imgIContainerObserver *aObserver)
{
  if (aWidth <= 0 || aHeight <= 0) {
    NS_WARNING("error - negative image size\n");
    return NS_ERROR_FAILURE;
  }

  mSize.SizeTo(aWidth, aHeight);
  
  mObserver = do_GetWeakReference(aObserver);
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetPreferredAlphaChannelFormat(gfx_format *aFormat)
{
  NS_ASSERTION(aFormat, "imgContainer::GetPreferredAlphaChannelFormat; Invalid Arg");
  if (!aFormat)
    return NS_ERROR_INVALID_ARG;

  
  *aFormat = gfxIFormats::RGB_A8;
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetWidth(PRInt32 *aWidth)
{
  NS_ASSERTION(aWidth, "imgContainer::GetWidth; Invalid Arg");
  if (!aWidth)
    return NS_ERROR_INVALID_ARG;

  *aWidth = mSize.width;
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetHeight(PRInt32 *aHeight)
{
  NS_ASSERTION(aHeight, "imgContainer::GetHeight; Invalid Arg");
  if (!aHeight)
    return NS_ERROR_INVALID_ARG;

  *aHeight = mSize.height;
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetCurrentFrame(gfxIImageFrame **aCurrentFrame)
{
  NS_ASSERTION(aCurrentFrame, "imgContainer::GetCurrentFrame; Invalid Arg");
  if (!aCurrentFrame)
    return NS_ERROR_INVALID_POINTER;
  
  if (!(*aCurrentFrame = inlinedGetCurrentFrame()))
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aCurrentFrame);
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetNumFrames(PRUint32 *aNumFrames)
{
  NS_ASSERTION(aNumFrames, "imgContainer::GetNumFrames; Invalid Arg");
  if (!aNumFrames)
    return NS_ERROR_INVALID_ARG;

  *aNumFrames = mFrames.Count();
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetFrameAt(PRUint32 index, gfxIImageFrame **_retval)
{
  NS_ENSURE_ARG(index < NS_STATIC_CAST(PRUint32, mFrames.Count()));
  
  NS_ASSERTION(_retval, "imgContainer::GetFrameAt; Invalid Arg");
  if (!_retval)
    return NS_ERROR_INVALID_POINTER;

  if (!(*_retval = mFrames[index]))
    return NS_ERROR_FAILURE;

  NS_ADDREF(*_retval);
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::AppendFrame(gfxIImageFrame *item)
{
  NS_ASSERTION(item, "imgContainer::AppendFrame; Invalid Arg");
  if (!item)
    return NS_ERROR_INVALID_ARG;
  
  PRInt32 numFrames = mFrames.Count();
  
  if (numFrames == 0) {
    
    mFrames.AppendObject(item);
    return NS_OK;
  }
  
  if (numFrames == 1) {
    
    if (!ensureAnimExists())
      return NS_ERROR_OUT_OF_MEMORY;
    
    
    
    
    PRInt32 frameDisposalMethod;
    mFrames[0]->GetFrameDisposalMethod(&frameDisposalMethod);
    if (frameDisposalMethod == imgIContainer::kDisposeClear ||
        frameDisposalMethod == imgIContainer::kDisposeRestorePrevious)
      mFrames[0]->GetRect(mAnim->firstFrameRefreshArea);
  }
  
  
  
  
  nsIntRect itemRect;
  item->GetRect(itemRect);
  mAnim->firstFrameRefreshArea.UnionRect(mAnim->firstFrameRefreshArea, 
                                         itemRect);
  
  mFrames.AppendObject(item);
  
  
  
  
  if (numFrames == 1)
    StartAnimation();
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::RemoveFrame(gfxIImageFrame *item)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP imgContainer::EndFrameDecode(PRUint32 aFrameNum, PRUint32 aTimeout)
{
  
  
  if (mAnim)
    mAnim->currentDecodingFrameIndex = aFrameNum;
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::DecodingComplete(void)
{
  if (mAnim)
    mAnim->doneDecoding = PR_TRUE;
  
  
  if (mFrames.Count() == 1)
    mFrames[0]->SetMutable(PR_FALSE);
  return NS_OK;
}



NS_IMETHODIMP imgContainer::Clear()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP imgContainer::GetAnimationMode(PRUint16 *aAnimationMode)
{
  NS_ASSERTION(aAnimationMode, "imgContainer::GetAnimationMode; Invalid Arg");
  if (!aAnimationMode)
    return NS_ERROR_INVALID_ARG;
  
  *aAnimationMode = mAnimationMode;
  return NS_OK;
}



NS_IMETHODIMP imgContainer::SetAnimationMode(PRUint16 aAnimationMode)
{
  NS_ASSERTION(aAnimationMode == imgIContainer::kNormalAnimMode ||
               aAnimationMode == imgIContainer::kDontAnimMode ||
               aAnimationMode == imgIContainer::kLoopOnceAnimMode,
               "Wrong Animation Mode is being set!");
  
  switch (mAnimationMode = aAnimationMode) {
    case kDontAnimMode:
      StopAnimation();
      break;
    case kNormalAnimMode:
      if (mLoopCount != 0 || 
          (mAnim && (mAnim->currentAnimationFrameIndex + 1 < mFrames.Count())))
        StartAnimation();
      break;
    case kLoopOnceAnimMode:
      if (mAnim && (mAnim->currentAnimationFrameIndex + 1 < mFrames.Count()))
        StartAnimation();
      break;
  }
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::StartAnimation()
{
  if (mAnimationMode == kDontAnimMode || 
      (mAnim && (mAnim->timer || mAnim->animating)))
    return NS_OK;
  
  if (mFrames.Count() > 1) {
    if (!ensureAnimExists())
      return NS_ERROR_OUT_OF_MEMORY;
    
    PRInt32 timeout;
    gfxIImageFrame *currentFrame = inlinedGetCurrentFrame();
    if (currentFrame) {
      currentFrame->GetTimeout(&timeout);
      if (timeout <= 0) 
        return NS_OK;
    } else
      timeout = 100; 
                     
    
    mAnim->timer = do_CreateInstance("@mozilla.org/timer;1");
    if (!mAnim->timer)
      return NS_ERROR_OUT_OF_MEMORY;
    
    
    mAnim->animating = PR_TRUE;
    mAnim->timer->InitWithCallback(NS_STATIC_CAST(nsITimerCallback*, this),
                                   timeout, nsITimer::TYPE_REPEATING_SLACK);
  }
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::StopAnimation()
{
  if (mAnim) {
    mAnim->animating = PR_FALSE;

    if (!mAnim->timer)
      return NS_OK;

    mAnim->timer->Cancel();
    mAnim->timer = nsnull;
  }

  return NS_OK;
}



NS_IMETHODIMP imgContainer::ResetAnimation()
{
  if (mAnimationMode == kDontAnimMode || 
      !mAnim || mAnim->currentAnimationFrameIndex)
    return NS_OK;

  PRBool oldAnimating = mAnim->animating;

  if (mAnim->animating) {
    nsresult rv = StopAnimation();
    if (NS_FAILED(rv))
      return rv;
  }

  mAnim->lastCompositedFrameIndex = -1;
  mAnim->currentAnimationFrameIndex = 0;
  
  nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));
  if (observer)
    observer->FrameChanged(this, mFrames[0], &(mAnim->firstFrameRefreshArea));

  if (oldAnimating)
    return StartAnimation();
  else
    return NS_OK;
}



NS_IMETHODIMP imgContainer::GetLoopCount(PRInt32 *aLoopCount)
{
  NS_ASSERTION(aLoopCount, "imgContainer::GetLoopCount() called with null ptr");
  if (!aLoopCount)
    return NS_ERROR_INVALID_ARG;
  
  *aLoopCount = mLoopCount;
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::SetLoopCount(PRInt32 aLoopCount)
{
  
  
  
  
  mLoopCount = aLoopCount;

  return NS_OK;
}



NS_IMETHODIMP imgContainer::Notify(nsITimer *timer)
{
  
  
  NS_ASSERTION(mAnim, "imgContainer::Notify() called but mAnim is null");
  if (!mAnim)
    return NS_ERROR_UNEXPECTED;
  NS_ASSERTION(mAnim->timer == timer,
               "imgContainer::Notify() called with incorrect timer");

  if (!(mAnim->animating) || !(mAnim->timer))
    return NS_OK;

  nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));
  if (!observer) {
    
    StopAnimation();
    return NS_OK;
  }

  PRInt32 numFrames = mFrames.Count();
  if (!numFrames)
    return NS_OK;
  
  gfxIImageFrame *nextFrame = nsnull;
  PRInt32 previousFrameIndex = mAnim->currentAnimationFrameIndex;
  PRInt32 nextFrameIndex = mAnim->currentAnimationFrameIndex + 1;
  PRInt32 timeout = 0;

  
  
  
  
  if (mAnim->doneDecoding || 
      (numFrames == 2 && nextFrameIndex < 2) ||
      (numFrames > 2 && nextFrameIndex < mAnim->currentDecodingFrameIndex)) {
    if (numFrames == nextFrameIndex) {
      

      
      if (mAnimationMode == kLoopOnceAnimMode || mLoopCount == 0) {
        StopAnimation();
        return NS_OK;
      } else {
        
        
        if (mAnim->compositingFrame && mAnim->lastCompositedFrameIndex == -1)
          mAnim->compositingFrame = nsnull;
      }

      nextFrameIndex = 0;
      if (mLoopCount > 0)
        mLoopCount--;
    }

    if (!(nextFrame = mFrames[nextFrameIndex])) {
      
      mAnim->currentAnimationFrameIndex = nextFrameIndex;
      mAnim->timer->SetDelay(100);
      return NS_OK;
    }
    nextFrame->GetTimeout(&timeout);

  } else if ((numFrames == 2 && nextFrameIndex == 2) ||
             (numFrames > 2 && nextFrameIndex == mAnim->currentDecodingFrameIndex)) {
    
    
    mAnim->timer->SetDelay(100);
    return NS_OK;
  } else { 
    
    
    NS_WARNING("imgContainer::Notify()  Frame is passed decoded frame");
    nextFrameIndex = mAnim->currentDecodingFrameIndex;
    if (!(nextFrame = mFrames[nextFrameIndex])) {
      
      mAnim->currentAnimationFrameIndex = nextFrameIndex;
      mAnim->timer->SetDelay(100);
      return NS_OK;
    }
    nextFrame->GetTimeout(&timeout);
  }

  if (timeout > 0)
    mAnim->timer->SetDelay(timeout);
  else
    StopAnimation();

  nsIntRect dirtyRect;
  gfxIImageFrame *frameToUse = nsnull;

  if (nextFrameIndex == 0) {
    frameToUse = nextFrame;
    dirtyRect = mAnim->firstFrameRefreshArea;
  } else {
    gfxIImageFrame *prevFrame = mFrames[previousFrameIndex];
    if (!prevFrame)
      return NS_OK;

    
    if (NS_FAILED(DoComposite(&frameToUse, &dirtyRect, prevFrame,
                              nextFrame, nextFrameIndex))) {
      
      NS_WARNING("imgContainer::Notify(): Composing Frame Failed\n");
      mAnim->currentAnimationFrameIndex = nextFrameIndex;
      return NS_OK;
    }
  }
  
  mAnim->currentAnimationFrameIndex = nextFrameIndex;
  
  observer->FrameChanged(this, frameToUse, &dirtyRect);
  
  return NS_OK;
}




nsresult imgContainer::DoComposite(gfxIImageFrame** aFrameToUse,
                                   nsIntRect* aDirtyRect,
                                   gfxIImageFrame* aPrevFrame,
                                   gfxIImageFrame* aNextFrame,
                                   PRInt32 aNextFrameIndex)
{
  NS_ASSERTION(aDirtyRect, "imgContainer::DoComposite aDirtyRect is null");
  NS_ASSERTION(aPrevFrame, "imgContainer::DoComposite aPrevFrame is null");
  NS_ASSERTION(aNextFrame, "imgContainer::DoComposite aNextFrame is null");
  NS_ASSERTION(aFrameToUse, "imgContainer::DoComposite aFrameToUse is null");
  
  PRInt32 prevFrameDisposalMethod;
  aPrevFrame->GetFrameDisposalMethod(&prevFrameDisposalMethod);

  if (prevFrameDisposalMethod == imgIContainer::kDisposeRestorePrevious &&
      !mAnim->compositingPrevFrame)
    prevFrameDisposalMethod = imgIContainer::kDisposeClear;

  
  
  if (prevFrameDisposalMethod == imgIContainer::kDisposeClearAll) {
    aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
    *aFrameToUse = aNextFrame;
    return NS_OK;
  }

  nsIntRect prevFrameRect;
  aPrevFrame->GetRect(prevFrameRect);
  PRBool isFullPrevFrame = (prevFrameRect.x == 0 && prevFrameRect.y == 0 &&
                            prevFrameRect.width == mSize.width &&
                            prevFrameRect.height == mSize.height);

  
  
  if (isFullPrevFrame && prevFrameDisposalMethod == imgIContainer::kDisposeClear) {
    aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
    *aFrameToUse = aNextFrame;
    return NS_OK;
  }

  PRInt32 nextFrameDisposalMethod;
  nsIntRect nextFrameRect;
  aNextFrame->GetFrameDisposalMethod(&nextFrameDisposalMethod);
  aNextFrame->GetRect(nextFrameRect);
  PRBool isFullNextFrame = (nextFrameRect.x == 0 && nextFrameRect.y == 0 &&
                            nextFrameRect.width == mSize.width &&
                            nextFrameRect.height == mSize.height);

  PRBool nextFrameHasAlpha;
  PRUint32 aBPR;
  nextFrameHasAlpha = NS_SUCCEEDED(aNextFrame->GetAlphaBytesPerRow(&aBPR));

  
  
  if (isFullNextFrame &&
      (nextFrameDisposalMethod != imgIContainer::kDisposeRestorePrevious) &&
      !nextFrameHasAlpha) {

    aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
    *aFrameToUse = aNextFrame;
    return NS_OK;
  }

  
  switch (prevFrameDisposalMethod) {
    default:
    case imgIContainer::kDisposeNotSpecified:
    case imgIContainer::kDisposeKeep:
      *aDirtyRect = nextFrameRect;
      break;

    case imgIContainer::kDisposeClear:
      
      
      
      
      
      
      
      aDirtyRect->UnionRect(nextFrameRect, prevFrameRect);
      break;

    case imgIContainer::kDisposeRestorePrevious:
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;
  }

  
  
  
  
  
  
  if (mAnim->lastCompositedFrameIndex == aNextFrameIndex) {
    *aFrameToUse = mAnim->compositingFrame;
    return NS_OK;
  }

  PRBool needToBlankComposite = PR_FALSE;

  
  if (!mAnim->compositingFrame) {
    nsresult rv;
    mAnim->compositingFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2", &rv);
    if (NS_FAILED(rv))
      return rv;
    rv = mAnim->compositingFrame->Init(0, 0, mSize.width, mSize.height,
                                       gfxIFormats::RGB_A1, 24);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to init compositingFrame!\n");
      mAnim->compositingFrame = nsnull;
      return rv;
    }
    needToBlankComposite = PR_TRUE;
  }

  
  
  
  
  
  
  if (mAnim->lastCompositedFrameIndex != aNextFrameIndex - 1 &&
      prevFrameDisposalMethod != imgIContainer::kDisposeRestorePrevious) {

    
    
    
    
    if (isFullPrevFrame) {
      CopyFrameImage(aPrevFrame, mAnim->compositingFrame);
    } else {
      BlackenFrame(mAnim->compositingFrame);
      SetMaskVisibility(mAnim->compositingFrame, PR_FALSE);
      aPrevFrame->DrawTo(mAnim->compositingFrame, prevFrameRect.x, prevFrameRect.y,
                         prevFrameRect.width, prevFrameRect.height);

      BuildCompositeMask(mAnim->compositingFrame, aPrevFrame);
      needToBlankComposite = PR_FALSE;
    }
  }

  
  switch (prevFrameDisposalMethod) {
    case imgIContainer::kDisposeClear:
      if (needToBlankComposite) {
        
        
        BlackenFrame(mAnim->compositingFrame);
        SetMaskVisibility(mAnim->compositingFrame, PR_FALSE);
        needToBlankComposite = PR_FALSE;
      } else {
        
        BlackenFrame(mAnim->compositingFrame, prevFrameRect);
        SetMaskVisibility(mAnim->compositingFrame, prevFrameRect, PR_FALSE);
      }
      break;

    case imgIContainer::kDisposeRestorePrevious:
      
      
      if (mAnim->compositingPrevFrame) {
        CopyFrameImage(mAnim->compositingPrevFrame, mAnim->compositingFrame);

        
        if (nextFrameDisposalMethod != imgIContainer::kDisposeRestorePrevious)
          mAnim->compositingPrevFrame = nsnull;
      } else {
        BlackenFrame(mAnim->compositingFrame);
        SetMaskVisibility(mAnim->compositingFrame, PR_FALSE);
      }
      break;
  }

  
  
  
  if ((nextFrameDisposalMethod == imgIContainer::kDisposeRestorePrevious) &&
      (prevFrameDisposalMethod != imgIContainer::kDisposeRestorePrevious)) {
    
    
    
    if (!mAnim->compositingPrevFrame) {
      nsresult rv;
      mAnim->compositingPrevFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2",
                                                       &rv);
      if (NS_FAILED(rv))
        return rv;
      rv = mAnim->compositingPrevFrame->Init(0, 0, mSize.width, mSize.height,
                                              gfxIFormats::RGB_A1, 24);
      if (NS_FAILED(rv))
        return rv;
    }
    CopyFrameImage(mAnim->compositingFrame, mAnim->compositingPrevFrame);
  }

  
  aNextFrame->DrawTo(mAnim->compositingFrame, nextFrameRect.x, nextFrameRect.y,
                     nextFrameRect.width, nextFrameRect.height);
  
  BuildCompositeMask(mAnim->compositingFrame, aNextFrame);
  
  
  PRInt32 timeout;
  aNextFrame->GetTimeout(&timeout);
  mAnim->compositingFrame->SetTimeout(timeout);

  if (isFullNextFrame && mAnimationMode == kNormalAnimMode && mLoopCount != 0) {
    
    
    
    
    
    if (CopyFrameImage(mAnim->compositingFrame, aNextFrame)) {
      aPrevFrame->SetFrameDisposalMethod(imgIContainer::kDisposeClearAll);
      mAnim->lastCompositedFrameIndex = -1;
      *aFrameToUse = aNextFrame;
      return NS_OK;
    }
  }

  mAnim->lastCompositedFrameIndex = aNextFrameIndex;
  *aFrameToUse = mAnim->compositingFrame;

  return NS_OK;
}


void imgContainer::BuildCompositeMask(gfxIImageFrame *aCompositingFrame,
                                      gfxIImageFrame *aOverlayFrame)
{
  if (!aCompositingFrame || !aOverlayFrame) return;

  nsresult res;
  PRUint8* compositingAlphaData;
  PRUint32 compositingAlphaDataLength;
  aCompositingFrame->LockAlphaData();
  res = aCompositingFrame->GetAlphaData(&compositingAlphaData,
                                        &compositingAlphaDataLength);
  if (!compositingAlphaData || !compositingAlphaDataLength || NS_FAILED(res)) {
    aCompositingFrame->UnlockAlphaData();
    return;
  }

  PRInt32 widthOverlay, heightOverlay;
  PRInt32 overlayXOffset, overlayYOffset;
  aOverlayFrame->GetWidth(&widthOverlay);
  aOverlayFrame->GetHeight(&heightOverlay);
  aOverlayFrame->GetX(&overlayXOffset);
  aOverlayFrame->GetY(&overlayYOffset);

  if (NS_FAILED(aOverlayFrame->LockAlphaData())) {
    
    SetMaskVisibility(aCompositingFrame, overlayXOffset, overlayYOffset,
                      widthOverlay, heightOverlay, PR_TRUE);
    aCompositingFrame->UnlockAlphaData();
    return;
  }

  PRUint32 abprComposite;
  aCompositingFrame->GetAlphaBytesPerRow(&abprComposite);

  PRUint32 abprOverlay;
  aOverlayFrame->GetAlphaBytesPerRow(&abprOverlay);

  
  PRInt32 widthComposite, heightComposite;
  aCompositingFrame->GetWidth(&widthComposite);
  aCompositingFrame->GetHeight(&heightComposite);

  PRUint8* overlayAlphaData;
  PRUint32 overlayAlphaDataLength;
  res = aOverlayFrame->GetAlphaData(&overlayAlphaData, &overlayAlphaDataLength);

  
  
  
  
  
  
    
    
    
    
  

  
  if (widthComposite <= overlayXOffset || heightComposite <= overlayYOffset)
    return;

  const PRUint32 width  = PR_MIN(widthOverlay,
                                 widthComposite - overlayXOffset);
  const PRUint32 height = PR_MIN(heightOverlay,
                                 heightComposite - overlayYOffset);

#ifdef MOZ_PLATFORM_IMAGES_BOTTOM_TO_TOP
  
  PRInt32 offset = ((heightComposite - 1) - overlayYOffset) * abprComposite;
#else
  PRInt32 offset = overlayYOffset * abprComposite;
#endif
  PRUint8* alphaLine = compositingAlphaData + offset + (overlayXOffset >> 3);

#ifdef MOZ_PLATFORM_IMAGES_BOTTOM_TO_TOP
  offset = (heightOverlay - 1) * abprOverlay;
#else
  offset = 0;
#endif
  PRUint8* overlayLine = overlayAlphaData + offset;

  









  PRUint8 mask_offset = (overlayXOffset & 0x7);

  for(PRUint32 i = 0; i < height; i++) {
    PRUint8 pixels;
    PRUint32 j;
    
    
    
    
    PRUint8 *localOverlay = overlayLine;
    PRUint8 *localAlpha   = alphaLine;

    for (j = width; j >= 8; j -= 8) {
      
      pixels = *localOverlay++;

      if (pixels == 0) 
        localAlpha++;
      else {
        
        if (mask_offset == 0) 
          *localAlpha++ |= pixels;
        else {
          *localAlpha++ |= (pixels >> mask_offset);
          *localAlpha   |= (pixels << (8U-mask_offset));
        }
      }
    }
    if (j != 0) {
      
      pixels = *localOverlay++;
      if (pixels != 0) {
        
        

        
        pixels = (pixels >> (8U-j)) << (8U-j);
        *localAlpha++ |= (pixels >> mask_offset);
        
        if (j > (8U - mask_offset))
          *localAlpha |= (pixels << (8U-mask_offset));
      }
    }

#ifdef MOZ_PLATFORM_IMAGES_BOTTOM_TO_TOP
    alphaLine   -= abprComposite;
    overlayLine -= abprOverlay;
#else
    alphaLine   += abprComposite;
    overlayLine += abprOverlay;
#endif
  }

  aCompositingFrame->UnlockAlphaData();
  aOverlayFrame->UnlockAlphaData();
  return;
}


void imgContainer::SetMaskVisibility(gfxIImageFrame *aFrame,
                                     PRInt32 aX, PRInt32 aY,
                                     PRInt32 aWidth, PRInt32 aHeight,
                                     PRBool aVisible)
{
  if (!aFrame)
    return;

  PRInt32 frameWidth;
  PRInt32 frameHeight;
  aFrame->GetWidth(&frameWidth);
  aFrame->GetHeight(&frameHeight);

  const PRInt32 width  = PR_MIN(aWidth, frameWidth - aX);
  const PRInt32 height = PR_MIN(aHeight, frameHeight - aY);

  if (width <= 0 || height <= 0) {
    return;
  }

  PRUint8* alphaData;
  PRUint32 alphaDataLength;
  const PRUint8 setMaskTo = aVisible ? 0xFF : 0x00;

  aFrame->LockImageData();
  nsresult res = aFrame->GetImageData(&alphaData, &alphaDataLength);
  if (NS_SUCCEEDED(res)) {
#ifdef IS_LITTLE_ENDIAN
    alphaData += aY*frameWidth*4 + 3;
#else
    alphaData += aY*frameWidth*4;
#endif
    for (PRInt32 j = height; j > 0; --j) {
      for (PRInt32 i = (aX+width-1)*4; i >= aX; i -= 4) {
        alphaData[i] = setMaskTo;
      }
      alphaData += frameWidth*4;
    }
  }
  aFrame->UnlockImageData();
}


void imgContainer::SetMaskVisibility(gfxIImageFrame *aFrame, PRBool aVisible)
{
  if (!aFrame)
    return;

  PRUint8* alphaData;
  PRUint32 alphaDataLength;
  const PRUint8 setMaskTo = aVisible ? 0xFF : 0x00;

  aFrame->LockImageData();
  nsresult res = aFrame->GetImageData(&alphaData, &alphaDataLength);
  if (NS_SUCCEEDED(res)) {
    for (PRUint32 i = 0; i < alphaDataLength; i+=4) {
#ifdef IS_LITTLE_ENDIAN
      alphaData[i+3] = setMaskTo;
#else
      alphaData[i] = setMaskTo;
#endif
    }
  }
  aFrame->UnlockImageData();
}



void imgContainer::BlackenFrame(gfxIImageFrame *aFrame)
{
  if (!aFrame)
    return;

  PRInt32 widthFrame;
  PRInt32 heightFrame;
  aFrame->GetWidth(&widthFrame);
  aFrame->GetHeight(&heightFrame);

  BlackenFrame(aFrame, 0, 0, widthFrame, heightFrame);
}


void imgContainer::BlackenFrame(gfxIImageFrame *aFrame,
                                   PRInt32 aX, PRInt32 aY,
                                   PRInt32 aWidth, PRInt32 aHeight)
{
  if (!aFrame)
    return;

  nsCOMPtr<nsIImage> img(do_GetInterface(aFrame));
  if (!img)
    return;

  nsRefPtr<gfxASurface> surf;
  img->GetSurface(getter_AddRefs(surf));

  nsRefPtr<gfxContext> ctx = new gfxContext(surf);
  ctx->SetColor(gfxRGBA(0, 0, 0));
  ctx->Rectangle(gfxRect(aX, aY, aWidth, aHeight));
  ctx->Fill();

  nsIntRect r(aX, aY, aWidth, aHeight);
  img->ImageUpdated(nsnull, nsImageUpdateFlags_kBitsChanged, &r);
}





PRBool imgContainer::CopyFrameImage(gfxIImageFrame *aSrcFrame,
                                       gfxIImageFrame *aDstFrame)
{
  PRUint8* aDataSrc;
  PRUint8* aDataDest;
  PRUint32 aDataLengthSrc;
  PRUint32 aDataLengthDest;

  if (!aSrcFrame || !aDstFrame)
    return PR_FALSE;

  if (NS_FAILED(aDstFrame->LockImageData()))
    return PR_FALSE;

  
  aSrcFrame->GetImageData(&aDataSrc, &aDataLengthSrc);
  aDstFrame->GetImageData(&aDataDest, &aDataLengthDest);
  if (!aDataDest || !aDataSrc || aDataLengthDest != aDataLengthSrc) {
    aDstFrame->UnlockImageData();
    return PR_FALSE;
  }
  memcpy(aDataDest, aDataSrc, aDataLengthSrc);
  aDstFrame->UnlockImageData();

  
  nsCOMPtr<nsIInterfaceRequestor> ireq(do_QueryInterface(aDstFrame));
  if (!ireq)
    return PR_FALSE;
  nsCOMPtr<nsIImage> img(do_GetInterface(ireq));
  if (!img)
    return PR_FALSE;
  nsIntRect r;
  aDstFrame->GetRect(r);
  img->ImageUpdated(nsnull, nsImageUpdateFlags_kBitsChanged, &r);

  return PR_TRUE;
}
