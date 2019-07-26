





#include "nsBuiltinDecoder.h"
#include <limits>
#include "nsNetUtil.h"
#include "nsAudioStream.h"
#include "nsHTMLVideoElement.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsTArray.h"
#include "VideoUtils.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "nsTimeRanges.h"
#include "nsContentUtils.h"
#include "ImageContainer.h"

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
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
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

void nsBuiltinDecoder::SetAudioCaptured(bool aCaptured)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mInitialAudioCaptured = aCaptured;
  if (mDecoderStateMachine) {
    mDecoderStateMachine->SetAudioCaptured(aCaptured);
  }
}

void nsBuiltinDecoder::ConnectDecodedStreamToOutputStream(OutputStreamData* aStream)
{
  NS_ASSERTION(!aStream->mPort, "Already connected?");

  
  
  aStream->mPort = aStream->mStream->AllocateInputPort(mDecodedStream->mStream,
      MediaInputPort::FLAG_BLOCK_INPUT | MediaInputPort::FLAG_BLOCK_OUTPUT);
  
  
  aStream->mStream->ChangeExplicitBlockerCount(-1);
}

nsBuiltinDecoder::DecodedStreamData::DecodedStreamData(nsBuiltinDecoder* aDecoder,
                                                       int64_t aInitialTime,
                                                       SourceMediaStream* aStream)
  : mLastAudioPacketTime(-1),
    mLastAudioPacketEndTime(-1),
    mAudioFramesWritten(0),
    mInitialTime(aInitialTime),
    mNextVideoTime(aInitialTime),
    mStreamInitialized(false),
    mHaveSentFinish(false),
    mHaveSentFinishAudio(false),
    mHaveSentFinishVideo(false),
    mStream(aStream),
    mMainThreadListener(new DecodedStreamMainThreadListener(aDecoder)),
    mHaveBlockedForPlayState(false)
{
  mStream->AddMainThreadListener(mMainThreadListener);
}

nsBuiltinDecoder::DecodedStreamData::~DecodedStreamData()
{
  mStream->RemoveMainThreadListener(mMainThreadListener);
  mStream->Destroy();
}

void nsBuiltinDecoder::DestroyDecodedStream()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  GetReentrantMonitor().AssertCurrentThreadIn();

  
  
  for (uint32_t i = 0; i < mOutputStreams.Length(); ++i) {
    OutputStreamData& os = mOutputStreams[i];
    
    
    
    if (!os.mStream->IsDestroyed()) {
      os.mStream->ChangeExplicitBlockerCount(1);
    }
    
    
    os.mPort->Destroy();
    os.mPort = nullptr;
  }

  mDecodedStream = nullptr;
}

void nsBuiltinDecoder::RecreateDecodedStream(int64_t aStartTimeUSecs)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  GetReentrantMonitor().AssertCurrentThreadIn();
  LOG(PR_LOG_DEBUG, ("nsBuiltinDecoder::RecreateDecodedStream this=%p aStartTimeUSecs=%lld!",
                     this, (long long)aStartTimeUSecs));

  DestroyDecodedStream();

  mDecodedStream = new DecodedStreamData(this, aStartTimeUSecs,
    MediaStreamGraph::GetInstance()->CreateSourceStream(nullptr));

  
  
  
  for (uint32_t i = 0; i < mOutputStreams.Length(); ++i) {
    ConnectDecodedStreamToOutputStream(&mOutputStreams[i]);
  }

  mDecodedStream->mHaveBlockedForPlayState = mPlayState != PLAY_STATE_PLAYING;
  if (mDecodedStream->mHaveBlockedForPlayState) {
    mDecodedStream->mStream->ChangeExplicitBlockerCount(1);
  }
}

void nsBuiltinDecoder::NotifyDecodedStreamMainThreadStateChanged()
{
  if (mTriggerPlaybackEndedWhenSourceStreamFinishes && mDecodedStream &&
      mDecodedStream->mStream->IsFinished()) {
    mTriggerPlaybackEndedWhenSourceStreamFinishes = false;
    if (GetState() == PLAY_STATE_PLAYING) {
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(this, &nsBuiltinDecoder::PlaybackEnded);
      NS_DispatchToCurrentThread(event);
    }
  }
}

void nsBuiltinDecoder::AddOutputStream(ProcessedMediaStream* aStream,
                                       bool aFinishWhenEnded)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  LOG(PR_LOG_DEBUG, ("nsBuiltinDecoder::AddOutputStream this=%p aStream=%p!",
                     this, aStream));

  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    if (!mDecodedStream) {
      RecreateDecodedStream(mDecoderStateMachine ?
          int64_t(mDecoderStateMachine->GetCurrentTime()*USECS_PER_S) : 0);
    }
    OutputStreamData* os = mOutputStreams.AppendElement();
    os->Init(aStream, aFinishWhenEnded);
    ConnectDecodedStreamToOutputStream(os);
    if (aFinishWhenEnded) {
      
      aStream->SetAutofinish(true);
    }
  }

  
  
  
  if (mDecoderStateMachine) {
    
    
    ScheduleStateMachineThread();
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

void nsBuiltinDecoder::SetInfinite(bool aInfinite)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mInfiniteStream = aInfinite;
}

bool nsBuiltinDecoder::IsInfinite()
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
  mSeekable(true),
  mReentrantMonitor("media.decoder"),
  mPlayState(PLAY_STATE_PAUSED),
  mNextState(PLAY_STATE_PAUSED),
  mResourceLoaded(false),
  mIgnoreProgressData(false),
  mInfiniteStream(false),
  mTriggerPlaybackEndedWhenSourceStreamFinishes(false)
{
  MOZ_COUNT_CTOR(nsBuiltinDecoder);
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
#ifdef PR_LOGGING
  if (!gBuiltinDecoderLog) {
    gBuiltinDecoderLog = PR_NewLogModule("nsBuiltinDecoder");
  }
#endif
}

bool nsBuiltinDecoder::Init(nsHTMLMediaElement* aElement)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (!nsMediaDecoder::Init(aElement))
    return false;

  nsContentUtils::RegisterShutdownObserver(this);
  return true;
}

void nsBuiltinDecoder::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (mShuttingDown)
    return;

  mShuttingDown = true;

  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    DestroyDecodedStream();
  }

  
  
  
  if (mDecoderStateMachine) {
    mDecoderStateMachine->Shutdown();
  }

  
  
  if (mResource) {
    mResource->Close();
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

nsresult nsBuiltinDecoder::OpenResource(MediaResource* aResource,
                                        nsIStreamListener** aStreamListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (aStreamListener) {
    *aStreamListener = nullptr;
  }

  {
    
    
    
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

    nsresult rv = aResource->Open(aStreamListener);
    if (NS_FAILED(rv)) {
      LOG(PR_LOG_DEBUG, ("%p Failed to open stream!", this));
      delete aResource;
      return rv;
    }

    mResource = aResource;
  }
  return NS_OK;
}

nsresult nsBuiltinDecoder::Load(MediaResource* aResource,
                                nsIStreamListener** aStreamListener,
                                nsMediaDecoder* aCloneDonor)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  nsresult rv = OpenResource(aResource, aStreamListener);
  NS_ENSURE_SUCCESS(rv, rv);

  mDecoderStateMachine = CreateStateMachine();
  if (!mDecoderStateMachine) {
    LOG(PR_LOG_DEBUG, ("%p Failed to create state machine!", this));
    return NS_ERROR_FAILURE;
  }

  return InitializeStateMachine(aCloneDonor);
}

nsresult nsBuiltinDecoder::InitializeStateMachine(nsMediaDecoder* aCloneDonor)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  nsBuiltinDecoder* cloneDonor = static_cast<nsBuiltinDecoder*>(aCloneDonor);
  if (NS_FAILED(mDecoderStateMachine->Init(cloneDonor ?
                                           cloneDonor->mDecoderStateMachine : nullptr))) {
    LOG(PR_LOG_DEBUG, ("%p Failed to init state machine!", this));
    return NS_ERROR_FAILURE;
  }
  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    mDecoderStateMachine->SetSeekable(mSeekable);
    mDecoderStateMachine->SetDuration(mDuration);
    mDecoderStateMachine->SetVolume(mInitialVolume);
    mDecoderStateMachine->SetAudioCaptured(mInitialAudioCaptured);

    if (mFrameBufferLength > 0) {
      
      mDecoderStateMachine->SetFrameBufferLength(mFrameBufferLength);
    }
  }

  ChangeState(PLAY_STATE_LOADING);

  return ScheduleStateMachineThread();
}

nsresult nsBuiltinDecoder::RequestFrameBufferLength(uint32_t aLength)
{
  nsresult res = nsMediaDecoder::RequestFrameBufferLength(aLength);
  NS_ENSURE_SUCCESS(res,res);

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
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
  NS_ENSURE_STATE(mDecoderStateMachine);

  if (mShuttingDown)
    return NS_OK;
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  nsBuiltinDecoderStateMachine* m =
    static_cast<nsBuiltinDecoderStateMachine*>(mDecoderStateMachine.get());
  return m->ScheduleStateMachine();
}

nsresult nsBuiltinDecoder::Play()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  NS_ASSERTION(mDecoderStateMachine != nullptr, "Should have state machine.");
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








static bool IsInRanges(nsTimeRanges& aRanges, double aValue, int32_t& aIntervalIndex) {
  uint32_t length;
  aRanges.GetLength(&length);
  for (uint32_t i = 0; i < length; i++) {
    double start, end;
    aRanges.Start(i, &start);
    if (start > aValue) {
      aIntervalIndex = i - 1;
      return false;
    }
    aRanges.End(i, &end);
    if (aValue <= end) {
      aIntervalIndex = i;
      return true;
    }
  }
  aIntervalIndex = length - 1;
  return false;
}

nsresult nsBuiltinDecoder::Seek(double aTime)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  NS_ABORT_IF_FALSE(aTime >= 0.0, "Cannot seek to a negative value.");

  nsTimeRanges seekable;
  nsresult res;
  uint32_t length = 0;
  res = GetSeekable(&seekable);
  NS_ENSURE_SUCCESS(res, NS_OK);

  seekable.GetLength(&length);
  if (!length) {
    return NS_OK;
  }

  
  
  
  
  
  int32_t range = 0;
  if (!IsInRanges(seekable, aTime, range)) {
    if (range != -1) {
      
      
      if (uint32_t(range + 1) < length) {
        double leftBound, rightBound;
        res = seekable.End(range, &leftBound);
        NS_ENSURE_SUCCESS(res, NS_OK);
        res = seekable.Start(range + 1, &rightBound);
        NS_ENSURE_SUCCESS(res, NS_OK);
        double distanceLeft = NS_ABS(leftBound - aTime);
        double distanceRight = NS_ABS(rightBound - aTime);
        if (distanceLeft == distanceRight) {
          distanceLeft = NS_ABS(leftBound - mCurrentTime);
          distanceRight = NS_ABS(rightBound - mCurrentTime);
        } 
        aTime = (distanceLeft < distanceRight) ? leftBound : rightBound;
      } else {
        
        
        res = seekable.End(length - 1, &aTime);
        NS_ENSURE_SUCCESS(res, NS_OK);
      }
    } else {
      
      
      seekable.Start(0, &aTime);
    }
  }

  mRequestedSeekTime = aTime;
  mCurrentTime = aTime;

  
  
  
  if (mPlayState != PLAY_STATE_SEEKING) {
    bool paused = false;
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

already_AddRefed<nsIPrincipal> nsBuiltinDecoder::GetCurrentPrincipal()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return mResource ? mResource->GetCurrentPrincipal() : nullptr;
}

void nsBuiltinDecoder::AudioAvailable(float* aFrameBuffer,
                                      uint32_t aFrameBufferLength,
                                      float aTime)
{
  
  
  
  nsAutoArrayPtr<float> frameBuffer(aFrameBuffer);
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown || !mElement) {
    return;
  }
  mElement->NotifyAudioAvailable(frameBuffer.forget(), aFrameBufferLength, aTime);
}

void nsBuiltinDecoder::MetadataLoaded(uint32_t aChannels,
                                      uint32_t aRate,
                                      bool aHasAudio,
                                      const MetadataTags* aTags)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown) {
    return;
  }

  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    mDuration = mDecoderStateMachine ? mDecoderStateMachine->GetDuration() : -1;
    
    UpdatePlaybackRate();
  }

  if (mDuration == -1) {
    SetInfinite(true);
  }

  if (mElement) {
    
    
    Invalidate();
    mElement->MetadataLoaded(aChannels, aRate, aHasAudio, aTags);
  }

  if (!mResourceLoaded) {
    StartProgress();
  } else if (mElement) {
    
    
    mElement->DispatchAsyncEvent(NS_LITERAL_STRING("progress"));
  }

  
  
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  bool resourceIsLoaded = !mResourceLoaded && mResource &&
    mResource->IsDataCachedToEndOfResource(mDecoderPosition);
  if (mElement) {
    mElement->FirstFrameLoaded(resourceIsLoaded);
  }

  
  mResource->EnsureCacheUpToDate();

  
  
  
  
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

  
  
  NotifySuspendedStatusChanged();
}

void nsBuiltinDecoder::ResourceLoaded()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  
  
  
  if (mShuttingDown)
    return;

  {
    
    
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    if (mIgnoreProgressData || mResourceLoaded || mPlayState == PLAY_STATE_LOADING)
      return;

    Progress(false);

    mResourceLoaded = true;
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

bool nsBuiltinDecoder::IsSeeking() const
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return mPlayState == PLAY_STATE_SEEKING;
}

bool nsBuiltinDecoder::IsEnded() const
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return mPlayState == PLAY_STATE_ENDED || mPlayState == PLAY_STATE_SHUTDOWN;
}

void nsBuiltinDecoder::PlaybackEnded()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (mShuttingDown || mPlayState == nsBuiltinDecoder::PLAY_STATE_SEEKING)
    return;

  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

    if (mDecodedStream && !mDecodedStream->mStream->IsFinished()) {
      
      mTriggerPlaybackEndedWhenSourceStreamFinishes = true;
      return;
    }

    for (int32_t i = mOutputStreams.Length() - 1; i >= 0; --i) {
      OutputStreamData& os = mOutputStreams[i];
      if (os.mFinishWhenEnded) {
        
        
        os.mStream->Finish();
        os.mPort->Destroy();
        os.mPort = nullptr;
        
        
        os.mStream->ChangeExplicitBlockerCount(1);
        mOutputStreams.RemoveElementAt(i);
      }
    }
  }

  PlaybackPositionChanged();
  ChangeState(PLAY_STATE_ENDED);

  if (mElement)  {
    UpdateReadyStateForData();
    mElement->PlaybackEnded();
  }

  
  
  if (IsInfinite()) {
    SetInfinite(false);
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

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (mResource) {
    result.mDownloadRate =
      mResource->GetDownloadRate(&result.mDownloadRateReliable);
    result.mDownloadPosition =
      mResource->GetCachedDataEnd(mDecoderPosition);
    result.mTotalBytes = mResource->GetLength();
    result.mPlaybackRate = ComputePlaybackRate(&result.mPlaybackRateReliable);
    result.mDecoderPosition = mDecoderPosition;
    result.mPlaybackPosition = mPlaybackPosition;
  }
  else {
    result.mDownloadRate = 0;
    result.mDownloadRateReliable = true;
    result.mPlaybackRate = 0;
    result.mPlaybackRateReliable = true;
    result.mDecoderPosition = 0;
    result.mPlaybackPosition = 0;
    result.mDownloadPosition = 0;
    result.mTotalBytes = 0;
  }

  return result;
}

double nsBuiltinDecoder::ComputePlaybackRate(bool* aReliable)
{
  GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(NS_IsMainThread() || OnStateMachineThread(),
               "Should be on main or state machine thread.");

  int64_t length = mResource ? mResource->GetLength() : -1;
  if (mDuration >= 0 && length >= 0) {
    *aReliable = true;
    return length * static_cast<double>(USECS_PER_S) / mDuration;
  }
  return mPlaybackStatistics.GetRateAtLastStop(aReliable);
}

void nsBuiltinDecoder::UpdatePlaybackRate()
{
  NS_ASSERTION(NS_IsMainThread() || OnStateMachineThread(),
               "Should be on main or state machine thread.");
  GetReentrantMonitor().AssertCurrentThreadIn();
  if (!mResource)
    return;
  bool reliable;
  uint32_t rate = uint32_t(ComputePlaybackRate(&reliable));
  if (reliable) {
    
    rate = NS_MAX(rate, 1u);
  }
  else {
    
    
    rate = NS_MAX(rate, 10000u);
  }
  mResource->SetPlaybackRate(rate);
}

void nsBuiltinDecoder::NotifySuspendedStatusChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (!mResource)
    return;
  MediaResource* activeStream;
  bool suspended = mResource->IsSuspendedByCache(&activeStream);

  if (mElement) {
    if (suspended) {
      
      
      mElement->NotifyAutoplayDataReady();
    }
    mElement->NotifySuspendedByCache(suspended);
    UpdateReadyStateForData();
  }
}

void nsBuiltinDecoder::NotifyBytesDownloaded()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  UpdateReadyStateForData();
  Progress(false);
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
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
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

void nsBuiltinDecoder::NotifyPrincipalChanged()
{
  if (mElement) {
    mElement->NotifyDecoderPrincipalChanged();
  }
}

void nsBuiltinDecoder::NotifyBytesConsumed(int64_t aBytes)
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
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

  mElement->UpdateReadyStateForData(nsMediaDecoder::NEXT_FRAME_UNAVAILABLE_BUFFERING);
}

void nsBuiltinDecoder::NextFrameAvailable()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
  if (!mElement || mShuttingDown || !mDecoderStateMachine)
    return;

  mElement->UpdateReadyStateForData(nsMediaDecoder::NEXT_FRAME_AVAILABLE);
}

void nsBuiltinDecoder::NextFrameUnavailable()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
  if (!mElement || mShuttingDown || !mDecoderStateMachine)
    return;
  mElement->UpdateReadyStateForData(nsMediaDecoder::NEXT_FRAME_UNAVAILABLE);
}

void nsBuiltinDecoder::UpdateReadyStateForData()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
  if (!mElement || mShuttingDown || !mDecoderStateMachine)
    return;
  NextFrameStatus frameStatus =
    mDecoderStateMachine->GetNextFrameStatus();
  mElement->UpdateReadyStateForData(frameStatus);
}

void nsBuiltinDecoder::SeekingStopped()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (mShuttingDown)
    return;

  bool seekWasAborted = false;
  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

    
    
    if (mRequestedSeekTime >= 0.0) {
      ChangeState(PLAY_STATE_SEEKING);
      seekWasAborted = true;
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

  bool fireEnded = false;
  bool seekWasAborted = false;
  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

    
    
    if (mRequestedSeekTime >= 0.0) {
      ChangeState(PLAY_STATE_SEEKING);
      seekWasAborted = true;
    } else {
      UnpinForSeek();
      fireEnded = true;
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
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  if (mNextState == aState) {
    mNextState = PLAY_STATE_PAUSED;
  }

  if (mPlayState == PLAY_STATE_SHUTDOWN) {
    GetReentrantMonitor().NotifyAll();
    return;
  }

  if (mDecodedStream) {
    bool blockForPlayState = aState != PLAY_STATE_PLAYING;
    if (mDecodedStream->mHaveBlockedForPlayState != blockForPlayState) {
      mDecodedStream->mStream->ChangeExplicitBlockerCount(blockForPlayState ? 1 : -1);
      mDecodedStream->mHaveBlockedForPlayState = blockForPlayState;
    }
  }
  mPlayState = aState;
  if (mDecoderStateMachine) {
    switch (aState) {
    case PLAY_STATE_PLAYING:
      mDecoderStateMachine->Play();
      break;
    case PLAY_STATE_SEEKING:
      mDecoderStateMachine->Seek(mRequestedSeekTime);
      mRequestedSeekTime = -1.0;
      break;
    default:
      
      break;
    }
  }
  GetReentrantMonitor().NotifyAll();
}

void nsBuiltinDecoder::PlaybackPositionChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mShuttingDown)
    return;

  double lastTime = mCurrentTime;

  
  
  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    if (mDecoderStateMachine) {
      if (!IsSeeking()) {
        
        
        
        
        
        
        
        mCurrentTime = mDecoderStateMachine->GetCurrentTime();
      }
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
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  int64_t oldDuration = mDuration;
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
  mDuration = static_cast<int64_t>(NS_round(aDuration * static_cast<double>(USECS_PER_S)));

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (mDecoderStateMachine) {
    mDecoderStateMachine->SetDuration(mDuration);
  }

  
  UpdatePlaybackRate();
}

void nsBuiltinDecoder::SetSeekable(bool aSeekable)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mSeekable = aSeekable;
  if (mDecoderStateMachine) {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    mDecoderStateMachine->SetSeekable(aSeekable);
  }
}

bool nsBuiltinDecoder::IsSeekable()
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

  if (mDecoderStateMachine && mDecoderStateMachine->IsSeekableInBufferedRanges()) {
    return GetBuffered(aSeekable);
  } else {
    
    
    return NS_OK;
  }
}

void nsBuiltinDecoder::SetEndTime(double aTime)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mDecoderStateMachine) {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    mDecoderStateMachine->SetFragmentEndTime(static_cast<int64_t>(aTime * USECS_PER_S));
  }
}

void nsBuiltinDecoder::Suspend()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mResource) {
    mResource->Suspend(true);
  }
}

void nsBuiltinDecoder::Resume(bool aForceBuffering)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mResource) {
    mResource->Resume();
  }
  if (aForceBuffering) {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    if (mDecoderStateMachine) {
      mDecoderStateMachine->StartBuffering();
    }
  }
}

void nsBuiltinDecoder::StopProgressUpdates()
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  GetReentrantMonitor().AssertCurrentThreadIn();
  mIgnoreProgressData = true;
  if (mResource) {
    mResource->SetReadMode(nsMediaCacheStream::MODE_METADATA);
  }
}

void nsBuiltinDecoder::StartProgressUpdates()
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  GetReentrantMonitor().AssertCurrentThreadIn();
  mIgnoreProgressData = false;
  if (mResource) {
    mResource->SetReadMode(nsMediaCacheStream::MODE_PLAYBACK);
    mDecoderPosition = mPlaybackPosition = mResource->Tell();
  }
}

void nsBuiltinDecoder::MoveLoadsToBackground()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mResource) {
    mResource->MoveLoadsToBackground();
  }
}

void nsBuiltinDecoder::UpdatePlaybackOffset(int64_t aOffset)
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  mPlaybackPosition = NS_MAX(aOffset, mPlaybackPosition);
}

bool nsBuiltinDecoder::OnStateMachineThread() const
{
  return IsCurrentThread(nsBuiltinDecoderStateMachine::GetStateMachineThread());
}

void nsBuiltinDecoder::NotifyAudioAvailableListener()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mDecoderStateMachine) {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    mDecoderStateMachine->NotifyAudioAvailableListener();
  }
}

bool nsBuiltinDecoder::OnDecodeThread() const {
  return mDecoderStateMachine->OnDecodeThread();
}

ReentrantMonitor& nsBuiltinDecoder::GetReentrantMonitor() {
  return mReentrantMonitor.GetReentrantMonitor();
}



nsresult nsBuiltinDecoder::GetBuffered(nsTimeRanges* aBuffered) {
  if (mDecoderStateMachine) {
    return mDecoderStateMachine->GetBuffered(aBuffered);
  }
  return NS_ERROR_FAILURE;
}

int64_t nsBuiltinDecoder::VideoQueueMemoryInUse() {
  if (mDecoderStateMachine) {
    return mDecoderStateMachine->VideoQueueMemoryInUse();
  }
  return 0;
}

int64_t nsBuiltinDecoder::AudioQueueMemoryInUse() {
  if (mDecoderStateMachine) {
    return mDecoderStateMachine->AudioQueueMemoryInUse();
  }
  return 0;
}

void nsBuiltinDecoder::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) {
  if (mDecoderStateMachine) {
    mDecoderStateMachine->NotifyDataArrived(aBuffer, aLength, aOffset);
  }
}

void nsBuiltinDecoder::UpdatePlaybackPosition(int64_t aTime)
{
  mDecoderStateMachine->UpdatePlaybackPosition(aTime);
}


nsBuiltinDecoderStateMachine* nsBuiltinDecoder::GetStateMachine() {
  return mDecoderStateMachine;
}


void nsBuiltinDecoder::ReleaseStateMachine() {
  mDecoderStateMachine = nullptr;
}

