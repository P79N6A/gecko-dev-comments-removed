









































#include "imgContainerGIF.h"

#include "nsIServiceManager.h"
#include "nsIImage.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsMemory.h"
#include "gfxContext.h"

NS_IMPL_ISUPPORTS2(imgContainerGIF, imgIContainer, nsITimerCallback)


imgContainerGIF::imgContainerGIF()
  : mObserver(nsnull)
  , mSize(0,0)
  , mFirstFrameRefreshArea()
  , mCurrentDecodingFrameIndex(0)
  , mCurrentAnimationFrameIndex(0)
  , mLastCompositedFrameIndex(-1)
  , mDoneDecoding(PR_FALSE)
  , mAnimating(PR_FALSE)
  , mAnimationMode(kNormalAnimMode)
  , mLoopCount(-1)
{
  
}


imgContainerGIF::~imgContainerGIF()
{
  if (mTimer)
    mTimer->Cancel();
}




NS_IMETHODIMP imgContainerGIF::Init(PRInt32 aWidth, PRInt32 aHeight,
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



NS_IMETHODIMP imgContainerGIF::GetPreferredAlphaChannelFormat(gfx_format *aFormat)
{
  *aFormat = gfxIFormats::RGB_A1;
  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::GetWidth(PRInt32 *aWidth)
{
  *aWidth = mSize.width;
  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::GetHeight(PRInt32 *aHeight)
{
  *aHeight = mSize.height;
  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::GetCurrentFrame(gfxIImageFrame * *aCurrentFrame)
{
  if (!(*aCurrentFrame = inlinedGetCurrentFrame()))
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aCurrentFrame);
  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::GetNumFrames(PRUint32 *aNumFrames)
{
  *aNumFrames = mFrames.Count();
  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::GetFrameAt(PRUint32 index,
                                          gfxIImageFrame **_retval)
{
  NS_ENSURE_ARG(index < mFrames.Count());

  if (!(*_retval = mFrames[index]))
    return NS_ERROR_FAILURE;

  NS_ADDREF(*_retval);
  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::AppendFrame(gfxIImageFrame *item)
{
  NS_ASSERTION(item, "imgContainerGIF::AppendFrame: item is null");
  if (!item)
    return NS_ERROR_NULL_POINTER;

  PRInt32 numFrames = mFrames.Count();
  if (numFrames == 0) {
    
    
    
    
    PRInt32 frameDisposalMethod;
    item->GetFrameDisposalMethod(&frameDisposalMethod);
    if (frameDisposalMethod == DISPOSE_CLEAR ||
        frameDisposalMethod == DISPOSE_RESTORE_PREVIOUS)
      item->GetRect(mFirstFrameRefreshArea);
  } else {
    
    
    
    nsIntRect itemRect;
    item->GetRect(itemRect);
    mFirstFrameRefreshArea.UnionRect(mFirstFrameRefreshArea, itemRect);
  }

  mFrames.AppendObject(item);

  
  
  
  if (numFrames == 1)
    StartAnimation();

  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::RemoveFrame(gfxIImageFrame *item)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP imgContainerGIF::EndFrameDecode(PRUint32 aFrameNum,
                                              PRUint32 aTimeout)
{
  
  
  mCurrentDecodingFrameIndex = aFrameNum;
  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::DecodingComplete(void)
{
  mDoneDecoding = PR_TRUE;
  
  
  if (mFrames.Count() == 1)
    mFrames[0]->SetMutable(PR_FALSE);
  return NS_OK;
}


NS_IMETHODIMP imgContainerGIF::Clear()
{
  mFrames.Clear();
  return NS_OK;
}


NS_IMETHODIMP imgContainerGIF::GetAnimationMode(PRUint16 *aAnimationMode)
{
  if (!aAnimationMode)
    return NS_ERROR_NULL_POINTER;
  *aAnimationMode = mAnimationMode;
  return NS_OK;
}


NS_IMETHODIMP imgContainerGIF::SetAnimationMode(PRUint16 aAnimationMode)
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
      if (mLoopCount != 0 || mCurrentAnimationFrameIndex + 1 < mFrames.Count())
        StartAnimation();
      break;
    case kLoopOnceAnimMode:
      if (mCurrentAnimationFrameIndex + 1 < mFrames.Count())
        StartAnimation();
      break;
  }

  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::StartAnimation()
{
  if (mAnimationMode == kDontAnimMode || mAnimating || mTimer)
    return NS_OK;

  if (mFrames.Count() > 1) {
    PRInt32 timeout;
    gfxIImageFrame *currentFrame = inlinedGetCurrentFrame();
    if (currentFrame) {
      currentFrame->GetTimeout(&timeout);
      if (timeout <= 0) 
        return NS_OK;
    } else
      timeout = 100; 
                     

    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (!mTimer)
      return NS_ERROR_OUT_OF_MEMORY;

    
    mAnimating = PR_TRUE;
    mTimer->InitWithCallback(static_cast<nsITimerCallback*>(this),
                             timeout, nsITimer::TYPE_REPEATING_SLACK);
  }

  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::StopAnimation()
{
  mAnimating = PR_FALSE;

  if (!mTimer)
    return NS_OK;

  mTimer->Cancel();
  mTimer = nsnull;

  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::ResetAnimation()
{
  if (mCurrentAnimationFrameIndex == 0 || mAnimationMode == kDontAnimMode)
    return NS_OK;

  PRBool oldAnimating = mAnimating;

  if (oldAnimating) {
    nsresult rv = StopAnimation();
    if (NS_FAILED(rv))
      return rv;
   }

  mLastCompositedFrameIndex = -1;
  mCurrentAnimationFrameIndex = 0;
  
  nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));
  if (observer)
    observer->FrameChanged(this, mFrames[0], &mFirstFrameRefreshArea);

  if (oldAnimating)
    return StartAnimation();
  else
    return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::GetLoopCount(PRInt32 *aLoopCount)
{
  NS_ASSERTION(aLoopCount, "ptr is null");
  *aLoopCount = mLoopCount;
  return NS_OK;
}

NS_IMETHODIMP imgContainerGIF::SetLoopCount(PRInt32 aLoopCount)
{
  
  
  
  
  mLoopCount = aLoopCount;

  return NS_OK;
}



NS_IMETHODIMP imgContainerGIF::Notify(nsITimer *timer)
{
  NS_ASSERTION(mTimer == timer,
               "imgContainerGIF::Notify called with incorrect timer");

  if (!mAnimating || !mTimer)
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
  PRInt32 previousFrameIndex = mCurrentAnimationFrameIndex;
  PRInt32 nextFrameIndex = mCurrentAnimationFrameIndex + 1;
  PRInt32 timeout = 0;

  
  
  if (mDoneDecoding || (nextFrameIndex < mCurrentDecodingFrameIndex)) {
    if (numFrames == nextFrameIndex) {
      

      
      if (mAnimationMode == kLoopOnceAnimMode || mLoopCount == 0) {
        StopAnimation();
        return NS_OK;
      } else {
        
        
        if (mCompositingFrame && mLastCompositedFrameIndex == -1)
          mCompositingFrame = nsnull;
      }

      nextFrameIndex = 0;
      if (mLoopCount > 0)
        mLoopCount--;
    }

    if (!(nextFrame = mFrames[nextFrameIndex])) {
      
      mCurrentAnimationFrameIndex = nextFrameIndex;
      mTimer->SetDelay(100);
      return NS_OK;
    }
    nextFrame->GetTimeout(&timeout);

  } else if (nextFrameIndex == mCurrentDecodingFrameIndex) {
    
    
    mTimer->SetDelay(100);
    return NS_OK;
  } else { 
    
    
    NS_WARNING("imgContainerGIF::Notify()  Frame is passed decoded frame");
    nextFrameIndex = mCurrentDecodingFrameIndex;
    if (!(nextFrame = mFrames[nextFrameIndex])) {
      
      mCurrentAnimationFrameIndex = nextFrameIndex;
      mTimer->SetDelay(100);
      return NS_OK;
    }
    nextFrame->GetTimeout(&timeout);
  }

  if (timeout > 0)
    mTimer->SetDelay(timeout);
  else
    StopAnimation();

  nsIntRect dirtyRect;
  gfxIImageFrame *frameToUse = nsnull;

  if (nextFrameIndex == 0) {
    frameToUse = nextFrame;
    dirtyRect = mFirstFrameRefreshArea;
  } else {
    gfxIImageFrame *prevFrame = mFrames[previousFrameIndex];
    if (!prevFrame)
      return NS_OK;

    
    if (NS_FAILED(DoComposite(&frameToUse, &dirtyRect, prevFrame,
                              nextFrame, nextFrameIndex))) {
      
      NS_WARNING("imgContainerGIF: Composing Frame Failed\n");
      mCurrentAnimationFrameIndex = nextFrameIndex;
      return NS_OK;
    }
  }
  
  mCurrentAnimationFrameIndex = nextFrameIndex;
  
  observer->FrameChanged(this, frameToUse, &dirtyRect);
  return NS_OK;
}




nsresult imgContainerGIF::DoComposite(gfxIImageFrame** aFrameToUse,
                                      nsIntRect* aDirtyRect,
                                      gfxIImageFrame* aPrevFrame,
                                      gfxIImageFrame* aNextFrame,
                                      PRInt32 aNextFrameIndex)
{
  NS_ASSERTION(aDirtyRect, "imgContainerGIF::DoComposite aDirtyRect is null");
  NS_ASSERTION(aPrevFrame, "imgContainerGIF::DoComposite aPrevFrame is null");
  NS_ASSERTION(aNextFrame, "imgContainerGIF::DoComposite aNextFrame is null");
  NS_ASSERTION(aFrameToUse, "imgContainerGIF::DoComposite aFrameToUse is null");

  PRInt32 prevFrameDisposalMethod;
  aPrevFrame->GetFrameDisposalMethod(&prevFrameDisposalMethod);

  if (prevFrameDisposalMethod == DISPOSE_RESTORE_PREVIOUS &&
      !mCompositingPrevFrame)
    prevFrameDisposalMethod = DISPOSE_CLEAR;

  
  
  if (prevFrameDisposalMethod == DISPOSE_CLEAR_ALL) {
    aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
    *aFrameToUse = aNextFrame;
    return NS_OK;
  }

  nsIntRect prevFrameRect;
  aPrevFrame->GetRect(prevFrameRect);
  PRBool isFullPrevFrame = (prevFrameRect.x == 0 && prevFrameRect.y == 0 &&
                            prevFrameRect.width == mSize.width &&
                            prevFrameRect.height == mSize.height);

  
  
  if (isFullPrevFrame && prevFrameDisposalMethod == DISPOSE_CLEAR) {
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
      (nextFrameDisposalMethod != DISPOSE_RESTORE_PREVIOUS) &&
      !nextFrameHasAlpha) {

    aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
    *aFrameToUse = aNextFrame;
    return NS_OK;
  }

  
  switch (prevFrameDisposalMethod) {
    default:
    case DISPOSE_NOT_SPECIFIED:
    case DISPOSE_KEEP:
      *aDirtyRect = nextFrameRect;
      break;

    case DISPOSE_CLEAR:
      
      
      
      
      
      
      
      aDirtyRect->UnionRect(nextFrameRect, prevFrameRect);
      break;

    case DISPOSE_RESTORE_PREVIOUS:
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;
  }

  
  
  
  
  
  
  if (mLastCompositedFrameIndex == aNextFrameIndex) {
    *aFrameToUse = mCompositingFrame;
    return NS_OK;
  }

  PRBool needToBlankComposite = PR_FALSE;

  
  if (!mCompositingFrame) {
    nsresult rv;
    mCompositingFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2", &rv);
    if (NS_FAILED(rv))
      return rv;
    rv = mCompositingFrame->Init(0, 0, mSize.width, mSize.height,
                                 gfxIFormats::RGB_A1, 24);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to init mCompositingFrame!\n");
      mCompositingFrame = nsnull;
      return rv;
    }
    needToBlankComposite = PR_TRUE;
  }

  
  
  
  
  
  
  if (mLastCompositedFrameIndex != aNextFrameIndex - 1 &&
      prevFrameDisposalMethod != DISPOSE_RESTORE_PREVIOUS) {

    
    
    
    
    if (isFullPrevFrame) {
      CopyFrameImage(aPrevFrame, mCompositingFrame);
    } else {
      BlackenFrame(mCompositingFrame);
      SetMaskVisibility(mCompositingFrame, PR_FALSE);
      aPrevFrame->DrawTo(mCompositingFrame, prevFrameRect.x, prevFrameRect.y,
                         prevFrameRect.width, prevFrameRect.height);

      BuildCompositeMask(mCompositingFrame, aPrevFrame);
      needToBlankComposite = PR_FALSE;
    }
  }

  
  switch (prevFrameDisposalMethod) {
    case DISPOSE_CLEAR:
      if (needToBlankComposite) {
        
        
        BlackenFrame(mCompositingFrame);
        SetMaskVisibility(mCompositingFrame, PR_FALSE);
        needToBlankComposite = PR_FALSE;
      } else {
        
        BlackenFrame(mCompositingFrame, prevFrameRect);
        SetMaskVisibility(mCompositingFrame, prevFrameRect, PR_FALSE);
      }
      break;

    case DISPOSE_RESTORE_PREVIOUS:
      
      
      if (mCompositingPrevFrame) {
        CopyFrameImage(mCompositingPrevFrame, mCompositingFrame);

        
        if (nextFrameDisposalMethod != DISPOSE_RESTORE_PREVIOUS)
          mCompositingPrevFrame = nsnull;
      } else {
        BlackenFrame(mCompositingFrame);
        SetMaskVisibility(mCompositingFrame, PR_FALSE);
      }
      break;
  }

  
  
  
  if ((nextFrameDisposalMethod == DISPOSE_RESTORE_PREVIOUS) &&
      (prevFrameDisposalMethod != DISPOSE_RESTORE_PREVIOUS)) {
    
    
    
    if (!mCompositingPrevFrame) {
      nsresult rv;
      mCompositingPrevFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2",
                                                &rv);
      if (NS_FAILED(rv))
        return rv;
      rv = mCompositingPrevFrame->Init(0, 0, mSize.width, mSize.height,
                                       gfxIFormats::RGB_A1, 24);
      if (NS_FAILED(rv))
        return rv;
    }
    CopyFrameImage(mCompositingFrame, mCompositingPrevFrame);
  }

  
  aNextFrame->DrawTo(mCompositingFrame, nextFrameRect.x, nextFrameRect.y,
                     nextFrameRect.width, nextFrameRect.height);
  
  BuildCompositeMask(mCompositingFrame, aNextFrame);
  
  
  PRInt32 timeout;
  aNextFrame->GetTimeout(&timeout);
  mCompositingFrame->SetTimeout(timeout);

  if (isFullNextFrame && mAnimationMode == kNormalAnimMode && mLoopCount != 0) {
    
    
    
    
    
    if (CopyFrameImage(mCompositingFrame, aNextFrame)) {
      aPrevFrame->SetFrameDisposalMethod(DISPOSE_CLEAR_ALL);
      mLastCompositedFrameIndex = -1;
      *aFrameToUse = aNextFrame;
      return NS_OK;
    }
  }

  mLastCompositedFrameIndex = aNextFrameIndex;
  *aFrameToUse = mCompositingFrame;

  return NS_OK;
}


void imgContainerGIF::BuildCompositeMask(gfxIImageFrame *aCompositingFrame,
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

  gfx_format format;
  aCompositingFrame->GetFormat(&format);
  if (format != gfxIFormats::RGB_A1 && format != gfxIFormats::BGR_A1) {
    NS_NOTREACHED("GIFs only support 1 bit alpha");
    aCompositingFrame->UnlockAlphaData();
    aOverlayFrame->UnlockAlphaData();
    return;
  }

  
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


void imgContainerGIF::SetMaskVisibility(gfxIImageFrame *aFrame,
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


void imgContainerGIF::SetMaskVisibility(gfxIImageFrame *aFrame, PRBool aVisible)
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



void imgContainerGIF::BlackenFrame(gfxIImageFrame *aFrame)
{
  if (!aFrame)
    return;

  PRInt32 widthFrame;
  PRInt32 heightFrame;
  aFrame->GetWidth(&widthFrame);
  aFrame->GetHeight(&heightFrame);

  BlackenFrame(aFrame, 0, 0, widthFrame, heightFrame);
}


void imgContainerGIF::BlackenFrame(gfxIImageFrame *aFrame,
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





PRBool imgContainerGIF::CopyFrameImage(gfxIImageFrame *aSrcFrame,
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
