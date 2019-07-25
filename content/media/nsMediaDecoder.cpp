





































#include "nsMediaDecoder.h"
#include "nsMediaStream.h"

#include "prlog.h"
#include "prmem.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsThreadUtils.h"
#include "nsIDOMHTMLMediaElement.h"
#include "nsNetUtil.h"
#include "nsHTMLMediaElement.h"
#include "nsAutoLock.h"
#include "nsIRenderingContext.h"
#include "gfxContext.h"
#include "nsPresContext.h"
#include "nsDOMError.h"
#include "nsDisplayList.h"
#ifdef MOZ_SVG
#include "nsSVGEffects.h"
#endif


#define PROGRESS_MS 350


#define STALL_MS 3000

nsMediaDecoder::nsMediaDecoder() :
  mElement(0),
  mRGBWidth(-1),
  mRGBHeight(-1),
  mProgressTime(),
  mDataTime(),
  mVideoUpdateLock(nsnull),
  mPixelAspectRatio(1.0),
  mPinnedForSeek(PR_FALSE),
  mSizeChanged(PR_FALSE),
  mShuttingDown(PR_FALSE)
{
  MOZ_COUNT_CTOR(nsMediaDecoder);
}

nsMediaDecoder::~nsMediaDecoder()
{
  if (mVideoUpdateLock) {
    PR_DestroyLock(mVideoUpdateLock);
    mVideoUpdateLock = nsnull;
  }
  MOZ_COUNT_DTOR(nsMediaDecoder);
}

PRBool nsMediaDecoder::Init(nsHTMLMediaElement* aElement)
{
  mElement = aElement;
  mVideoUpdateLock = PR_NewLock();

  return mVideoUpdateLock != nsnull;
}

void nsMediaDecoder::Shutdown()
{
  StopProgress();
  mElement = nsnull;
}

nsHTMLMediaElement* nsMediaDecoder::GetMediaElement()
{
  return mElement;
}

static PRInt32 ConditionDimension(float aValue, PRInt32 aDefault)
{
  
  if (aValue >= 1.0 && aValue <= 10000.0)
    return PRInt32(NS_round(aValue));
  return aDefault;
}

void nsMediaDecoder::Invalidate()
{
  if (!mElement)
    return;

  nsIFrame* frame = mElement->GetPrimaryFrame();

  {
    nsAutoLock lock(mVideoUpdateLock);
    if (mSizeChanged) {
      nsIntSize scaledSize(mRGBWidth, mRGBHeight);
      
      
      if (mPixelAspectRatio > 1.0) {
        
        scaledSize.width =
          ConditionDimension(mPixelAspectRatio*scaledSize.width, scaledSize.width);
      } else {
        
        scaledSize.height =
          ConditionDimension(scaledSize.height/mPixelAspectRatio, scaledSize.height);
      }
      mElement->UpdateMediaSize(scaledSize);

      mSizeChanged = PR_FALSE;
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
    
    frame->InvalidateLayer(contentRect, nsDisplayItem::TYPE_VIDEO);
  }

#ifdef MOZ_SVG
  nsSVGEffects::InvalidateDirectRenderingObservers(mElement);
#endif
}

static void ProgressCallback(nsITimer* aTimer, void* aClosure)
{
  nsMediaDecoder* decoder = static_cast<nsMediaDecoder*>(aClosure);
  decoder->Progress(PR_TRUE);
}

void nsMediaDecoder::Progress(PRBool aTimer)
{
  if (!mElement)
    return;

  TimeStamp now = TimeStamp::Now();

  if (!aTimer) {
    mDataTime = now;
  }

  
  
  if ((mProgressTime.IsNull() ||
       now - mProgressTime >= TimeDuration::FromMilliseconds(PROGRESS_MS)) &&
      !mDataTime.IsNull() &&
      now - mDataTime <= TimeDuration::FromMilliseconds(PROGRESS_MS)) {
    mElement->DispatchAsyncProgressEvent(NS_LITERAL_STRING("progress"));
    mProgressTime = now;
  }

  if (!mDataTime.IsNull() &&
      now - mDataTime >= TimeDuration::FromMilliseconds(STALL_MS)) {
    mElement->DownloadStalled();
    
    mDataTime = TimeStamp();
  }
}

nsresult nsMediaDecoder::StartProgress()
{
  if (mProgressTimer)
    return NS_OK;

  mProgressTimer = do_CreateInstance("@mozilla.org/timer;1");
  return mProgressTimer->InitWithFuncCallback(ProgressCallback,
                                              this,
                                              PROGRESS_MS,
                                              nsITimer::TYPE_REPEATING_SLACK);
}

nsresult nsMediaDecoder::StopProgress()
{
  if (!mProgressTimer)
    return NS_OK;

  nsresult rv = mProgressTimer->Cancel();
  mProgressTimer = nsnull;

  return rv;
}

void nsMediaDecoder::SetVideoData(const gfxIntSize& aSize,
                                  float aPixelAspectRatio,
                                  Image* aImage)
{
  nsAutoLock lock(mVideoUpdateLock);

  if (mRGBWidth != aSize.width || mRGBHeight != aSize.height ||
      mPixelAspectRatio != aPixelAspectRatio) {
    mRGBWidth = aSize.width;
    mRGBHeight = aSize.height;
    mPixelAspectRatio = aPixelAspectRatio;
    mSizeChanged = PR_TRUE;
  }
  if (mImageContainer && aImage) {
    mImageContainer->SetCurrentImage(aImage);
  }
}

void nsMediaDecoder::PinForSeek()
{
  nsMediaStream* stream = GetCurrentStream();
  if (!stream || mPinnedForSeek) {
    return;
  }
  mPinnedForSeek = PR_TRUE;
  stream->Pin();
}

void nsMediaDecoder::UnpinForSeek()
{
  nsMediaStream* stream = GetCurrentStream();
  if (!stream || !mPinnedForSeek) {
    return;
  }
  mPinnedForSeek = PR_FALSE;
  stream->Unpin();
}




static const PRInt32 gDownloadSizeSafetyMargin = 1000000;

PRBool nsMediaDecoder::CanPlayThrough()
{
  Statistics stats = GetStatistics();
  if (!stats.mDownloadRateReliable || !stats.mPlaybackRateReliable) {
    return PR_FALSE;
  }
  PRInt64 bytesToDownload = stats.mTotalBytes - stats.mDownloadPosition;
  PRInt64 bytesToPlayback = stats.mTotalBytes - stats.mPlaybackPosition;
  double timeToDownload =
    (bytesToDownload + gDownloadSizeSafetyMargin)/stats.mDownloadRate;
  double timeToPlay = bytesToPlayback/stats.mPlaybackRate;
  return timeToDownload <= timeToPlay;
}
