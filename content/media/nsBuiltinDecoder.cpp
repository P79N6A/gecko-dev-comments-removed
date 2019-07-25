






































#include <limits>
#include "nsNetUtil.h"
#include "nsAudioStream.h"
#include "nsHTMLVideoElement.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsTArray.h"
#include "VideoUtils.h"
#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "nsTimeRanges.h"
#include "nsContentUtils.h"

using namespace mozilla;

#ifdef PR_LOGGING
PRLogModuleInfo* gBuiltinDecoderLog;
#define LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#else
#define LOG(type, msg)
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(nsBuiltinDecoder, nsIObserver)

void nsBuiltinDecoder::Pause() 
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  if (mPlayState == PLAY_STATE_SEEKING || mPlayState == PLAY_STATE_ENDED) {
    mNextState = PLAY_STATE_PAUSED;
    return;
  }

  ChangeState(PLAY_STATE_PAUSED);
}

void nsBuiltinDecoder::SetVolume(double aVolume)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mInitialVolume = aVolume;
  if (mDecoderStateMachine) {
    mDecoderStateMachine->SetVolume(aVolume);
  }
}

double nsBuiltinDecoder::GetDuration()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mInfiniteStream) {
    return std::numeric_limits<double>::infinity();
  }
  if (mDuration >= 0) {
     return static_cast<double>(mDuration) / static_cast<double>(USECS_PER_S);
  }
  return std::numeric_limits<double>::quiet_NaN();
}

void nsBuiltinDecoder::SetInfinite(PRBool aInfinite)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mInfiniteStream = aInfinite;
}

PRBool nsBuiltinDecoder::IsInfinite()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return mInfiniteStream;
}

nsBuiltinDecoder::nsBuiltinDecoder() :
  mDecoderPosition(0),
  mPlaybackPosition(0),
  mCurrentTime(0.0),
  mInitialVolume(0.0),
  mRequestedSeekTime(-1.0),
  mDuration(-1),
  mSeekable(PR_TRUE),
  mReentrantMonitor("media.decoder"),
  mPlayState(PLAY_STATE_PAUSED),
  mNextState(PLAY_STATE_PAUSED),
  mResourceLoaded(PR_FALSE),
  mIgnoreProgressData(PR_FALSE),
  mInfiniteStream(PR_FALSE)
{
  MOZ_COUNT_CTOR(nsBuiltinDecoder);
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
#ifdef PR_LOGGING
  if (!gBuiltinDecoderLog) {
    gBuiltinDecoderLog = PR_NewLogModule("nsBuiltinDecoder");
  }
#endif
}

PRBool nsBuiltinDecoder::Init(nsHTMLMediaElement* aElement)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (!nsMediaDecoder::Init(aElement))
    return PR_FALSE;

  nsContentUtils::RegisterShutdownObserver(this);
  mImageContainer = aElement->GetImageContainer();
  return PR_TRUE;
}

void nsBuiltinDecoder::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  if (mShuttingDown)
    return;

  mShuttingDown = PR_TRUE;

  
  
  
  if (mDecoderStateMachine) {
    mDecoderStateMachine->Shutdown();
  }

  
  
  if (mStream) {
    mStream->Close();
  }

  ChangeState(PLAY_STATE_SHUTDOWN);
  nsMediaDecoder::Shutdown();

  nsContentUtils::UnregisterShutdownObserver(this);
}

nsBuiltinDecoder::~nsBuiltinDecoder()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  UnpinForSeek();
  MOZ_COUNT_DTOR(nsBuiltinDecoder);
}

nsresult nsBuiltinDecoder::Load(nsMediaStream* aStream,
                                nsIStreamListener** aStreamListener,
                                nsMediaDecoder* aCloneDonor)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (aStreamListener) {
    *aStreamListener = nsnull;
  }

  {
    
    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    nsresult rv = aStream->Open(aStreamListener);
    if (NS_FAILED(rv)) {
      delete aStream;
      return rv;
    }

    mStream = aStream;
  }

  mDecoderStateMachine = CreateStateMachine();
  if (!mDecoderStateMachine) {
    return NS_ERROR_FAILURE;
  }

  nsBuiltinDecoder* cloneDonor = static_cast<nsBuiltinDecoder*>(aCloneDonor);
  if (NS_FAILED(mDecoderStateMachine->Init(cloneDonor ?
                                           cloneDonor->mDecoderStateMachine : nsnull))) {
    return NS_ERROR_FAILURE;
  }
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mDecoderStateMachine->SetSeekable(mSeekable);
    mDecoderStateMachine->SetDuration(mDuration);
    
    if (mFrameBufferLength > 0) {
      
      mDecoderStateMachine->SetFrameBufferLength(mFrameBufferLength);
    }
  }

  ChangeState(PLAY_STATE_LOADING);

  return ScheduleStateMachineThread();
}

nsresult nsBuiltinDecoder::RequestFrameBufferLength(PRUint32 aLength)
{
  nsresult res = nsMediaDecoder::RequestFrameBufferLength(aLength);
  NS_ENSURE_SUCCESS(res,res);

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  if (mDecoderStateMachine) {
      mDecoderStateMachine->SetFrameBufferLength(aLength);
  }
  return res;
}

nsresult nsBuiltinDecoder::ScheduleStateMachineThread()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ASSERTION(mDecoderStateMachine,
               "Must have state machine to start state machine thread");

  if (mShuttingDown)
    return NS_OK;
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  nsBuiltinDecoderStateMachine* m =
    static_cast<nsBuiltinDecoderStateMachine*>(mDecoderStateMachine.get());
  return m->ScheduleStateMachine();
}

nsresult nsBuiltinDecoder::Play()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  nsresult res = ScheduleStateMachineThread();
  NS_ENSURE_SUCCESS(res,res);
  if (mPlayState == PLAY_STATE_SEEKING) {
    mNextState = PLAY_STATE_PLAYING;
    return NS_OK;
  }
  if (mPlayState == PLAY_STATE_ENDED)
    return Seek(0);

  ChangeState(PLAY_STATE_PLAYING);
  return NS_OK;
}








static PRBool IsInRanges(nsTimeRanges& aRanges, double aValue, PRInt32& aIntervalIndex) {
  PRUint32 length;
  aRanges.GetLength(&length);
  for (PRUint32 i = 0; i < length; i++) {
    double start, end;
    aRanges.Start(i, &start);
    if (start > aValue) {
      aIntervalIndex = i - 1;
      return PR_FALSE;
    }
    aRanges.End(i, &end);
    if (aValue <= end) {
      aIntervalIndex = i;
      return PR_TRUE;
    }
  }
  aIntervalIndex = length - 1;
  return PR_FALSE;
}

nsresult nsBuiltinDecoder::Seek(double aTime)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  NS_ABORT_IF_FALSE(aTime >= 0.0, "Cannot seek to a negative value.");

  nsTimeRanges seekable;
  nsresult res;
  PRUint32 length = 0;
  res = GetSeekable(&seekable);
  NS_ENSURE_SUCCESS(res, NS_OK);

  seekable.GetLength(&length);
  if (!length) {
    return NS_OK;
  }

  
  
  
  
  
  PRInt32 range = 0;
  if (!IsInRanges(seekable, aTime, range)) {
    if (range != -1) {
      double leftBound, rightBound;
      res = seekable.End(range, &leftBound);
      NS_ENSURE_SUCCESS(res, NS_OK);
      double distanceLeft = NS_ABS(leftBound - aTime);

      double distanceRight = -1;
      if (range + 1 < length) {
        res = seekable.Start(range+1, &rightBound);
        NS_ENSURE_SUCCESS(res, NS_OK);
        distanceRight = NS_ABS(rightBound - aTime);
      }

      if (distanceLeft == distanceRight) {
        distanceLeft = NS_ABS(leftBound - mCurrentTime);
        distanceRight = NS_ABS(rightBound - mCurrentTime);
      } 
      aTime = (distanceLeft < distanceRight) ? leftBound : rightBound;
    } else {
      
      
      seekable.Start(0, &aTime);
    }
  }

  mRequestedSeekTime = aTime;
  mCurrentTime = aTime;

  
  
  
  if (mPlayState != PLAY_STATE_SEEKING) {
    PRBool paused = PR_FALSE;
    if (mElement) {
      mElement->GetPaused(&paused);
    }
    mNextState = paused ? PLAY_STATE_PAUSED : PLAY_STATE_PLAYING;
    PinForSeek();
    ChangeState(PLAY_STATE_SEEKING);
  }

  return ScheduleStateMachineThread();
}

nsresult nsBuiltinDecoder::PlaybackRateChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return NS_ERROR_NOT_IMPLEMENTED;
}

double nsBuiltinDecoder::GetCurrentTime()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return mCurrentTime;
}

nsMediaStream* nsBuiltinDecoder::GetCurrentStream()
{
  return mStream;
}

already_AddRefed<nsIPrincipal> nsBuiltinDecoder::GetCurrentPrincipal()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return mStream ? mStream->GetCurrentPrincipal() : nsnull;
}

void nsBuiltinDecoder::AudioAvailable(float* aFrameBuffer,
                                      PRUint32 aFrameBufferLength,
                                      float aTime)
{
  
  
  
  nsAutoArrayPtr<float> frameBuffer(aFrameBuffer);
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown) {
    return;
  }

  if (!mElement || !mElement->MayHaveAudioAvailableEventListener()) {
    return;
  }

  mElement->NotifyAudioAvailable(frameBuffer.forget(), aFrameBufferLength, aTime);
}

void nsBuiltinDecoder::MetadataLoaded(PRUint32 aChannels,
                                      PRUint32 aRate)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown) {
    return;
  }

  
  
  PRBool notifyElement = PR_TRUE;
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mDuration = mDecoderStateMachine ? mDecoderStateMachine->GetDuration() : -1;
    
    UpdatePlaybackRate();

    notifyElement = mNextState != PLAY_STATE_SEEKING;
  }

  if (mDuration == -1) {
    SetInfinite(PR_TRUE);
  }

  if (mElement && notifyElement) {
    
    
    Invalidate();
    mElement->MetadataLoaded(aChannels, aRate);
  }

  if (!mResourceLoaded) {
    StartProgress();
  } else if (mElement) {
    
    
    mElement->DispatchAsyncEvent(NS_LITERAL_STRING("progress"));
  }

  
  
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  PRBool resourceIsLoaded = !mResourceLoaded && mStream &&
    mStream->IsDataCachedToEndOfStream(mDecoderPosition);
  if (mElement && notifyElement) {
    mElement->FirstFrameLoaded(resourceIsLoaded);
  }

  
  
  
  
  if (mPlayState == PLAY_STATE_LOADING) {
    if (mRequestedSeekTime >= 0.0) {
      ChangeState(PLAY_STATE_SEEKING);
    }
    else {
      ChangeState(mNextState);
    }
  }

  if (resourceIsLoaded) {
    ResourceLoaded();
  }
}

void nsBuiltinDecoder::ResourceLoaded()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  
  
  
  if (mShuttingDown)
    return;

  {
    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    if (mIgnoreProgressData || mResourceLoaded || mPlayState == PLAY_STATE_LOADING)
      return;

    Progress(PR_FALSE);

    mResourceLoaded = PR_TRUE;
    StopProgress();
  }

  
  if (mElement) {
    mElement->ResourceLoaded();
  }
}

void nsBuiltinDecoder::NetworkError()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown)
    return;

  if (mElement)
    mElement->NetworkError();

  Shutdown();
}

void nsBuiltinDecoder::DecodeError()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown)
    return;

  if (mElement)
    mElement->DecodeError();

  Shutdown();
}

PRBool nsBuiltinDecoder::IsSeeking() const
{
  return mPlayState == PLAY_STATE_SEEKING || mNextState == PLAY_STATE_SEEKING;
}

PRBool nsBuiltinDecoder::IsEnded() const
{
  return mPlayState == PLAY_STATE_ENDED || mPlayState == PLAY_STATE_SHUTDOWN;
}

void nsBuiltinDecoder::PlaybackEnded()
{
  if (mShuttingDown || mPlayState == nsBuiltinDecoder::PLAY_STATE_SEEKING)
    return;

  PlaybackPositionChanged();
  ChangeState(PLAY_STATE_ENDED);

  if (mElement)  {
    UpdateReadyStateForData();
    mElement->PlaybackEnded();
  }

  
  
  if (IsInfinite()) {
    SetInfinite(PR_FALSE);
  }
}

NS_IMETHODIMP nsBuiltinDecoder::Observe(nsISupports *aSubjet,
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
nsBuiltinDecoder::GetStatistics()
{
  NS_ASSERTION(NS_IsMainThread() || OnStateMachineThread(),
               "Should be on main or state machine thread.");
  Statistics result;

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  if (mStream) {
    result.mDownloadRate = 
      mStream->GetDownloadRate(&result.mDownloadRateReliable);
    result.mDownloadPosition =
      mStream->GetCachedDataEnd(mDecoderPosition);
    result.mTotalBytes = mStream->GetLength();
    result.mPlaybackRate = ComputePlaybackRate(&result.mPlaybackRateReliable);
    result.mDecoderPosition = mDecoderPosition;
    result.mPlaybackPosition = mPlaybackPosition;
  }
  else {
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

double nsBuiltinDecoder::ComputePlaybackRate(PRPackedBool* aReliable)
{
  GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(NS_IsMainThread() || OnStateMachineThread(),
               "Should be on main or state machine thread.");

  PRInt64 length = mStream ? mStream->GetLength() : -1;
  if (mDuration >= 0 && length >= 0) {
    *aReliable = PR_TRUE;
    return length * static_cast<double>(USECS_PER_S) / mDuration;
  }
  return mPlaybackStatistics.GetRateAtLastStop(aReliable);
}

void nsBuiltinDecoder::UpdatePlaybackRate()
{
  NS_ASSERTION(NS_IsMainThread() || OnStateMachineThread(),
               "Should be on main or state machine thread.");
  GetReentrantMonitor().AssertCurrentThreadIn();
  if (!mStream)
    return;
  PRPackedBool reliable;
  PRUint32 rate = PRUint32(ComputePlaybackRate(&reliable));
  if (reliable) {
    
    rate = NS_MAX(rate, 1u);
  }
  else {
    
    
    rate = NS_MAX(rate, 10000u);
  }
  mStream->SetPlaybackRate(rate);
}

void nsBuiltinDecoder::NotifySuspendedStatusChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (!mStream)
    return;
  if (mStream->IsSuspendedByCache() && mElement) {
    
    
    mElement->NotifyAutoplayDataReady();
  }
}

void nsBuiltinDecoder::NotifyBytesDownloaded()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  UpdateReadyStateForData();
  Progress(PR_FALSE);
}

void nsBuiltinDecoder::NotifyDownloadEnded(nsresult aStatus)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (aStatus == NS_BINDING_ABORTED) {
    
    if (mElement) {
      mElement->LoadAborted();
    }
    return;
  }

  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    UpdatePlaybackRate();
  }

  if (NS_SUCCEEDED(aStatus)) {
    ResourceLoaded();
  }
  else if (aStatus != NS_BASE_STREAM_CLOSED) {
    NetworkError();
  }
  UpdateReadyStateForData();
}

void nsBuiltinDecoder::NotifyBytesConsumed(PRInt64 aBytes)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  NS_ASSERTION(OnStateMachineThread() || mDecoderStateMachine->OnDecodeThread(),
               "Should be on play state machine or decode thread.");
  if (!mIgnoreProgressData) {
    mDecoderPosition += aBytes;
    mPlaybackStatistics.AddBytes(aBytes);
  }
}

void nsBuiltinDecoder::NextFrameUnavailableBuffering()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
  if (!mElement || mShuttingDown || !mDecoderStateMachine)
    return;

  mElement->UpdateReadyStateForData(nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE_BUFFERING);
}

void nsBuiltinDecoder::NextFrameAvailable()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
  if (!mElement || mShuttingDown || !mDecoderStateMachine)
    return;

  mElement->UpdateReadyStateForData(nsHTMLMediaElement::NEXT_FRAME_AVAILABLE);
}

void nsBuiltinDecoder::NextFrameUnavailable()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
  if (!mElement || mShuttingDown || !mDecoderStateMachine)
    return;
  mElement->UpdateReadyStateForData(nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE);
}

void nsBuiltinDecoder::UpdateReadyStateForData()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
  if (!mElement || mShuttingDown || !mDecoderStateMachine)
    return;
  nsHTMLMediaElement::NextFrameStatus frameStatus =
    mDecoderStateMachine->GetNextFrameStatus();
  mElement->UpdateReadyStateForData(frameStatus);
}

void nsBuiltinDecoder::SeekingStopped()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (mShuttingDown)
    return;

  PRBool seekWasAborted = PR_FALSE;
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    
    if (mRequestedSeekTime >= 0.0) {
      ChangeState(PLAY_STATE_SEEKING);
      seekWasAborted = PR_TRUE;
    } else {
      UnpinForSeek();
      ChangeState(mNextState);
    }
  }

  if (mElement) {
    UpdateReadyStateForData();
    if (!seekWasAborted) {
      mElement->SeekCompleted();
    }
  }
}



void nsBuiltinDecoder::SeekingStoppedAtEnd()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (mShuttingDown)
    return;

  PRBool fireEnded = PR_FALSE;
  PRBool seekWasAborted = PR_FALSE;
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    
    if (mRequestedSeekTime >= 0.0) {
      ChangeState(PLAY_STATE_SEEKING);
      seekWasAborted = PR_TRUE;
    } else {
      UnpinForSeek();
      fireEnded = PR_TRUE;
      ChangeState(PLAY_STATE_ENDED);
    }
  }

  if (mElement) {
    UpdateReadyStateForData();
    if (!seekWasAborted) {
      mElement->SeekCompleted();
      if (fireEnded) {
        mElement->PlaybackEnded();
      }
    }
  }
}

void nsBuiltinDecoder::SeekingStarted()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown)
    return;

  if (mElement) {
    UpdateReadyStateForData();
    mElement->SeekStarted();
  }
}

void nsBuiltinDecoder::ChangeState(PlayState aState)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");   
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (mNextState == aState) {
    mNextState = PLAY_STATE_PAUSED;
  }

  if (mPlayState == PLAY_STATE_SHUTDOWN) {
    mReentrantMonitor.NotifyAll();
    return;
  }

  mPlayState = aState;
  switch (aState) {
  case PLAY_STATE_PAUSED:
    
    break;
  case PLAY_STATE_PLAYING:
    mDecoderStateMachine->Play();
    break;
  case PLAY_STATE_SEEKING:
    mDecoderStateMachine->Seek(mRequestedSeekTime);
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
  mReentrantMonitor.NotifyAll();
}

void nsBuiltinDecoder::PlaybackPositionChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown)
    return;

  double lastTime = mCurrentTime;

  
  
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    if (mDecoderStateMachine) {
      mCurrentTime = mDecoderStateMachine->GetCurrentTime();
      mDecoderStateMachine->ClearPositionChangeFlag();
    }
  }

  
  
  
  
  Invalidate();

  if (mElement && lastTime != mCurrentTime) {
    FireTimeUpdate();
  }
}

void nsBuiltinDecoder::DurationChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  PRInt64 oldDuration = mDuration;
  mDuration = mDecoderStateMachine ? mDecoderStateMachine->GetDuration() : -1;
  
  UpdatePlaybackRate();

  if (mElement && oldDuration != mDuration && !IsInfinite()) {
    LOG(PR_LOG_DEBUG, ("%p duration changed to %lld", this, mDuration));
    mElement->DispatchEvent(NS_LITERAL_STRING("durationchange"));
  }
}

void nsBuiltinDecoder::SetDuration(double aDuration)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDuration = static_cast<PRInt64>(NS_round(aDuration * static_cast<double>(USECS_PER_S)));

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  if (mDecoderStateMachine) {
    mDecoderStateMachine->SetDuration(mDuration);
  }

  
  UpdatePlaybackRate();
}

void nsBuiltinDecoder::SetSeekable(PRBool aSeekable)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mSeekable = aSeekable;
  if (mDecoderStateMachine) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mDecoderStateMachine->SetSeekable(aSeekable);
  }
}

PRBool nsBuiltinDecoder::IsSeekable()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return mSeekable;
}

nsresult nsBuiltinDecoder::GetSeekable(nsTimeRanges* aSeekable)
{
  
  double initialTime = 0.0;

  if (IsSeekable()) {
    double end = IsInfinite() ? std::numeric_limits<double>::infinity()
                              : initialTime + GetDuration();
    aSeekable->Add(initialTime, end);
    return NS_OK;
  }

  return GetBuffered(aSeekable);
}

void nsBuiltinDecoder::SetEndTime(double aTime)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mDecoderStateMachine) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mDecoderStateMachine->SetFragmentEndTime(static_cast<PRInt64>(aTime * USECS_PER_S));
  }
}

void nsBuiltinDecoder::Suspend()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mStream) {
    mStream->Suspend(PR_TRUE);
  }
}

void nsBuiltinDecoder::Resume(PRBool aForceBuffering)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mStream) {
    mStream->Resume();
  }
  if (aForceBuffering) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mDecoderStateMachine->StartBuffering();
  }
}

void nsBuiltinDecoder::StopProgressUpdates()
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  GetReentrantMonitor().AssertCurrentThreadIn();
  mIgnoreProgressData = PR_TRUE;
  if (mStream) {
    mStream->SetReadMode(nsMediaCacheStream::MODE_METADATA);
  }
}

void nsBuiltinDecoder::StartProgressUpdates()
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  GetReentrantMonitor().AssertCurrentThreadIn();
  mIgnoreProgressData = PR_FALSE;
  if (mStream) {
    mStream->SetReadMode(nsMediaCacheStream::MODE_PLAYBACK);
    mDecoderPosition = mPlaybackPosition = mStream->Tell();
  }
}

void nsBuiltinDecoder::MoveLoadsToBackground()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mStream) {
    mStream->MoveLoadsToBackground();
  }
}

void nsBuiltinDecoder::UpdatePlaybackOffset(PRInt64 aOffset)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  mPlaybackPosition = NS_MAX(aOffset, mPlaybackPosition);
}

PRBool nsBuiltinDecoder::OnStateMachineThread() const {
  return IsCurrentThread(nsBuiltinDecoderStateMachine::GetStateMachineThread());
}
