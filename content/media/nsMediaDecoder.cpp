





#include "nsMediaDecoder.h"
#include "MediaResource.h"

#include "nsHTMLMediaElement.h"
#include "nsError.h"

using namespace mozilla;


static const PRUint32 PROGRESS_MS = 350;


static const PRUint32 STALL_MS = 3000;







static const PRInt64 CAN_PLAY_THROUGH_MARGIN = 10;

nsMediaDecoder::nsMediaDecoder() :
  mElement(nullptr),
  mFrameBufferLength(0),
  mPinnedForSeek(false),
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
  mVideoFrameContainer = aElement->GetVideoFrameContainer();
  return true;
}

void nsMediaDecoder::Shutdown()
{
  StopProgress();
  mElement = nullptr;
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
  mProgressTimer = nullptr;

  return rv;
}

void nsMediaDecoder::FireTimeUpdate()
{
  if (!mElement)
    return;
  mElement->FireTimeUpdate(true);
}

void nsMediaDecoder::PinForSeek()
{
  MediaResource* resource = GetResource();
  if (!resource || mPinnedForSeek) {
    return;
  }
  mPinnedForSeek = true;
  resource->Pin();
}

void nsMediaDecoder::UnpinForSeek()
{
  MediaResource* resource = GetResource();
  if (!resource || !mPinnedForSeek) {
    return;
  }
  mPinnedForSeek = false;
  resource->Unpin();
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
