





































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
#include "gfxContext.h"
#include "nsPresContext.h"
#include "nsDOMError.h"
#include "nsDisplayList.h"
#include "nsSVGEffects.h"
#include "VideoUtils.h"

using namespace mozilla;


static const PRUint32 PROGRESS_MS = 350;


static const PRUint32 STALL_MS = 3000;







static const PRInt64 CAN_PLAY_THROUGH_MARGIN = 10;

nsMediaDecoder::nsMediaDecoder() :
  mElement(nsnull),
  mRGBWidth(-1),
  mRGBHeight(-1),
  mVideoUpdateLock("nsMediaDecoder.mVideoUpdateLock"),
  mFrameBufferLength(0),
  mPinnedForSeek(false),
  mSizeChanged(false),
  mImageContainerSizeChanged(false),
  mShuttingDown(false)
{
  MOZ_COUNT_CTOR(nsMediaDecoder);
  MediaMemoryReporter::AddMediaDecoder(this);
}

nsMediaDecoder::~nsMediaDecoder()
{
  MOZ_COUNT_DTOR(nsMediaDecoder);
  MediaMemoryReporter::RemoveMediaDecoder(this);
}

bool nsMediaDecoder::Init(nsHTMLMediaElement* aElement)
{
  mElement = aElement;
  return true;
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

nsresult nsMediaDecoder::RequestFrameBufferLength(PRUint32 aLength)
{
  if (aLength < FRAMEBUFFER_LENGTH_MIN || aLength > FRAMEBUFFER_LENGTH_MAX) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  mFrameBufferLength = aLength;
  return NS_OK;
}

void nsMediaDecoder::Invalidate()
{
  if (!mElement)
    return;

  nsIFrame* frame = mElement->GetPrimaryFrame();
  bool invalidateFrame = false;

  {
    MutexAutoLock lock(mVideoUpdateLock);

    
    invalidateFrame = mImageContainerSizeChanged;
    mImageContainerSizeChanged = false;

    if (mSizeChanged) {
      mElement->UpdateMediaSize(nsIntSize(mRGBWidth, mRGBHeight));
      mSizeChanged = false;

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

static void ProgressCallback(nsITimer* aTimer, void* aClosure)
{
  nsMediaDecoder* decoder = static_cast<nsMediaDecoder*>(aClosure);
  decoder->Progress(true);
}

void nsMediaDecoder::Progress(bool aTimer)
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
    mElement->DispatchAsyncEvent(NS_LITERAL_STRING("progress"));
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

void nsMediaDecoder::FireTimeUpdate()
{
  if (!mElement)
    return;
  mElement->FireTimeUpdate(true);
}

void nsMediaDecoder::SetVideoData(const gfxIntSize& aSize,
                                  Image* aImage,
                                  TimeStamp aTarget)
{
  MutexAutoLock lock(mVideoUpdateLock);

  if (mRGBWidth != aSize.width || mRGBHeight != aSize.height) {
    mRGBWidth = aSize.width;
    mRGBHeight = aSize.height;
    mSizeChanged = true;
  }
  if (mImageContainer && aImage) {
    gfxIntSize oldFrameSize = mImageContainer->GetCurrentSize();

    TimeStamp paintTime = mImageContainer->GetPaintTime();
    if (!paintTime.IsNull() && !mPaintTarget.IsNull()) {
      mPaintDelay = paintTime - mPaintTarget;
    }

    mImageContainer->SetCurrentImage(aImage);
    gfxIntSize newFrameSize = mImageContainer->GetCurrentSize();
    if (oldFrameSize != newFrameSize) {
      mImageContainerSizeChanged = true;
    }
  }

  mPaintTarget = aTarget;
}

double nsMediaDecoder::GetFrameDelay()
{
  MutexAutoLock lock(mVideoUpdateLock);
  return mPaintDelay.ToSeconds();
}

void nsMediaDecoder::PinForSeek()
{
  nsMediaStream* stream = GetCurrentStream();
  if (!stream || mPinnedForSeek) {
    return;
  }
  mPinnedForSeek = true;
  stream->Pin();
}

void nsMediaDecoder::UnpinForSeek()
{
  nsMediaStream* stream = GetCurrentStream();
  if (!stream || !mPinnedForSeek) {
    return;
  }
  mPinnedForSeek = false;
  stream->Unpin();
}

bool nsMediaDecoder::CanPlayThrough()
{
  Statistics stats = GetStatistics();
  if (!stats.mDownloadRateReliable || !stats.mPlaybackRateReliable) {
    return false;
  }
  PRInt64 bytesToDownload = stats.mTotalBytes - stats.mDownloadPosition;
  PRInt64 bytesToPlayback = stats.mTotalBytes - stats.mPlaybackPosition;
  double timeToDownload = bytesToDownload / stats.mDownloadRate;
  double timeToPlay = bytesToPlayback / stats.mPlaybackRate;

  if (timeToDownload > timeToPlay) {
    
    
    return false;
  }

  
  
  
  
  
  
  
  PRInt64 readAheadMargin =
    static_cast<PRInt64>(stats.mPlaybackRate * CAN_PLAY_THROUGH_MARGIN);
  return stats.mTotalBytes == stats.mDownloadPosition ||
         stats.mDownloadPosition > stats.mPlaybackPosition + readAheadMargin;
}

namespace mozilla {

MediaMemoryReporter* MediaMemoryReporter::sUniqueInstance;

NS_MEMORY_REPORTER_IMPLEMENT(MediaDecodedVideoMemory,
                             "explicit/media/decoded-video",
                             KIND_HEAP,
                             UNITS_BYTES,
                             MediaMemoryReporter::GetDecodedVideoMemory,
                             "Memory used by decoded video frames.")

NS_MEMORY_REPORTER_IMPLEMENT(MediaDecodedAudioMemory,
                             "explicit/media/decoded-audio",
                             KIND_HEAP,
                             UNITS_BYTES,
                             MediaMemoryReporter::GetDecodedAudioMemory,
                             "Memory used by decoded audio chunks.")

MediaMemoryReporter::MediaMemoryReporter()
  : mMediaDecodedVideoMemory(new NS_MEMORY_REPORTER_NAME(MediaDecodedVideoMemory))
  , mMediaDecodedAudioMemory(new NS_MEMORY_REPORTER_NAME(MediaDecodedAudioMemory))
{
  NS_RegisterMemoryReporter(mMediaDecodedVideoMemory);
  NS_RegisterMemoryReporter(mMediaDecodedAudioMemory);
}

MediaMemoryReporter::~MediaMemoryReporter()
{
  NS_UnregisterMemoryReporter(mMediaDecodedVideoMemory);
  NS_UnregisterMemoryReporter(mMediaDecodedAudioMemory);
}

} 
