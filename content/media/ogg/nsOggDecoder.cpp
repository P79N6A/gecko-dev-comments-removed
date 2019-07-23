






































#include "nsOggPlayStateMachine.h"
#include <limits>
#include "nsNetUtil.h"
#include "nsAudioStream.h"
#include "nsHTMLVideoElement.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsTArray.h"
#include "nsOggCodecState.h"
#include "nsOggDecoder.h"
#include "nsOggReader.h"

#ifdef PR_LOGGING
PRLogModuleInfo* gOggDecoderLog;
#define LOG(type, msg) PR_LOG(gOggDecoderLog, type, msg)
#else
#define LOG(type, msg)
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(nsOggDecoder, nsIObserver)

void nsOggDecoder::Pause() 
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  MonitorAutoEnter mon(mMonitor);
  if (mPlayState == PLAY_STATE_SEEKING || mPlayState == PLAY_STATE_ENDED) {
    mNextState = PLAY_STATE_PAUSED;
    return;
  }

  ChangeState(PLAY_STATE_PAUSED);
}

void nsOggDecoder::SetVolume(float volume)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mInitialVolume = volume;
  if (mDecodeStateMachine) {
    mDecodeStateMachine->SetVolume(volume);
  }
}

float nsOggDecoder::GetDuration()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mDuration >= 0) {
     return static_cast<float>(mDuration) / 1000.0;
  }
  return std::numeric_limits<float>::quiet_NaN();
}

nsOggDecoder::nsOggDecoder() :
  mDecoderPosition(0),
  mPlaybackPosition(0),
  mCurrentTime(0.0),
  mInitialVolume(0.0),
  mRequestedSeekTime(-1.0),
  mDuration(-1),
  mSeekable(PR_TRUE),
  mMonitor("media.decoder"),
  mPlayState(PLAY_STATE_PAUSED),
  mNextState(PLAY_STATE_PAUSED),
  mResourceLoaded(PR_FALSE),
  mIgnoreProgressData(PR_FALSE)
{
  MOZ_COUNT_CTOR(nsOggDecoder);
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
#ifdef PR_LOGGING
  if (!gOggDecoderLog) {
    gOggDecoderLog = PR_NewLogModule("nsOggDecoder");
  }
#endif
}

PRBool nsOggDecoder::Init(nsHTMLMediaElement* aElement)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (!nsMediaDecoder::Init(aElement))
    return PR_FALSE;

  nsContentUtils::RegisterShutdownObserver(this);
  mImageContainer = aElement->GetImageContainer();
  return PR_TRUE;
}

void nsOggDecoder::Stop()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread");

  
  
  
  if (mStateMachineThread)
    mStateMachineThread->Shutdown();

  mStateMachineThread = nsnull;
  mDecodeStateMachine = nsnull;
}

void nsOggDecoder::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  if (mShuttingDown)
    return;

  mShuttingDown = PR_TRUE;

  
  
  
  if (mDecodeStateMachine) {
    mDecodeStateMachine->Shutdown();
  }

  
  
  if (mStream) {
    mStream->Close();
  }

  ChangeState(PLAY_STATE_SHUTDOWN);
  nsMediaDecoder::Shutdown();

  
  
  
  
  
  
  
  nsCOMPtr<nsIRunnable> event =
    NS_NEW_RUNNABLE_METHOD(nsOggDecoder, this, Stop);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);

  nsContentUtils::UnregisterShutdownObserver(this);
}

nsOggDecoder::~nsOggDecoder()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  MOZ_COUNT_DTOR(nsOggDecoder);
}

nsresult nsOggDecoder::Load(nsMediaStream* aStream,
                            nsIStreamListener** aStreamListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (aStreamListener) {
    *aStreamListener = nsnull;
  }

  {
    
    
    
    MonitorAutoEnter mon(mMonitor);

    nsresult rv = aStream->Open(aStreamListener);
    if (NS_FAILED(rv)) {
      delete aStream;
      return rv;
    }

    mStream = aStream;
  }

  nsresult rv = NS_NewThread(getter_AddRefs(mStateMachineThread));
  NS_ENSURE_SUCCESS(rv, rv);

  mDecodeStateMachine = new nsOggPlayStateMachine(this);
  if (NS_FAILED(mDecodeStateMachine->Init())) {
    return NS_ERROR_FAILURE;
  }
  {
    MonitorAutoEnter mon(mMonitor);
    mDecodeStateMachine->SetSeekable(mSeekable);
    mDecodeStateMachine->SetDuration(mDuration);
  }

  ChangeState(PLAY_STATE_LOADING);

  return mStateMachineThread->Dispatch(mDecodeStateMachine, NS_DISPATCH_NORMAL);
}

nsresult nsOggDecoder::Play()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  MonitorAutoEnter mon(mMonitor);
  if (mPlayState == PLAY_STATE_SEEKING) {
    mNextState = PLAY_STATE_PLAYING;
    return NS_OK;
  }
  if (mPlayState == PLAY_STATE_ENDED)
    return Seek(0);

  ChangeState(PLAY_STATE_PLAYING);

  return NS_OK;
}

nsresult nsOggDecoder::Seek(float aTime)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  MonitorAutoEnter mon(mMonitor);

  if (aTime < 0.0)
    return NS_ERROR_FAILURE;

  mRequestedSeekTime = aTime;

  
  
  
  if (mPlayState != PLAY_STATE_SEEKING) {
    if (mPlayState == PLAY_STATE_ENDED) {
      mNextState = PLAY_STATE_PLAYING;
    } else {
      mNextState = mPlayState;
    }
    ChangeState(PLAY_STATE_SEEKING);
  }

  return NS_OK;
}

nsresult nsOggDecoder::PlaybackRateChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return NS_ERROR_NOT_IMPLEMENTED;
}

float nsOggDecoder::GetCurrentTime()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return mCurrentTime;
}

nsMediaStream* nsOggDecoder::GetCurrentStream()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return mStream;
}

already_AddRefed<nsIPrincipal> nsOggDecoder::GetCurrentPrincipal()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return mStream ? mStream->GetCurrentPrincipal() : nsnull;
}

void nsOggDecoder::MetadataLoaded()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown)
    return;

  
  
  PRBool notifyElement = PR_TRUE;
  {
    MonitorAutoEnter mon(mMonitor);
    mDuration = mDecodeStateMachine ? mDecodeStateMachine->GetDuration() : -1;
    notifyElement = mNextState != PLAY_STATE_SEEKING;
  }

  if (mElement && notifyElement) {
    
    
    Invalidate();
    mElement->MetadataLoaded();
  }

  if (!mResourceLoaded) {
    StartProgress();
  } else if (mElement) {
    
    
    mElement->DispatchAsyncProgressEvent(NS_LITERAL_STRING("progress"));
  }

  
  
  PRBool resourceIsLoaded = !mResourceLoaded && mStream &&
    mStream->IsDataCachedToEndOfStream(mDecoderPosition);
  if (mElement && notifyElement) {
    mElement->FirstFrameLoaded(resourceIsLoaded);
  }

  
  
  
  
  MonitorAutoEnter mon(mMonitor);
  if (mPlayState == PLAY_STATE_LOADING) {
    if (mRequestedSeekTime >= 0.0) {
      ChangeState(PLAY_STATE_SEEKING);
    } else {
      ChangeState(mNextState);
    }
  }

  if (resourceIsLoaded) {
    ResourceLoaded();
  }
}

void nsOggDecoder::ResourceLoaded()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  
  
  
  if (mShuttingDown)
    return;

  {
    
    
    MonitorAutoEnter mon(mMonitor);
    if (mIgnoreProgressData || mResourceLoaded || mPlayState == PLAY_STATE_LOADING)
      return;

    Progress(PR_FALSE);

    mResourceLoaded = PR_TRUE;
    StopProgress();
  }

  
  if (mElement) {
    mElement->DispatchAsyncProgressEvent(NS_LITERAL_STRING("progress"));
    mElement->ResourceLoaded();
  }
}

void nsOggDecoder::NetworkError()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown)
    return;

  if (mElement)
    mElement->NetworkError();

  Shutdown();
}

void nsOggDecoder::DecodeError()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown)
    return;

  if (mElement)
    mElement->DecodeError();

  Shutdown();
}

PRBool nsOggDecoder::IsSeeking() const
{
  return mPlayState == PLAY_STATE_SEEKING || mNextState == PLAY_STATE_SEEKING;
}

PRBool nsOggDecoder::IsEnded() const
{
  return mPlayState == PLAY_STATE_ENDED || mPlayState == PLAY_STATE_SHUTDOWN;
}

void nsOggDecoder::PlaybackEnded()
{
  if (mShuttingDown || mPlayState == nsOggDecoder::PLAY_STATE_SEEKING)
    return;

  PlaybackPositionChanged();
  ChangeState(PLAY_STATE_ENDED);

  if (mElement)  {
    UpdateReadyStateForData();
    mElement->PlaybackEnded();
  }
}

NS_IMETHODIMP nsOggDecoder::Observe(nsISupports *aSubjet,
                                    const char *aTopic,
                                    const PRUnichar *someData)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    Shutdown();
  }

  return NS_OK;
}

nsMediaDecoder::Statistics
nsOggDecoder::GetStatistics()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  Statistics result;

  MonitorAutoEnter mon(mMonitor);
  if (mStream) {
    result.mDownloadRate = 
      mStream->GetDownloadRate(&result.mDownloadRateReliable);
    result.mDownloadPosition =
      mStream->GetCachedDataEnd(mDecoderPosition);
    result.mTotalBytes = mStream->GetLength();
    result.mPlaybackRate = ComputePlaybackRate(&result.mPlaybackRateReliable);
    result.mDecoderPosition = mDecoderPosition;
    result.mPlaybackPosition = mPlaybackPosition;
  } else {
    result.mDownloadRate = 0;
    result.mDownloadRateReliable = PR_TRUE;
    result.mPlaybackRate = 0;
    result.mPlaybackRateReliable = PR_TRUE;
    result.mDecoderPosition = 0;
    result.mPlaybackPosition = 0;
    result.mDownloadPosition = 0;
    result.mTotalBytes = 0;
  }

  return result;
}

double nsOggDecoder::ComputePlaybackRate(PRPackedBool* aReliable)
{
  GetMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(NS_IsMainThread() || IsThread(mStateMachineThread),
               "Should be on main or state machine thread.");

  PRInt64 length = mStream ? mStream->GetLength() : -1;
  if (mDuration >= 0 && length >= 0) {
    *aReliable = PR_TRUE;
    return double(length)*1000.0/mDuration;
  }
  return mPlaybackStatistics.GetRateAtLastStop(aReliable);
}

void nsOggDecoder::UpdatePlaybackRate()
{
  NS_ASSERTION(NS_IsMainThread() || IsThread(mStateMachineThread),
               "Should be on main or state machine thread.");
  GetMonitor().AssertCurrentThreadIn();
  if (!mStream)
    return;
  PRPackedBool reliable;
  PRUint32 rate = PRUint32(ComputePlaybackRate(&reliable));
  if (reliable) {
    
    rate = NS_MAX(rate, 1u);
  } else {
    
    
    rate = NS_MAX(rate, 10000u);
  }
  mStream->SetPlaybackRate(rate);
}

void nsOggDecoder::NotifySuspendedStatusChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (!mStream)
    return;
  if (mStream->IsSuspendedByCache() && mElement) {
    
    
    mElement->NotifyAutoplayDataReady();
  }
}

void nsOggDecoder::NotifyBytesDownloaded()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  UpdateReadyStateForData();
  Progress(PR_FALSE);
}

void nsOggDecoder::NotifyDownloadEnded(nsresult aStatus)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (aStatus == NS_BINDING_ABORTED)
    return;

  {
    MonitorAutoEnter mon(mMonitor);
    UpdatePlaybackRate();
  }

  if (NS_SUCCEEDED(aStatus)) {
    ResourceLoaded();
  } else if (aStatus != NS_BASE_STREAM_CLOSED) {
    NetworkError();
  }
  UpdateReadyStateForData();
}

void nsOggDecoder::NotifyBytesConsumed(PRInt64 aBytes)
{
  MonitorAutoEnter mon(mMonitor);
  NS_ASSERTION(mDecodeStateMachine->OnStateMachineThread() || mDecodeStateMachine->OnDecodeThread(),
               "Should be on play state machine or decode thread.");
  if (!mIgnoreProgressData) {
    mDecoderPosition += aBytes;
  }
}

void nsOggDecoder::NextFrameUnavailableBuffering()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
  if (!mElement || mShuttingDown || !mDecodeStateMachine)
    return;

  mElement->UpdateReadyStateForData(nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE_BUFFERING);
}

void nsOggDecoder::NextFrameAvailable()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
  if (!mElement || mShuttingDown || !mDecodeStateMachine)
    return;

  mElement->UpdateReadyStateForData(nsHTMLMediaElement::NEXT_FRAME_AVAILABLE);
}

void nsOggDecoder::NextFrameUnavailable()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
  if (!mElement || mShuttingDown || !mDecodeStateMachine)
    return;

  mElement->UpdateReadyStateForData(nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE);
}

void nsOggDecoder::UpdateReadyStateForData()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
  if (!mElement || mShuttingDown || !mDecodeStateMachine)
    return;

  nsHTMLMediaElement::NextFrameStatus frameStatus =
    mDecodeStateMachine->GetNextFrameStatus();
  mElement->UpdateReadyStateForData(frameStatus);
}

void nsOggDecoder::SeekingStopped()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (mShuttingDown)
    return;

  {
    MonitorAutoEnter mon(mMonitor);

    
    
    if (mRequestedSeekTime >= 0.0)
      ChangeState(PLAY_STATE_SEEKING);
    else
      ChangeState(mNextState);
  }

  if (mElement) {
    UpdateReadyStateForData();
    mElement->SeekCompleted();
  }
}



void nsOggDecoder::SeekingStoppedAtEnd()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (mShuttingDown)
    return;

  PRBool fireEnded = PR_FALSE;
  {
    MonitorAutoEnter mon(mMonitor);

    
    
    if (mRequestedSeekTime >= 0.0) {
      ChangeState(PLAY_STATE_SEEKING);
    } else {
      fireEnded = mNextState != PLAY_STATE_PLAYING;
      ChangeState(fireEnded ? PLAY_STATE_ENDED : mNextState);
    }
  }

  if (mElement) {
    UpdateReadyStateForData();
    mElement->SeekCompleted();
    if (fireEnded) {
      mElement->PlaybackEnded();
    }
  }
}

void nsOggDecoder::SeekingStarted()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown)
    return;

  if (mElement) {
    UpdateReadyStateForData();
    mElement->SeekStarted();
  }
}

void nsOggDecoder::ChangeState(PlayState aState)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");   
  MonitorAutoEnter mon(mMonitor);

  if (mNextState == aState) {
    mNextState = PLAY_STATE_PAUSED;
  }

  if (mPlayState == PLAY_STATE_SHUTDOWN) {
    mMonitor.NotifyAll();
    return;
  }

  mPlayState = aState;
  switch (aState) {
  case PLAY_STATE_PAUSED:
    
    break;
  case PLAY_STATE_PLAYING:
    mDecodeStateMachine->Decode();
    break;
  case PLAY_STATE_SEEKING:
    mDecodeStateMachine->Seek(mRequestedSeekTime);
    mRequestedSeekTime = -1.0;
    break;
  case PLAY_STATE_LOADING:
    
    break;
  case PLAY_STATE_START:
    
    break;
  case PLAY_STATE_ENDED:
    
    break;
  case PLAY_STATE_SHUTDOWN:
    
    break;
  }
  mMonitor.NotifyAll();
}

void nsOggDecoder::PlaybackPositionChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown)
    return;

  float lastTime = mCurrentTime;

  
  
  {
    MonitorAutoEnter mon(mMonitor);

    if (mDecodeStateMachine) {
      mCurrentTime = mDecodeStateMachine->GetCurrentTime();
      mDecodeStateMachine->ClearPositionChangeFlag();
    }
  }

  
  
  
  
  Invalidate();

  if (mElement && lastTime != mCurrentTime) {
    mElement->DispatchSimpleEvent(NS_LITERAL_STRING("timeupdate"));
  }
}

void nsOggDecoder::DurationChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  MonitorAutoEnter mon(mMonitor);
  PRInt64 oldDuration = mDuration;
  mDuration = mDecodeStateMachine ? mDecodeStateMachine->GetDuration() : -1;
  if (mElement && oldDuration != mDuration) {
    LOG(PR_LOG_DEBUG, ("%p duration changed to %lldms", this, mDuration));
    mElement->DispatchSimpleEvent(NS_LITERAL_STRING("durationchange"));
  }
}

void nsOggDecoder::SetDuration(PRInt64 aDuration)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDuration = aDuration;
  if (mDecodeStateMachine) {
    MonitorAutoEnter mon(mMonitor);
    mDecodeStateMachine->SetDuration(mDuration);
    UpdatePlaybackRate();
  }
}

void nsOggDecoder::SetSeekable(PRBool aSeekable)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mSeekable = aSeekable;
  if (mDecodeStateMachine) {
    MonitorAutoEnter mon(mMonitor);
    mDecodeStateMachine->SetSeekable(aSeekable);
  }
}

PRBool nsOggDecoder::GetSeekable()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return mSeekable;
}

void nsOggDecoder::Suspend()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mStream) {
    mStream->Suspend(PR_TRUE);
  }
}

void nsOggDecoder::Resume()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mStream) {
    mStream->Resume();
  }
}

void nsOggDecoder::StopProgressUpdates()
{
  NS_ASSERTION(IsThread(mStateMachineThread), "Should be on state machine thread.");
  mIgnoreProgressData = PR_TRUE;
  if (mStream) {
    mStream->SetReadMode(nsMediaCacheStream::MODE_METADATA);
  }
}

void nsOggDecoder::StartProgressUpdates()
{
  NS_ASSERTION(IsThread(mStateMachineThread), "Should be on state machine thread.");
  mIgnoreProgressData = PR_FALSE;
  if (mStream) {
    mStream->SetReadMode(nsMediaCacheStream::MODE_PLAYBACK);
    mDecoderPosition = mPlaybackPosition = mStream->Tell();
  }
}

void nsOggDecoder::MoveLoadsToBackground()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mStream) {
    mStream->MoveLoadsToBackground();
  }
}
