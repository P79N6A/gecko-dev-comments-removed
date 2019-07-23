










































#include "nsComponentManagerUtils.h"
#include "imgIContainerObserver.h"
#include "ImageErrors.h"
#include "nsIImage.h"
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

nsresult imgContainer::GetCurrentFrameNoRef(gfxIImageFrame **aFrame)
{
  nsresult result;

  result = RestoreDiscardedData();
  if (NS_FAILED (result)) {
    PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
            ("CompressedImageAccounting: imgContainer::GetCurrentFrameNoRef(): error %d in RestoreDiscardedData(); "
             "returning a null frame from imgContainer %p",
             result,
             this));

    *aFrame = nsnull;
    return result;
  }

  if (!mAnim)
    *aFrame = mFrames.SafeObjectAt(0);
  else if (mAnim->lastCompositedFrameIndex == mAnim->currentAnimationFrameIndex)
    *aFrame = mAnim->compositingFrame;
  else
    *aFrame = mFrames.SafeObjectAt(mAnim->currentAnimationFrameIndex);

  if (!*aFrame)
    PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
            ("CompressedImageAccounting: imgContainer::GetCurrentFrameNoRef(): returning null frame from imgContainer %p "
             "(no errors when restoring data)",
              this));

  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetCurrentFrame(gfxIImageFrame **aCurrentFrame)
{
  nsresult result;

  NS_ASSERTION(aCurrentFrame, "imgContainer::GetCurrentFrame; Invalid Arg");
  if (!aCurrentFrame)
    return NS_ERROR_INVALID_POINTER;
  
  result = GetCurrentFrameNoRef (aCurrentFrame);
  if (NS_FAILED (result))
    return result;

  if (!*aCurrentFrame)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aCurrentFrame);
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetNumFrames(PRUint32 *aNumFrames)
{
  NS_ASSERTION(aNumFrames, "imgContainer::GetNumFrames; Invalid Arg");
  if (!aNumFrames)
    return NS_ERROR_INVALID_ARG;

  *aNumFrames = mNumFrames;
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetFrameAt(PRUint32 index, gfxIImageFrame **_retval)
{
  nsresult result;

  NS_ASSERTION(_retval, "imgContainer::GetFrameAt; Invalid Arg");
  if (!_retval)
    return NS_ERROR_INVALID_POINTER;

  if (mNumFrames == 0) {
    *_retval = nsnull;
    return NS_OK;
  }

  NS_ENSURE_ARG((int) index < mNumFrames);

  result = RestoreDiscardedData ();
  if (NS_FAILED (result)) {
    *_retval = nsnull;
    return result;
  }
  
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

  if (mFrames.Count() == 0) {
    
    mFrames.AppendObject(item);

    mNumFrames++;

    return NS_OK;
  }
  
  if (mFrames.Count() == 1) {
    
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

  mNumFrames++;
  
  
  
  
  if (mFrames.Count() == 2)
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
  
  
  if (mNumFrames == 1)
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
    
    PRInt32 timeout;
    nsresult result;
    gfxIImageFrame *currentFrame;

    result = GetCurrentFrameNoRef (&currentFrame);
    if (NS_FAILED (result))
      return result;

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
    if (NS_FAILED(rv))
      return rv;
  }

  mAnim->lastCompositedFrameIndex = -1;
  mAnim->currentAnimationFrameIndex = 0;
  
  nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));
  if (observer) {
    nsresult result;

    result = RestoreDiscardedData ();
    if (NS_FAILED (result))
      return result;

    observer->FrameChanged(this, mFrames[0], &(mAnim->firstFrameRefreshArea));
  }

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
  NS_ASSERTION(aMimeType, "imgContainer::SetDiscardable() called with null aMimeType");

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
  NS_ASSERTION(aBuffer, "imgContainer::AddRestoreData() called with null aBuffer");

  if (!DiscardingEnabled ())
    return NS_OK;

  if (!mDiscardable) {
    NS_WARNING ("imgContainer::AddRestoreData() can only be called if SetDiscardable is called first");
    return NS_ERROR_FAILURE;
  }

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

  if (!DiscardingEnabled ())
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
             mFrames.Count (),
             mNumFrames,
             mRestoreData.Elements(),
             buf,
             mRestoreData.Length()));
  }

  return ResetDiscardTimer();
}



NS_IMETHODIMP imgContainer::Notify(nsITimer *timer)
{
  nsresult result;

  result = RestoreDiscardedData();
  if (NS_FAILED (result))
    return result;

  
  
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

  if (mNumFrames == 0)
    return NS_OK;
  
  gfxIImageFrame *nextFrame = nsnull;
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
    nextFrame->GetTimeout(&timeout);

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
  nsIntRect prevFrameRect;
  aPrevFrame->GetRect(prevFrameRect);
  PRBool isFullPrevFrame = (prevFrameRect.x == 0 && prevFrameRect.y == 0 &&
                            prevFrameRect.width == mSize.width &&
                            prevFrameRect.height == mSize.height);

  
  
  if (isFullPrevFrame && 
      (prevFrameDisposalMethod == imgIContainer::kDisposeClear))
    prevFrameDisposalMethod = imgIContainer::kDisposeClearAll;

  PRInt32 nextFrameDisposalMethod;
  nsIntRect nextFrameRect;
  aNextFrame->GetFrameDisposalMethod(&nextFrameDisposalMethod);
  aNextFrame->GetRect(nextFrameRect);
  PRBool isFullNextFrame = (nextFrameRect.x == 0 && nextFrameRect.y == 0 &&
                            nextFrameRect.width == mSize.width &&
                            nextFrameRect.height == mSize.height);

  gfx_format nextFormat;
  aNextFrame->GetFormat(&nextFormat);
  if (nextFormat != gfxIFormats::PAL && nextFormat != gfxIFormats::PAL_A1) {
    
    
    if (prevFrameDisposalMethod == imgIContainer::kDisposeClearAll) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      *aFrameToUse = aNextFrame;
      return NS_OK;
    }
  
    
    
    if (isFullNextFrame &&
        (nextFrameDisposalMethod != imgIContainer::kDisposeRestorePrevious) &&
        (nextFormat == gfxIFormats::RGB)) {
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
  } else if (aNextFrameIndex == 1) {
    
    needToBlankComposite = PR_TRUE;
  }

  
  PRBool doDisposal = PR_TRUE;
  if ((nextFormat == gfxIFormats::RGB)||(nextFormat == gfxIFormats::PAL)) {
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
          gfx_format prevFormat;
          aPrevFrame->GetFormat(&prevFormat);
          if (isFullPrevFrame && 
              prevFormat != gfxIFormats::PAL && prevFormat != gfxIFormats::PAL_A1) {
            
            CopyFrameImage(aPrevFrame, mAnim->compositingFrame);
          } else {
            if (needToBlankComposite) {
              
              if (!isFullPrevFrame ||
                  (prevFormat != gfxIFormats::RGB && prevFormat != gfxIFormats::PAL)) {
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

  
  DrawFrameTo(aNextFrame, mAnim->compositingFrame, nextFrameRect);
  
  
  PRInt32 timeout;
  aNextFrame->GetTimeout(&timeout);
  mAnim->compositingFrame->SetTimeout(timeout);

  
  nsIntRect r;
  mAnim->compositingFrame->GetRect(r);
  nsCOMPtr<nsIImage> img = do_GetInterface(mAnim->compositingFrame);
  img->ImageUpdated(nsnull, nsImageUpdateFlags_kBitsChanged, &r);

  
  if (isFullNextFrame && mAnimationMode == kNormalAnimMode && mLoopCount != 0 &&
      nextFormat != gfxIFormats::PAL && nextFormat != gfxIFormats::PAL_A1) {
    
    
    
    
    
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



void imgContainer::ClearFrame(gfxIImageFrame *aFrame)
{
  if (!aFrame)
    return;

  nsCOMPtr<nsIImage> img(do_GetInterface(aFrame));
  nsRefPtr<gfxASurface> surf;
  img->GetSurface(getter_AddRefs(surf));

  
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Paint();
}


void imgContainer::ClearFrame(gfxIImageFrame *aFrame, nsIntRect &aRect)
{
  if (!aFrame || aRect.width <= 0 || aRect.height <= 0) {
    return;
  }

  nsCOMPtr<nsIImage> img(do_GetInterface(aFrame));
  nsRefPtr<gfxASurface> surf;
  img->GetSurface(getter_AddRefs(surf));

  
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Rectangle(gfxRect(aRect.x, aRect.y, aRect.width, aRect.height));
  ctx.Fill();
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

  return PR_TRUE;
}


nsresult imgContainer::DrawFrameTo(gfxIImageFrame *aSrc,
                                   gfxIImageFrame *aDst, 
                                   nsIntRect& aDstRect)
{
  if (!aSrc || !aDst)
    return NS_ERROR_NOT_INITIALIZED;

  nsIntRect srcRect, dstRect;
  aSrc->GetRect(srcRect);
  aDst->GetRect(dstRect);

  gfx_format format;
  aSrc->GetFormat(&format);
  if (format == gfxIFormats::PAL || format == gfxIFormats::PAL_A1) {
    
    if ((aDstRect.x > dstRect.width) || (aDstRect.y > dstRect.height)) {
      return NS_OK;
    }
    
    PRInt32 width = (PRUint32)aDstRect.width;
    PRInt32 height = (PRUint32)aDstRect.height;
    if (aDstRect.x + aDstRect.width > dstRect.width) {
      width = dstRect.width - aDstRect.x;
    }
    if (aDstRect.y + aDstRect.height > dstRect.height) {
      height = dstRect.height - aDstRect.y;
    }
    
    NS_ASSERTION((aDstRect.x >= 0) && (aDstRect.y >= 0) &&
                 (aDstRect.x + width <= dstRect.width) &&
                 (aDstRect.y + height <= dstRect.height),
                "imgContainer::DrawFrameTo: Invalid aDstRect");

    
    NS_ASSERTION((width <= srcRect.width) && (height <= srcRect.height),
                 "imgContainer::DrawFrameTo: source must be smaller than dest");

    if (NS_FAILED(aDst->LockImageData()))
      return NS_ERROR_FAILURE;
    
    PRUint32 size;
    PRUint8 *srcPixels;
    gfx_color *colormap;
    gfx_color *dstPixels;

    aSrc->GetImageData(&srcPixels, &size);
    aDst->GetImageData((PRUint8**)&dstPixels, &size);
    aSrc->GetPaletteData(&colormap, &size);
    if (!srcPixels || !dstPixels || !colormap) {
      aDst->UnlockImageData();
      return NS_ERROR_FAILURE;
    }

    
    dstPixels += aDstRect.x + (aDstRect.y * dstRect.width);
    if (format == gfxIFormats::PAL) {
      for (PRInt32 r = height; r > 0; --r) {
        for (PRInt32 c = 0; c < width; c++) {
          dstPixels[c] = colormap[srcPixels[c]];
        }
        
        srcPixels += srcRect.width;
        dstPixels += dstRect.width;
      }
    } else {
      
      for (PRInt32 r = height; r > 0; --r) {
        for (PRInt32 c = 0; c < width; c++) {
          const PRUint32 color = colormap[srcPixels[c]];
          if (color)
            dstPixels[c] = color;
        }
        
        srcPixels += srcRect.width;
        dstPixels += dstRect.width;
      }
    }
    aDst->UnlockImageData();
    return NS_OK;
  }

  nsCOMPtr<nsIImage> srcImg(do_GetInterface(aSrc));
  nsRefPtr<gfxASurface> srcSurf;
  srcImg->GetSurface(getter_AddRefs(srcSurf));

  nsCOMPtr<nsIImage> dstImg(do_GetInterface(aDst));
  nsRefPtr<gfxASurface> dstSurf;
  dstImg->GetSurface(getter_AddRefs(dstSurf));

  gfxContext dst(dstSurf);
  
  
  PRInt32 blendMethod;
  aSrc->GetBlendMethod(&blendMethod);
  gfxContext::GraphicsOperator defaultOperator = dst.CurrentOperator();
  if (blendMethod == imgIContainer::kBlendSource) {
    dst.SetOperator(gfxContext::OPERATOR_CLEAR);
    dst.Rectangle(gfxRect(aDstRect.x, aDstRect.y, aDstRect.width, aDstRect.height));
    dst.Fill();
  }
  
  dst.NewPath();
  dst.SetOperator(defaultOperator);
  
  
  
  dst.Translate(gfxPoint(aDstRect.x, aDstRect.y));
  dst.Rectangle(gfxRect(0, 0, aDstRect.width, aDstRect.height), PR_TRUE);
  dst.Scale(double(aDstRect.width) / srcRect.width, 
            double(aDstRect.height) / srcRect.height);
  dst.SetSource(srcSurf);
  dst.Paint();

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
  
  return 45000; 
}

void
imgContainer::sDiscardTimerCallback(nsITimer *aTimer, void *aClosure)
{
  imgContainer *self = (imgContainer *) aClosure;
  int old_frame_count;

  NS_ASSERTION(aTimer == self->mDiscardTimer,
               "imgContainer::DiscardTimerCallback() got a callback for an unknown timer");

  self->mDiscardTimer = nsnull;

  old_frame_count = self->mFrames.Count();

  if (self->mAnim) {
    delete self->mAnim;
    self->mAnim = nsnull;
  }

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
  if (!DiscardingEnabled())
    return NS_OK;

  if (!mDiscardTimer) {
    mDiscardTimer = do_CreateInstance("@mozilla.org/timer;1");

    if (!mDiscardTimer)
      return NS_ERROR_OUT_OF_MEMORY;
  } else {
    if (NS_FAILED(mDiscardTimer->Cancel()))
      return NS_ERROR_FAILURE;
  }

  return mDiscardTimer->InitWithFuncCallback(sDiscardTimerCallback,
                                             (void *) this,
                                             get_discard_timer_ms (),
                                             nsITimer::TYPE_ONE_SHOT);
}

nsresult
imgContainer::RestoreDiscardedData(void)
{
  nsresult result;
  int num_expected_frames;

  if (!mDiscardable)
    return NS_OK;

  result = ResetDiscardTimer();
  if (NS_FAILED (result))
    return result;

  if (!mDiscarded)
    return NS_OK;

  num_expected_frames = mNumFrames;

  result = ReloadImages ();
  if (NS_FAILED (result)) {
    PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
            ("CompressedImageAccounting: imgContainer::RestoreDiscardedData() for container %p failed to ReloadImages()",
             this));
    return result;
  }

  mDiscarded = PR_FALSE;

  NS_ASSERTION (mNumFrames == mFrames.Count(),
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
ContainerLoader::OnStartFrame(imgIRequest *aRequest, gfxIImageFrame *aFrame)
{
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::OnDataAvailable(imgIRequest *aRequest, gfxIImageFrame *aFrame, const nsIntRect * aRect)
{
  return NS_OK;
}


NS_IMETHODIMP
ContainerLoader::OnStopFrame(imgIRequest *aRequest, gfxIImageFrame *aFrame)
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
ContainerLoader::FrameChanged(imgIContainer *aContainer, gfxIImageFrame *aFrame, nsIntRect * aDirtyRect)
{
  return NS_OK;
}

nsresult
imgContainer::ReloadImages(void)
{
  nsresult result = NS_ERROR_FAILURE;
  nsCOMPtr<nsIInputStream> stream;

  NS_ASSERTION(!mRestoreData.IsEmpty(),
               "imgContainer::ReloadImages(): mRestoreData should not be empty");
  NS_ASSERTION(mRestoreDataDone,
               "imgContainer::ReloadImages(): mRestoreDataDone shoudl be true!");

  mNumFrames = 0;
  NS_ASSERTION(mFrames.Count() == 0,
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

  result = decoder->Init(loader);
  if (NS_FAILED(result)) {
    PR_LOG(gCompressedImageAccountingLog, PR_LOG_WARNING,
           ("CompressedImageAccounting: imgContainer::ReloadImages() image container %p "
            "failed to initialize the decoder (%s)",
            this,
            mDiscardableMimeType.get()));
    return result;
  }

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
  result = decoder->WriteFrom(stream, mRestoreData.Length(), &written);
  NS_ENSURE_SUCCESS(result, result);

  if (NS_FAILED(decoder->Flush()))
    return result;

  result = decoder->Close();
  NS_ENSURE_SUCCESS(result, result);

  NS_ASSERTION(mFrames.Count() == mNumFrames,
               "imgContainer::ReloadImages(): the restored mFrames.Count() doesn't match mNumFrames!");

  return result;
}
