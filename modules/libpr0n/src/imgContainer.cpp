










































#include "nsComponentManagerUtils.h"
#include "imgIContainerObserver.h"
#include "ImageErrors.h"
#include "imgILoad.h"
#include "imgIDecoder.h"
#include "imgIDecoderObserver.h"
#include "imgContainer.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAutoPtr.h"
#include "nsStringStream.h"
#include "prmem.h"
#include "prlog.h"
#include "prenv.h"

#include "gfxContext.h"


#if defined(PR_LOGGING)
static PRLogModuleInfo *gCompressedImageAccountingLog = PR_NewLogModule ("CompressedImageAccounting");
#else
#define gCompressedImageAccountingLog
#endif

static int num_containers_with_discardable_data;
static PRInt64 num_compressed_image_bytes;


NS_IMPL_ISUPPORTS3(imgContainer, imgIContainer, nsITimerCallback, nsIProperties)


imgContainer::imgContainer() :
  mSize(0,0),
  mNumFrames(0),
  mAnim(nsnull),
  mAnimationMode(kNormalAnimMode),
  mLoopCount(-1),
  mObserver(nsnull),
  mDiscardable(PR_FALSE),
  mDiscarded(PR_FALSE),
  mRestoreDataDone(PR_FALSE),
  mDiscardTimer(nsnull)
{
}


imgContainer::~imgContainer()
{
  if (mAnim)
    delete mAnim;

  for (unsigned int i = 0; i < mFrames.Length(); ++i)
    delete mFrames[i];

  if (!mRestoreData.IsEmpty()) {
    num_containers_with_discardable_data--;
    num_compressed_image_bytes -= mRestoreData.Length();

    PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
            ("CompressedImageAccounting: destroying imgContainer %p.  "
             "Compressed containers: %d, Compressed data bytes: %lld",
             this,
             num_containers_with_discardable_data,
             num_compressed_image_bytes));
  }

  if (mDiscardTimer) {
    mDiscardTimer->Cancel ();
    mDiscardTimer = nsnull;
  }
}




NS_IMETHODIMP imgContainer::Init(PRInt32 aWidth, PRInt32 aHeight,
                                 imgIContainerObserver *aObserver)
{
  if (aWidth <= 0 || aHeight <= 0) {
    NS_WARNING("error - negative image size\n");
    return NS_ERROR_FAILURE;
  }

  mSize.SizeTo(aWidth, aHeight);
  
  
  mDiscarded = PR_FALSE;

  mObserver = do_GetWeakReference(aObserver);
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::ExtractCurrentFrame(const nsIntRect &aRegion, imgIContainer **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsRefPtr<imgContainer> img(new imgContainer());
  NS_ENSURE_TRUE(img, NS_ERROR_OUT_OF_MEMORY);

  img->Init(aRegion.width, aRegion.height, nsnull);

  imgFrame *frame = GetCurrentImgFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  
  
  nsIntRect framerect = frame->GetRect();
  framerect.IntersectRect(framerect, aRegion);

  if (framerect.IsEmpty())
    return NS_ERROR_NOT_AVAILABLE;

  nsAutoPtr<imgFrame> subframe;
  nsresult rv = frame->Extract(framerect, getter_Transfers(subframe));
  if (NS_FAILED(rv))
    return rv;

  img->mFrames.AppendElement(subframe.forget());
  img->mNumFrames++;

  *_retval = img.forget().get();

  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetWidth(PRInt32 *aWidth)
{
  NS_ENSURE_ARG_POINTER(aWidth);

  *aWidth = mSize.width;
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetHeight(PRInt32 *aHeight)
{
  NS_ENSURE_ARG_POINTER(aHeight);

  *aHeight = mSize.height;
  return NS_OK;
}

imgFrame *imgContainer::GetImgFrame(PRUint32 framenum)
{
  nsresult rv = RestoreDiscardedData();
  NS_ENSURE_SUCCESS(rv, nsnull);

  if (!mAnim) {
    NS_ASSERTION(framenum == 0, "Don't ask for a frame > 0 if we're not animated!");
    return mFrames.SafeElementAt(0, nsnull);
  }
  if (mAnim->lastCompositedFrameIndex == PRInt32(framenum))
    return mAnim->compositingFrame;
  return mFrames.SafeElementAt(framenum, nsnull);
}

PRInt32 imgContainer::GetCurrentImgFrameIndex() const
{
  if (mAnim)
    return mAnim->currentAnimationFrameIndex;

  return 0;
}

imgFrame *imgContainer::GetCurrentImgFrame()
{
  return GetImgFrame(GetCurrentImgFrameIndex());
}



NS_IMETHODIMP imgContainer::GetCurrentFrameIsOpaque(PRBool *aIsOpaque)
{
  NS_ENSURE_ARG_POINTER(aIsOpaque);

  imgFrame *curframe = GetCurrentImgFrame();
  NS_ENSURE_TRUE(curframe, NS_ERROR_FAILURE);

  *aIsOpaque = !curframe->GetNeedsBackground();

  
  
  nsIntRect framerect = curframe->GetRect();
  *aIsOpaque = *aIsOpaque && (framerect != nsIntRect(0, 0, mSize.width, mSize.height));

  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetCurrentFrameRect(nsIntRect &aRect)
{
  imgFrame *curframe = GetCurrentImgFrame();
  NS_ENSURE_TRUE(curframe, NS_ERROR_FAILURE);

  aRect = curframe->GetRect();

  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetCurrentFrameIndex(PRUint32 *aCurrentFrameIdx)
{
  NS_ENSURE_ARG_POINTER(aCurrentFrameIdx);
  
  *aCurrentFrameIdx = GetCurrentImgFrameIndex();

  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetNumFrames(PRUint32 *aNumFrames)
{
  NS_ENSURE_ARG_POINTER(aNumFrames);

  *aNumFrames = mNumFrames;
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetAnimated(PRBool *aAnimated)
{
  NS_ENSURE_ARG_POINTER(aAnimated);

  *aAnimated = (mNumFrames > 1);
  
  return NS_OK;
}




NS_IMETHODIMP imgContainer::CopyCurrentFrame(gfxImageSurface **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  imgFrame *frame = GetImgFrame(GetCurrentImgFrameIndex());
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  nsRefPtr<gfxPattern> pattern;
  frame->GetPattern(getter_AddRefs(pattern));
  nsIntRect intframerect = frame->GetRect();
  gfxRect framerect(intframerect.x, intframerect.y, intframerect.width, intframerect.height);

  
  
  nsRefPtr<gfxImageSurface> imgsurface = new gfxImageSurface(gfxIntSize(mSize.width, mSize.height),
                                                             gfxASurface::ImageFormatARGB32);
  gfxContext ctx(imgsurface);
  ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx.SetPattern(pattern);
  ctx.Rectangle(framerect);
  ctx.Fill();

  *_retval = imgsurface.forget().get();
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetCurrentFrame(gfxASurface **_retval)
{
  imgFrame *frame = GetImgFrame(GetCurrentImgFrameIndex());
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  nsRefPtr<gfxASurface> framesurf;
  nsresult rv = NS_OK;

  
  
  nsIntRect framerect = frame->GetRect();
  if (framerect.x == 0 && framerect.y == 0 &&
      framerect.width == mSize.width &&
      framerect.height == mSize.height)
    rv = frame->GetSurface(getter_AddRefs(framesurf));

  
  
  if (!framesurf) {
    nsRefPtr<gfxImageSurface> imgsurf;
    rv = CopyCurrentFrame(getter_AddRefs(imgsurf));
    framesurf = imgsurf;
  }

  *_retval = framesurf.forget().get();

  return rv;
}



NS_IMETHODIMP imgContainer::GetFrameImageDataLength(PRUint32 framenum, PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  if (framenum >= PRUint32(mNumFrames))
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(framenum);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  *_retval = frame->GetImageDataLength();

  return NS_OK;
}





NS_IMETHODIMP imgContainer::GetFrameColormap(PRUint32 framenum, PRUint32 **aPaletteData,
                                             PRUint32 *aPaletteLength)
{
  NS_ENSURE_ARG_POINTER(aPaletteData);
  NS_ENSURE_ARG_POINTER(aPaletteLength);

  if (framenum >= PRUint32(mNumFrames))
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(framenum);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  if (!frame->GetIsPaletted())
    return NS_ERROR_FAILURE;

  frame->GetPaletteData(aPaletteData, aPaletteLength);

  return NS_OK;
}

nsresult imgContainer::InternalAddFrameHelper(PRUint32 framenum, imgFrame *aFrame,
                                              PRUint8 **imageData, PRUint32 *imageLength,
                                              PRUint32 **paletteData, PRUint32 *paletteLength)
{
  if (framenum > PRUint32(mNumFrames))
    return NS_ERROR_INVALID_ARG;

  nsAutoPtr<imgFrame> frame(aFrame);

  if (paletteData && paletteLength)
    frame->GetPaletteData(paletteData, paletteLength);

  frame->GetImageData(imageData, imageLength);

  mFrames.InsertElementAt(framenum, frame.forget());
  mNumFrames++;

  return NS_OK;
}
                                  
nsresult imgContainer::InternalAddFrame(PRUint32 framenum,
                                        PRInt32 aX, PRInt32 aY,
                                        PRInt32 aWidth, PRInt32 aHeight,
                                        gfxASurface::gfxImageFormat aFormat,
                                        PRUint8 aPaletteDepth,
                                        PRUint8 **imageData,
                                        PRUint32 *imageLength,
                                        PRUint32 **paletteData,
                                        PRUint32 *paletteLength)
{
  if (framenum > PRUint32(mNumFrames))
    return NS_ERROR_INVALID_ARG;

  nsAutoPtr<imgFrame> frame(new imgFrame());
  NS_ENSURE_TRUE(frame, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = frame->Init(aX, aY, aWidth, aHeight, aFormat, aPaletteDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mFrames.Length() == 0) {
    return InternalAddFrameHelper(framenum, frame.forget(), imageData, imageLength, 
                                  paletteData, paletteLength);
  }

  if (mFrames.Length() == 1) {
    
    if (!ensureAnimExists())
      return NS_ERROR_OUT_OF_MEMORY;
    
    
    
    
    PRInt32 frameDisposalMethod = mFrames[0]->GetFrameDisposalMethod();
    if (frameDisposalMethod == imgIContainer::kDisposeClear ||
        frameDisposalMethod == imgIContainer::kDisposeRestorePrevious)
      mAnim->firstFrameRefreshArea = mFrames[0]->GetRect();
  }

  
  
  
  nsIntRect frameRect = frame->GetRect();
  mAnim->firstFrameRefreshArea.UnionRect(mAnim->firstFrameRefreshArea, 
                                         frameRect);
  
  rv = InternalAddFrameHelper(framenum, frame.forget(), imageData, imageLength,
                              paletteData, paletteLength);
  
  
  
  
  if (mFrames.Length() == 2)
    StartAnimation();
  
  return rv;
}


NS_IMETHODIMP imgContainer::AppendFrame(PRInt32 aX, PRInt32 aY, PRInt32 aWidth,
                                        PRInt32 aHeight, 
                                        gfxASurface::gfxImageFormat aFormat,
                                        PRUint8 **imageData,
                                        PRUint32 *imageLength)
{
  NS_ENSURE_ARG_POINTER(imageData);
  NS_ENSURE_ARG_POINTER(imageLength);

  return InternalAddFrame(mNumFrames, aX, aY, aWidth, aHeight, aFormat, 
                           0, imageData, imageLength,
                           nsnull, 
                           nsnull);
}


NS_IMETHODIMP imgContainer::AppendPalettedFrame(PRInt32 aX, PRInt32 aY,
                                                PRInt32 aWidth, PRInt32 aHeight,
                                                gfxASurface::gfxImageFormat aFormat,
                                                PRUint8 aPaletteDepth,
                                                PRUint8 **imageData,
                                                PRUint32 *imageLength,
                                                PRUint32 **paletteData,
                                                PRUint32 *paletteLength)
{
  NS_ENSURE_ARG_POINTER(imageData);
  NS_ENSURE_ARG_POINTER(imageLength);
  NS_ENSURE_ARG_POINTER(paletteData);
  NS_ENSURE_ARG_POINTER(paletteLength);

  return InternalAddFrame(mNumFrames, aX, aY, aWidth, aHeight, aFormat, 
                          aPaletteDepth, imageData, imageLength,
                          paletteData, paletteLength);
}


NS_IMETHODIMP imgContainer::EnsureCleanFrame(PRUint32 aFrameNum, PRInt32 aX, PRInt32 aY,
                                             PRInt32 aWidth, PRInt32 aHeight, 
                                             gfxASurface::gfxImageFormat aFormat,
                                             PRUint8 **imageData, PRUint32 *imageLength)
{
  NS_ENSURE_ARG_POINTER(imageData);
  NS_ENSURE_ARG_POINTER(imageLength);
  if (aFrameNum > PRUint32(mNumFrames))
    return NS_ERROR_INVALID_ARG;

  
  if (aFrameNum == PRUint32(mNumFrames))
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat, 
                             0, imageData, imageLength,
                             nsnull, 
                             nsnull);

  imgFrame *frame = GetImgFrame(aFrameNum);
  if (!frame)
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat, 
                             0, imageData, imageLength,
                             nsnull, 
                             nsnull);

  
  nsIntRect rect = frame->GetRect();
  if (rect.x != aX || rect.y != aY || rect.width != aWidth || rect.height != aHeight ||
      frame->GetFormat() != aFormat) {
    delete frame;
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat, 
                             0, imageData, imageLength,
                             nsnull, 
                             nsnull);
  }

  
  frame->GetImageData(imageData, imageLength);

  return NS_OK;
}




NS_IMETHODIMP imgContainer::FrameUpdated(PRUint32 aFrameNum, nsIntRect &aUpdatedRect)
{
  if (aFrameNum >= PRUint32(mNumFrames))
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->ImageUpdated(aUpdatedRect);

  return NS_OK;
}



NS_IMETHODIMP imgContainer::SetFrameDisposalMethod(PRUint32 aFrameNum, PRInt32 aDisposalMethod)
{
  if (aFrameNum >= PRUint32(mNumFrames))
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetFrameDisposalMethod(aDisposalMethod);

  return NS_OK;
}



NS_IMETHODIMP imgContainer::SetFrameTimeout(PRUint32 aFrameNum, PRInt32 aTimeout)
{
  if (aFrameNum >= PRUint32(mNumFrames))
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetTimeout(aTimeout);

  return NS_OK;
}



NS_IMETHODIMP imgContainer::SetFrameBlendMethod(PRUint32 aFrameNum, PRInt32 aBlendMethod)
{
  if (aFrameNum >= PRUint32(mNumFrames))
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetBlendMethod(aBlendMethod);

  return NS_OK;
}




NS_IMETHODIMP imgContainer::SetFrameHasNoAlpha(PRUint32 aFrameNum)
{
  if (aFrameNum >= PRUint32(mNumFrames))
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetHasNoAlpha();

  return NS_OK;
}



NS_IMETHODIMP imgContainer::EndFrameDecode(PRUint32 aFrameNum)
{
  
  
  if (mAnim)
    mAnim->currentDecodingFrameIndex = aFrameNum;
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::DecodingComplete(void)
{
  if (mAnim)
    mAnim->doneDecoding = PR_TRUE;

  
  
  if (mNumFrames == 1)
    return mFrames[0]->Optimize();

  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetAnimationMode(PRUint16 *aAnimationMode)
{
  NS_ENSURE_ARG_POINTER(aAnimationMode);
  
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
          (mAnim && (mAnim->currentAnimationFrameIndex + 1 < mNumFrames)))
        StartAnimation();
      break;
    case kLoopOnceAnimMode:
      if (mAnim && (mAnim->currentAnimationFrameIndex + 1 < mNumFrames))
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
  
  if (mNumFrames > 1) {
    if (!ensureAnimExists())
      return NS_ERROR_OUT_OF_MEMORY;
    
    
    
    PRInt32 timeout = 100;
    imgFrame *currentFrame = GetCurrentImgFrame();
    if (currentFrame) {
      timeout = currentFrame->GetTimeout();
      if (timeout <= 0) 
        return NS_OK;
    }
    
    mAnim->timer = do_CreateInstance("@mozilla.org/timer;1");
    NS_ENSURE_TRUE(mAnim->timer, NS_ERROR_OUT_OF_MEMORY);
    
    
    mAnim->animating = PR_TRUE;
    mAnim->timer->InitWithCallback(static_cast<nsITimerCallback*>(this),
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
      !mAnim || mAnim->currentAnimationFrameIndex == 0)
    return NS_OK;

  PRBool oldAnimating = mAnim->animating;

  if (mAnim->animating) {
    nsresult rv = StopAnimation();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mAnim->lastCompositedFrameIndex = -1;
  mAnim->currentAnimationFrameIndex = 0;
  
  nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));
  if (observer) {
    nsresult rv = RestoreDiscardedData();
    NS_ENSURE_SUCCESS(rv, rv);
    observer->FrameChanged(this, &(mAnim->firstFrameRefreshArea));
  }

  if (oldAnimating)
    return StartAnimation();
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetLoopCount(PRInt32 *aLoopCount)
{
  NS_ENSURE_ARG_POINTER(aLoopCount);
  
  *aLoopCount = mLoopCount;
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::SetLoopCount(PRInt32 aLoopCount)
{
  
  
  
  
  mLoopCount = aLoopCount;

  return NS_OK;
}

static PRBool
DiscardingEnabled(void)
{
  static PRBool inited;
  static PRBool enabled;

  if (!inited) {
    inited = PR_TRUE;

    enabled = (PR_GetEnv("MOZ_DISABLE_IMAGE_DISCARD") == nsnull);
  }

  return enabled;
}



NS_IMETHODIMP imgContainer::SetDiscardable(const char* aMimeType)
{
  NS_ENSURE_ARG_POINTER(aMimeType);

  if (!DiscardingEnabled())
    return NS_OK;

  if (mDiscardable) {
    NS_WARNING ("imgContainer::SetDiscardable(): cannot change an imgContainer which is already discardable");
    return NS_ERROR_FAILURE;
  }

  mDiscardableMimeType.Assign(aMimeType);
  mDiscardable = PR_TRUE;

  num_containers_with_discardable_data++;
  PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
          ("CompressedImageAccounting: Making imgContainer %p (%s) discardable.  "
           "Compressed containers: %d, Compressed data bytes: %lld",
           this,
           aMimeType,
           num_containers_with_discardable_data,
           num_compressed_image_bytes));

  return NS_OK;
}



NS_IMETHODIMP imgContainer::AddRestoreData(const char *aBuffer, PRUint32 aCount)
{
  NS_ENSURE_ARG_POINTER(aBuffer);

  if (!mDiscardable)
    return NS_OK;

  if (mRestoreDataDone) {
    




    return NS_OK;
  }

  if (!mRestoreData.AppendElements(aBuffer, aCount))
    return NS_ERROR_OUT_OF_MEMORY;

  num_compressed_image_bytes += aCount;

  PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
          ("CompressedImageAccounting: Added compressed data to imgContainer %p (%s).  "
           "Compressed containers: %d, Compressed data bytes: %lld",
           this,
           mDiscardableMimeType.get(),
           num_containers_with_discardable_data,
           num_compressed_image_bytes));

  return NS_OK;
}



static void
get_header_str (char *buf, char *data, PRSize data_len)
{
  int i;
  int n;
  static char hex[] = "0123456789abcdef";

  n = data_len < 4 ? data_len : 4;

  for (i = 0; i < n; i++) {
    buf[i * 2]     = hex[(data[i] >> 4) & 0x0f];
    buf[i * 2 + 1] = hex[data[i] & 0x0f];
  }

  buf[i * 2] = 0;
}



NS_IMETHODIMP imgContainer::RestoreDataDone (void)
{
  
  if (!mDiscardable)
    return NS_OK;

  if (mRestoreDataDone)
    return NS_OK;

  mRestoreData.Compact();

  mRestoreDataDone = PR_TRUE;

  if (PR_LOG_TEST(gCompressedImageAccountingLog, PR_LOG_DEBUG)) {
    char buf[9];
    get_header_str(buf, mRestoreData.Elements(), mRestoreData.Length());
    PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
            ("CompressedImageAccounting: imgContainer::RestoreDataDone() - data is done for container %p (%s), %d real frames (cached as %d frames) - header %p is 0x%s (length %d)",
             this,
             mDiscardableMimeType.get(),
             mFrames.Length (),
             mNumFrames,
             mRestoreData.Elements(),
             buf,
             mRestoreData.Length()));
  }

  return ResetDiscardTimer();
}



NS_IMETHODIMP imgContainer::Notify(nsITimer *timer)
{
  
  
  nsresult rv = RestoreDiscardedData();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  NS_ENSURE_TRUE(mAnim, NS_ERROR_UNEXPECTED);
  NS_ASSERTION(mAnim->timer == timer,
               "imgContainer::Notify() called with incorrect timer");

  if (!mAnim->animating || !mAnim->timer)
    return NS_OK;

  nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));
  if (!observer) {
    
    StopAnimation();
    return NS_OK;
  }

  if (mNumFrames == 0)
    return NS_OK;
  
  imgFrame *nextFrame = nsnull;
  PRInt32 previousFrameIndex = mAnim->currentAnimationFrameIndex;
  PRInt32 nextFrameIndex = mAnim->currentAnimationFrameIndex + 1;
  PRInt32 timeout = 0;

  
  
  
  
  if (mAnim->doneDecoding || 
      (nextFrameIndex < mAnim->currentDecodingFrameIndex)) {
    if (mNumFrames == nextFrameIndex) {
      

      
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
    timeout = nextFrame->GetTimeout();

  } else if (nextFrameIndex == mAnim->currentDecodingFrameIndex) {
    
    
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
    timeout = nextFrame->GetTimeout();
  }

  if (timeout > 0)
    mAnim->timer->SetDelay(timeout);
  else
    StopAnimation();

  nsIntRect dirtyRect;
  imgFrame *frameToUse = nsnull;

  if (nextFrameIndex == 0) {
    frameToUse = nextFrame;
    dirtyRect = mAnim->firstFrameRefreshArea;
  } else {
    imgFrame *prevFrame = mFrames[previousFrameIndex];
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
  
  observer->FrameChanged(this, &dirtyRect);
  
  return NS_OK;
}




nsresult imgContainer::DoComposite(imgFrame** aFrameToUse,
                                   nsIntRect* aDirtyRect,
                                   imgFrame* aPrevFrame,
                                   imgFrame* aNextFrame,
                                   PRInt32 aNextFrameIndex)
{
  NS_ENSURE_ARG_POINTER(aDirtyRect);
  NS_ENSURE_ARG_POINTER(aPrevFrame);
  NS_ENSURE_ARG_POINTER(aNextFrame);
  NS_ENSURE_ARG_POINTER(aFrameToUse);

  PRInt32 prevFrameDisposalMethod = aPrevFrame->GetFrameDisposalMethod();
  if (prevFrameDisposalMethod == imgIContainer::kDisposeRestorePrevious &&
      !mAnim->compositingPrevFrame)
    prevFrameDisposalMethod = imgIContainer::kDisposeClear;

  nsIntRect prevFrameRect = aPrevFrame->GetRect();
  PRBool isFullPrevFrame = (prevFrameRect.x == 0 && prevFrameRect.y == 0 &&
                            prevFrameRect.width == mSize.width &&
                            prevFrameRect.height == mSize.height);

  
  
  if (isFullPrevFrame && 
      (prevFrameDisposalMethod == imgIContainer::kDisposeClear))
    prevFrameDisposalMethod = imgIContainer::kDisposeClearAll;

  PRInt32 nextFrameDisposalMethod = aNextFrame->GetFrameDisposalMethod();
  nsIntRect nextFrameRect = aNextFrame->GetRect();
  PRBool isFullNextFrame = (nextFrameRect.x == 0 && nextFrameRect.y == 0 &&
                            nextFrameRect.width == mSize.width &&
                            nextFrameRect.height == mSize.height);

  if (!aNextFrame->GetIsPaletted()) {
    
    
    if (prevFrameDisposalMethod == imgIContainer::kDisposeClearAll) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      *aFrameToUse = aNextFrame;
      return NS_OK;
    }
  
    
    
    if (isFullNextFrame &&
        (nextFrameDisposalMethod != imgIContainer::kDisposeRestorePrevious) &&
        !aNextFrame->GetHasAlpha()) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      *aFrameToUse = aNextFrame;
      return NS_OK;
    }
  }

  
  switch (prevFrameDisposalMethod) {
    default:
    case imgIContainer::kDisposeNotSpecified:
    case imgIContainer::kDisposeKeep:
      *aDirtyRect = nextFrameRect;
      break;

    case imgIContainer::kDisposeClearAll:
      
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
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
    mAnim->compositingFrame = new imgFrame();
    if (!mAnim->compositingFrame) {
      NS_WARNING("Failed to init compositingFrame!\n");
      return NS_ERROR_OUT_OF_MEMORY;
    }
    nsresult rv = mAnim->compositingFrame->Init(0, 0, mSize.width, mSize.height,
                                                gfxASurface::ImageFormatARGB32);
    NS_ENSURE_SUCCESS(rv, rv);
    needToBlankComposite = PR_TRUE;
  } else if (aNextFrameIndex == 1) {
    
    needToBlankComposite = PR_TRUE;
  }

  
  PRBool doDisposal = PR_TRUE;
  if (!aNextFrame->GetHasAlpha()) {
    if (isFullNextFrame) {
      
      
      doDisposal = PR_FALSE;
      
      needToBlankComposite = PR_FALSE;
    } else {
      if ((prevFrameRect.x >= nextFrameRect.x) &&
          (prevFrameRect.y >= nextFrameRect.y) &&
          (prevFrameRect.x + prevFrameRect.width <= nextFrameRect.x + nextFrameRect.width) &&
          (prevFrameRect.y + prevFrameRect.height <= nextFrameRect.y + nextFrameRect.height)) {
        
        
        doDisposal = PR_FALSE;  
      }
    }      
  }

  if (doDisposal) {
    
    switch (prevFrameDisposalMethod) {
      case imgIContainer::kDisposeClear:
        if (needToBlankComposite) {
          
          
          ClearFrame(mAnim->compositingFrame);
        } else {
          
          ClearFrame(mAnim->compositingFrame, prevFrameRect);
        }
        break;
  
      case imgIContainer::kDisposeClearAll:
        ClearFrame(mAnim->compositingFrame);
        break;
  
      case imgIContainer::kDisposeRestorePrevious:
        
        
        if (mAnim->compositingPrevFrame) {
          CopyFrameImage(mAnim->compositingPrevFrame, mAnim->compositingFrame);
  
          
          if (nextFrameDisposalMethod != imgIContainer::kDisposeRestorePrevious)
            mAnim->compositingPrevFrame = nsnull;
        } else {
          ClearFrame(mAnim->compositingFrame);
        }
        break;
      
      default:
        
        
        
        
        
        
        if (mAnim->lastCompositedFrameIndex != aNextFrameIndex - 1) {
          if (isFullPrevFrame && !aPrevFrame->GetIsPaletted()) {
            
            CopyFrameImage(aPrevFrame, mAnim->compositingFrame);
          } else {
            if (needToBlankComposite) {
              
              if (aPrevFrame->GetHasAlpha() || !isFullPrevFrame) {
                ClearFrame(mAnim->compositingFrame);
              }
            }
            DrawFrameTo(aPrevFrame, mAnim->compositingFrame, prevFrameRect);
          }
        }
    }
  } else if (needToBlankComposite) {
    
    
    ClearFrame(mAnim->compositingFrame);
  }

  
  
  
  if ((nextFrameDisposalMethod == imgIContainer::kDisposeRestorePrevious) &&
      (prevFrameDisposalMethod != imgIContainer::kDisposeRestorePrevious)) {
    
    
    
    if (!mAnim->compositingPrevFrame) {
      mAnim->compositingPrevFrame = new imgFrame();
      if (!mAnim->compositingPrevFrame) {
        NS_WARNING("Failed to init compositingFrame!\n");
        return NS_ERROR_OUT_OF_MEMORY;
      }
      nsresult rv = mAnim->compositingPrevFrame->Init(0, 0, mSize.width, mSize.height,
                                                      gfxASurface::ImageFormatARGB32);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    CopyFrameImage(mAnim->compositingFrame, mAnim->compositingPrevFrame);
  }

  
  DrawFrameTo(aNextFrame, mAnim->compositingFrame, nextFrameRect);

  
  
  PRInt32 timeout = aNextFrame->GetTimeout();
  mAnim->compositingFrame->SetTimeout(timeout);

  
  nsresult rv = mAnim->compositingFrame->ImageUpdated(mAnim->compositingFrame->GetRect());
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  if (isFullNextFrame && mAnimationMode == kNormalAnimMode && mLoopCount != 0 &&
      !aNextFrame->GetIsPaletted()) {
    
    
    
    
    
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



void imgContainer::ClearFrame(imgFrame *aFrame)
{
  if (!aFrame)
    return;

  aFrame->LockImageData();

  nsRefPtr<gfxASurface> surf;
  aFrame->GetSurface(getter_AddRefs(surf));

  
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Paint();

  aFrame->UnlockImageData();
}


void imgContainer::ClearFrame(imgFrame *aFrame, nsIntRect &aRect)
{
  if (!aFrame || aRect.width <= 0 || aRect.height <= 0)
    return;

  aFrame->LockImageData();

  nsRefPtr<gfxASurface> surf;
  aFrame->GetSurface(getter_AddRefs(surf));

  
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Rectangle(gfxRect(aRect.x, aRect.y, aRect.width, aRect.height));
  ctx.Fill();

  aFrame->UnlockImageData();
}





PRBool imgContainer::CopyFrameImage(imgFrame *aSrcFrame,
                                    imgFrame *aDstFrame)
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

  return PR_TRUE;
}








nsresult imgContainer::DrawFrameTo(imgFrame *aSrc,
                                   imgFrame *aDst, 
                                   nsIntRect& aSrcRect)
{
  NS_ENSURE_ARG_POINTER(aSrc);
  NS_ENSURE_ARG_POINTER(aDst);

  nsIntRect dstRect = aDst->GetRect();

  
  if (aSrcRect.x < 0 || aSrcRect.y < 0) {
    NS_WARNING("imgContainer::DrawFrameTo: negative offsets not allowed");
    return NS_ERROR_FAILURE;
  }
  
  if ((aSrcRect.x > dstRect.width) || (aSrcRect.y > dstRect.height)) {
    return NS_OK;
  }

  if (aSrc->GetIsPaletted()) {
    
    PRInt32 width = PR_MIN(aSrcRect.width, dstRect.width - aSrcRect.x);
    PRInt32 height = PR_MIN(aSrcRect.height, dstRect.height - aSrcRect.y);

    
    NS_ASSERTION((aSrcRect.x >= 0) && (aSrcRect.y >= 0) &&
                 (aSrcRect.x + width <= dstRect.width) &&
                 (aSrcRect.y + height <= dstRect.height),
                "imgContainer::DrawFrameTo: Invalid aSrcRect");

    
    NS_ASSERTION((width <= aSrcRect.width) && (height <= aSrcRect.height),
                 "imgContainer::DrawFrameTo: source must be smaller than dest");

    if (NS_FAILED(aDst->LockImageData()))
      return NS_ERROR_FAILURE;

    
    PRUint32 size;
    PRUint8 *srcPixels;
    PRUint32 *colormap;
    PRUint32 *dstPixels;

    aSrc->GetImageData(&srcPixels, &size);
    aSrc->GetPaletteData(&colormap, &size);
    aDst->GetImageData((PRUint8 **)&dstPixels, &size);
    if (!srcPixels || !dstPixels || !colormap) {
      aDst->UnlockImageData();
      return NS_ERROR_FAILURE;
    }

    
    dstPixels += aSrcRect.x + (aSrcRect.y * dstRect.width);
    if (!aSrc->GetHasAlpha()) {
      for (PRInt32 r = height; r > 0; --r) {
        for (PRInt32 c = 0; c < width; c++) {
          dstPixels[c] = colormap[srcPixels[c]];
        }
        
        srcPixels += aSrcRect.width;
        dstPixels += dstRect.width;
      }
    } else {
      for (PRInt32 r = height; r > 0; --r) {
        for (PRInt32 c = 0; c < width; c++) {
          const PRUint32 color = colormap[srcPixels[c]];
          if (color)
            dstPixels[c] = color;
        }
        
        srcPixels += aSrcRect.width;
        dstPixels += dstRect.width;
      }
    }

    aDst->UnlockImageData();
    return NS_OK;
  }

  nsRefPtr<gfxPattern> srcPatt;
  aSrc->GetPattern(getter_AddRefs(srcPatt));

  aDst->LockImageData();
  nsRefPtr<gfxASurface> dstSurf;
  aDst->GetSurface(getter_AddRefs(dstSurf));

  gfxContext dst(dstSurf);
  dst.Translate(gfxPoint(aSrcRect.x, aSrcRect.y));
  dst.Rectangle(gfxRect(0, 0, aSrcRect.width, aSrcRect.height), PR_TRUE);
  
  
  PRInt32 blendMethod = aSrc->GetBlendMethod();
  if (blendMethod == imgIContainer::kBlendSource) {
    gfxContext::GraphicsOperator defaultOperator = dst.CurrentOperator();
    dst.SetOperator(gfxContext::OPERATOR_CLEAR);
    dst.Fill();
    dst.SetOperator(defaultOperator);
  }
  dst.SetPattern(srcPatt);
  dst.Paint();

  aDst->UnlockImageData();

  return NS_OK;
}



NS_IMETHODIMP imgContainer::Get(const char *prop, const nsIID & iid, void * *result)
{
  if (!mProperties)
    return NS_ERROR_FAILURE;
  return mProperties->Get(prop, iid, result);
}

NS_IMETHODIMP imgContainer::Set(const char *prop, nsISupports *value)
{
  if (!mProperties)
    mProperties = do_CreateInstance("@mozilla.org/properties;1");
  if (!mProperties)
    return NS_ERROR_OUT_OF_MEMORY;
  return mProperties->Set(prop, value);
}

NS_IMETHODIMP imgContainer::Has(const char *prop, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if (!mProperties) {
    *_retval = PR_FALSE;
    return NS_OK;
  }
  return mProperties->Has(prop, _retval);
}

NS_IMETHODIMP imgContainer::Undefine(const char *prop)
{
  if (!mProperties)
    return NS_ERROR_FAILURE;
  return mProperties->Undefine(prop);
}

NS_IMETHODIMP imgContainer::GetKeys(PRUint32 *count, char ***keys)
{
  if (!mProperties) {
    *count = 0;
    *keys = nsnull;
    return NS_OK;
  }
  return mProperties->GetKeys(count, keys);
}

static int
get_discard_timer_ms (void)
{
  
  return 15000; 
}

void
imgContainer::sDiscardTimerCallback(nsITimer *aTimer, void *aClosure)
{
  imgContainer *self = (imgContainer *) aClosure;

  NS_ASSERTION(aTimer == self->mDiscardTimer,
               "imgContainer::DiscardTimerCallback() got a callback for an unknown timer");

  self->mDiscardTimer = nsnull;

  int old_frame_count = self->mFrames.Length();

  
  if (self->mAnim) {
    return;
  }

  for (int i = 0; i < old_frame_count; ++i)
    delete self->mFrames[i];
  self->mFrames.Clear();

  self->mDiscarded = PR_TRUE;

  PR_LOG(gCompressedImageAccountingLog, PR_LOG_DEBUG,
         ("CompressedImageAccounting: discarded uncompressed image data from imgContainer %p (%s) - %d frames (cached count: %d); "
          "Compressed containers: %d, Compressed data bytes: %lld",
          self,
          self->mDiscardableMimeType.get(),
          old_frame_count,
          self->mNumFrames,
          num_containers_with_discardable_data,
          num_compressed_image_bytes));
}

nsresult
imgContainer::ResetDiscardTimer (void)
{
  if (!mRestoreDataDone)
    return NS_OK;

  if (mDiscardTimer) {
    
    nsresult rv = mDiscardTimer->Cancel();
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    mDiscardTimer = nsnull;
  }

  
  if (mAnim && mAnim->animating)
    return NS_OK;

  if (!mDiscardTimer) {
    mDiscardTimer = do_CreateInstance("@mozilla.org/timer;1");
    NS_ENSURE_TRUE(mDiscardTimer, NS_ERROR_OUT_OF_MEMORY);
  }

  return mDiscardTimer->InitWithFuncCallback(sDiscardTimerCallback,
                                             (void *) this,
                                             get_discard_timer_ms (),
                                             nsITimer::TYPE_ONE_SHOT);
}

nsresult
imgContainer::RestoreDiscardedData(void)
{
  
  
  if (!mRestoreDataDone) 
    return NS_OK;

  
  nsresult rv = ResetDiscardTimer();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mDiscarded)
    return NS_OK;

  int num_expected_frames = mNumFrames;

  
  mDiscarded = PR_FALSE;

  rv = ReloadImages();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION (mNumFrames == PRInt32(mFrames.Length()),
                "number of restored image frames doesn't match");
  NS_ASSERTION (num_expected_frames == mNumFrames,
                "number of restored image frames doesn't match the original number of frames!");
  
  PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
          ("CompressedImageAccounting: imgContainer::RestoreDiscardedData() restored discarded data "
           "for imgContainer %p (%s) - %d image frames.  "
           "Compressed containers: %d, Compressed data bytes: %lld",
           this,
           mDiscardableMimeType.get(),
           mNumFrames,
           num_containers_with_discardable_data,
           num_compressed_image_bytes));

  return NS_OK;
}


 
NS_IMETHODIMP imgContainer::Draw(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter, 
                                 gfxMatrix &aUserSpaceToImageSpace, gfxRect &aFill,
                                 nsIntRect &aSubimage)
{
  NS_ENSURE_ARG_POINTER(aContext);

  imgFrame *frame = GetCurrentImgFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  nsIntRect framerect = frame->GetRect();
  nsIntMargin padding(framerect.x, framerect.y, 
                      mSize.width - framerect.XMost(),
                      mSize.height - framerect.YMost());

  frame->Draw(aContext, aFilter, aUserSpaceToImageSpace, aFill, padding, aSubimage);

  return NS_OK;
}

class ContainerLoader : public imgILoad,
                        public imgIDecoderObserver,
                        public nsSupportsWeakReference
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_IMGILOAD
  NS_DECL_IMGIDECODEROBSERVER
  NS_DECL_IMGICONTAINEROBSERVER

  ContainerLoader(void);

private:

  nsCOMPtr<imgIContainer> mContainer;
};

NS_IMPL_ISUPPORTS4 (ContainerLoader, imgILoad, imgIDecoderObserver, imgIContainerObserver, nsISupportsWeakReference)

ContainerLoader::ContainerLoader (void)
{
}


NS_IMETHODIMP
ContainerLoader::GetImage(imgIContainer **aImage)
{
  *aImage = mContainer;
  NS_IF_ADDREF (*aImage);
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::SetImage(imgIContainer *aImage)
{
  mContainer = aImage;
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::GetIsMultiPartChannel(PRBool *aIsMultiPartChannel)
{
  *aIsMultiPartChannel = PR_FALSE; 
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::OnStartRequest(imgIRequest *aRequest)
{
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::OnStartDecode(imgIRequest *aRequest)
{
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::OnStartContainer(imgIRequest *aRequest, imgIContainer *aContainer)
{
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::OnStartFrame(imgIRequest *aRequest, PRUint32 aFrame)
{
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::OnDataAvailable(imgIRequest *aRequest, PRBool aCurrentFrame, const nsIntRect * aRect)
{
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::OnStopFrame(imgIRequest *aRequest, PRUint32 aFrame)
{
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::OnStopContainer(imgIRequest *aRequest, imgIContainer *aContainer)
{
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::OnStopDecode(imgIRequest *aRequest, nsresult status, const PRUnichar *statusArg)
{
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::OnStopRequest(imgIRequest *aRequest, PRBool aIsLastPart)
{
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::FrameChanged(imgIContainer *aContainer, nsIntRect * aDirtyRect)
{
  return NS_OK;
}

nsresult
imgContainer::ReloadImages(void)
{
  NS_ASSERTION(!mRestoreData.IsEmpty(),
               "imgContainer::ReloadImages(): mRestoreData should not be empty");
  NS_ASSERTION(mRestoreDataDone,
               "imgContainer::ReloadImages(): mRestoreDataDone shoudl be true!");

  mNumFrames = 0;
  NS_ASSERTION(mFrames.Length() == 0,
               "imgContainer::ReloadImages(): mFrames should be empty");

  nsCAutoString decoderCID(NS_LITERAL_CSTRING("@mozilla.org/image/decoder;2?type=") + mDiscardableMimeType);

  nsCOMPtr<imgIDecoder> decoder = do_CreateInstance(decoderCID.get());
  if (!decoder) {
    PR_LOG(gCompressedImageAccountingLog, PR_LOG_WARNING,
           ("CompressedImageAccounting: imgContainer::ReloadImages() could not create decoder for %s",
            mDiscardableMimeType.get()));
    return NS_IMAGELIB_ERROR_NO_DECODER;
  }

  nsCOMPtr<imgILoad> loader = new ContainerLoader();
  if (!loader) {
    PR_LOG(gCompressedImageAccountingLog, PR_LOG_WARNING,
           ("CompressedImageAccounting: imgContainer::ReloadImages() could not allocate ContainerLoader "
            "when reloading the images for container %p",
            this));
    return NS_ERROR_OUT_OF_MEMORY;
  }

  loader->SetImage(this);

  nsresult result = decoder->Init(loader);
  if (NS_FAILED(result)) {
    PR_LOG(gCompressedImageAccountingLog, PR_LOG_WARNING,
           ("CompressedImageAccounting: imgContainer::ReloadImages() image container %p "
            "failed to initialize the decoder (%s)",
            this,
            mDiscardableMimeType.get()));
    return result;
  }

  nsCOMPtr<nsIInputStream> stream;
  result = NS_NewByteInputStream(getter_AddRefs(stream), mRestoreData.Elements(), mRestoreData.Length(), NS_ASSIGNMENT_DEPEND);
  NS_ENSURE_SUCCESS(result, result);

  if (PR_LOG_TEST(gCompressedImageAccountingLog, PR_LOG_DEBUG)) {
    char buf[9];
    get_header_str(buf, mRestoreData.Elements(), mRestoreData.Length());
    PR_LOG(gCompressedImageAccountingLog, PR_LOG_WARNING,
           ("CompressedImageAccounting: imgContainer::ReloadImages() starting to restore images for container %p (%s) - "
            "header %p is 0x%s (length %d)",
            this,
            mDiscardableMimeType.get(),
            mRestoreData.Elements(),
            buf,
            mRestoreData.Length()));
  }

  
  PRUint32 written;
  (void)decoder->WriteFrom(stream, mRestoreData.Length(), &written);

  result = decoder->Flush();
  NS_ENSURE_SUCCESS(result, result);

  result = decoder->Close();
  NS_ENSURE_SUCCESS(result, result);

  NS_ASSERTION(PRInt32(mFrames.Length()) == mNumFrames,
               "imgContainer::ReloadImages(): the restored mFrames.Length() doesn't match mNumFrames!");

  return result;
}
