





#include "MediaDecoder.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/MathAlgorithms.h"
#include <limits>
#include "nsIObserver.h"
#include "nsTArray.h"
#include "VideoUtils.h"
#include "MediaDecoderStateMachine.h"
#include "mozilla/dom/TimeRanges.h"
#include "ImageContainer.h"
#include "MediaResource.h"
#include "nsError.h"
#include "mozilla/Preferences.h"
#include "mozilla/StaticPtr.h"
#include "nsIMemoryReporter.h"
#include "nsComponentManagerUtils.h"
#include <algorithm>
#include "MediaShutdownManager.h"
#include "AudioChannelService.h"
#include "mozilla/dom/AudioTrack.h"
#include "mozilla/dom/AudioTrackList.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/VideoTrack.h"
#include "mozilla/dom/VideoTrackList.h"

#ifdef MOZ_WMF
#include "WMFDecoder.h"
#endif

using namespace mozilla::layers;
using namespace mozilla::dom;


static const int DEFAULT_HEURISTIC_DORMANT_TIMEOUT_MSECS = 60000;

namespace mozilla {







static const int64_t CAN_PLAY_THROUGH_MARGIN = 1;


#undef DECODER_LOG

#ifdef PR_LOGGING
PRLogModuleInfo* gMediaDecoderLog;
#define DECODER_LOG(x, ...) \
  PR_LOG(gMediaDecoderLog, PR_LOG_DEBUG, ("Decoder=%p " x, this, ##__VA_ARGS__))
#else
#define DECODER_LOG(x, ...)
#endif

static const char* const gPlayStateStr[] = {
  "START",
  "LOADING",
  "PAUSED",
  "PLAYING",
  "SEEKING",
  "ENDED",
  "SHUTDOWN"
};

class MediaMemoryTracker : public nsIMemoryReporter
{
  virtual ~MediaMemoryTracker();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTER

  MOZ_DEFINE_MALLOC_SIZE_OF(MallocSizeOf);

  MediaMemoryTracker();
  void InitMemoryReporter();

  static StaticRefPtr<MediaMemoryTracker> sUniqueInstance;

  static MediaMemoryTracker* UniqueInstance() {
    if (!sUniqueInstance) {
      sUniqueInstance = new MediaMemoryTracker();
      sUniqueInstance->InitMemoryReporter();
    }
    return sUniqueInstance;
  }

  typedef nsTArray<MediaDecoder*> DecodersArray;
  static DecodersArray& Decoders() {
    return UniqueInstance()->mDecoders;
  }

  DecodersArray mDecoders;

public:
  static void AddMediaDecoder(MediaDecoder* aDecoder)
  {
    Decoders().AppendElement(aDecoder);
  }

  static void RemoveMediaDecoder(MediaDecoder* aDecoder)
  {
    DecodersArray& decoders = Decoders();
    decoders.RemoveElement(aDecoder);
    if (decoders.IsEmpty()) {
      sUniqueInstance = nullptr;
    }
  }
};

StaticRefPtr<MediaMemoryTracker> MediaMemoryTracker::sUniqueInstance;

NS_IMPL_ISUPPORTS(MediaMemoryTracker, nsIMemoryReporter)

NS_IMPL_ISUPPORTS(MediaDecoder, nsIObserver)

void MediaDecoder::NotifyOwnerActivityChanged()
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  if (!mOwner) {
    NS_WARNING("MediaDecoder without a decoder owner, can't update dormant");
    return;
  }

  UpdateDormantState(false , false );
  
  StartDormantTimer();
}

void MediaDecoder::UpdateDormantState(bool aDormantTimeout, bool aActivity)
{
  MOZ_ASSERT(NS_IsMainThread());
  GetReentrantMonitor().AssertCurrentThreadIn();

  if (!mDecoderStateMachine ||
      mPlayState == PLAY_STATE_SHUTDOWN ||
      !mOwner->GetVideoFrameContainer() ||
      (mOwner->GetMediaElement() && mOwner->GetMediaElement()->IsBeingDestroyed()) ||
      !mDecoderStateMachine->IsDormantNeeded())
  {
    return;
  }

  DECODER_LOG("UpdateDormantState aTimeout=%d aActivity=%d mIsDormant=%d "
              "ownerActive=%d ownerHidden=%d mIsHeuristicDormant=%d mPlayState=%s",
              aDormantTimeout, aActivity, mIsDormant, mOwner->IsActive(),
              mOwner->IsHidden(), mIsHeuristicDormant, PlayStateStr());

  bool prevDormant = mIsDormant;
  mIsDormant = false;
  if (!mOwner->IsActive()) {
    mIsDormant = true;
  }
#ifdef MOZ_WIDGET_GONK
  if (mOwner->IsHidden()) {
    mIsDormant = true;
  }
#endif
  
  bool prevHeuristicDormant = mIsHeuristicDormant;
  mIsHeuristicDormant = false;
  if (mIsHeuristicDormantSupported && mOwner->IsHidden()) {
    if (aDormantTimeout && !aActivity &&
        (mPlayState == PLAY_STATE_PAUSED || IsEnded())) {
      
      mIsHeuristicDormant = true;
    } else if(prevHeuristicDormant && !aActivity) {
      
      mIsHeuristicDormant = true;
    }

    if (mIsHeuristicDormant) {
      mIsDormant = true;
    }
  }

  if (prevDormant == mIsDormant) {
    
    return;
  }

  if (mIsDormant) {
    DECODER_LOG("UpdateDormantState() entering DORMANT state");
    
    RefPtr<nsRunnable> event =
      NS_NewRunnableMethodWithArg<bool>(
        mDecoderStateMachine,
        &MediaDecoderStateMachine::SetDormant,
        true);
    mDecoderStateMachine->TaskQueue()->Dispatch(event);

    if (IsEnded()) {
      mWasEndedWhenEnteredDormant = true;
    }
    mNextState = mPlayState;
    ChangeState(PLAY_STATE_LOADING);
  } else {
    DECODER_LOG("UpdateDormantState() leaving DORMANT state");
    
    
    RefPtr<nsRunnable> event =
      NS_NewRunnableMethodWithArg<bool>(
        mDecoderStateMachine,
        &MediaDecoderStateMachine::SetDormant,
        false);
    mDecoderStateMachine->TaskQueue()->Dispatch(event);
  }
}

void MediaDecoder::DormantTimerExpired(nsITimer* aTimer, void* aClosure)
{
  MOZ_ASSERT(aClosure);
  MediaDecoder* decoder = static_cast<MediaDecoder*>(aClosure);
  ReentrantMonitorAutoEnter mon(decoder->GetReentrantMonitor());
  decoder->UpdateDormantState(true ,
                              false );
}

void MediaDecoder::StartDormantTimer()
{
  if (!mIsHeuristicDormantSupported) {
    return;
  }

  if (mIsHeuristicDormant ||
      mShuttingDown ||
      !mOwner ||
      !mOwner->IsHidden() ||
      (mPlayState != PLAY_STATE_PAUSED &&
       !IsEnded()))
  {
    return;
  }

  if (!mDormantTimer) {
    mDormantTimer = do_CreateInstance("@mozilla.org/timer;1");
  }
  mDormantTimer->InitWithFuncCallback(&MediaDecoder::DormantTimerExpired,
                                      this,
                                      mHeuristicDormantTimeout,
                                      nsITimer::TYPE_ONE_SHOT);
}

void MediaDecoder::CancelDormantTimer()
{
  if (mDormantTimer) {
    mDormantTimer->Cancel();
  }
}

void MediaDecoder::Pause()
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (mPlayState == PLAY_STATE_LOADING ||
      mPlayState == PLAY_STATE_SEEKING ||
      IsEnded()) {
    mNextState = PLAY_STATE_PAUSED;
    return;
  }

  ChangeState(PLAY_STATE_PAUSED);
}

void MediaDecoder::SetVolume(double aVolume)
{
  MOZ_ASSERT(NS_IsMainThread());
  mInitialVolume = aVolume;
  if (mDecoderStateMachine) {
    mDecoderStateMachine->SetVolume(aVolume);
  }
}

void MediaDecoder::ConnectDecodedStreamToOutputStream(OutputStreamData* aStream)
{
  NS_ASSERTION(!aStream->mPort, "Already connected?");

  
  
  aStream->mPort = aStream->mStream->AllocateInputPort(mDecodedStream->mStream,
      MediaInputPort::FLAG_BLOCK_INPUT | MediaInputPort::FLAG_BLOCK_OUTPUT);
  
  
  aStream->mStream->ChangeExplicitBlockerCount(-1);
}

MediaDecoder::DecodedStreamData::DecodedStreamData(MediaDecoder* aDecoder,
                                                   int64_t aInitialTime,
                                                   SourceMediaStream* aStream)
  : mAudioFramesWritten(0),
    mInitialTime(aInitialTime),
    mNextVideoTime(-1),
    mNextAudioTime(-1),
    mDecoder(aDecoder),
    mStreamInitialized(false),
    mHaveSentFinish(false),
    mHaveSentFinishAudio(false),
    mHaveSentFinishVideo(false),
    mStream(aStream),
    mHaveBlockedForPlayState(false),
    mHaveBlockedForStateMachineNotPlaying(false)
{
  mListener = new DecodedStreamGraphListener(mStream, this);
  mStream->AddListener(mListener);
}

MediaDecoder::DecodedStreamData::~DecodedStreamData()
{
  mListener->Forget();
  mStream->Destroy();
}

MediaDecoder::DecodedStreamGraphListener::DecodedStreamGraphListener(MediaStream* aStream,
                                                                     DecodedStreamData* aData)
  : mData(aData),
    mMutex("MediaDecoder::DecodedStreamData::mMutex"),
    mStream(aStream),
    mLastOutputTime(aStream->
                    StreamTimeToMicroseconds(aStream->GetCurrentTime())),
    mStreamFinishedOnMainThread(false)
{
}

void
MediaDecoder::DecodedStreamGraphListener::NotifyOutput(MediaStreamGraph* aGraph,
                                                       GraphTime aCurrentTime)
{
  MutexAutoLock lock(mMutex);
  if (mStream) {
    mLastOutputTime = mStream->
      StreamTimeToMicroseconds(mStream->GraphTimeToStreamTime(aCurrentTime));
  }
}

void
MediaDecoder::DecodedStreamGraphListener::DoNotifyFinished()
{
  MutexAutoLock lock(mMutex);
  mStreamFinishedOnMainThread = true;
}

void
MediaDecoder::DecodedStreamGraphListener::NotifyEvent(MediaStreamGraph* aGraph,
  MediaStreamListener::MediaStreamGraphEvent event)
{
  if (event == EVENT_FINISHED) {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &DecodedStreamGraphListener::DoNotifyFinished);
    aGraph->DispatchToMainThreadAfterStreamStateUpdate(event.forget());
  }
}

class MediaDecoder::OutputStreamListener : public MediaStreamListener {
public:
  OutputStreamListener(MediaDecoder* aDecoder, MediaStream* aStream)
    : mDecoder(aDecoder), mStream(aStream) {}

  virtual void NotifyEvent(
      MediaStreamGraph* aGraph,
      MediaStreamListener::MediaStreamGraphEvent event) override {
    if (event == EVENT_FINISHED) {
      nsCOMPtr<nsIRunnable> r = NS_NewRunnableMethod(
          this, &OutputStreamListener::DoNotifyFinished);
      aGraph->DispatchToMainThreadAfterStreamStateUpdate(r.forget());
    }
  }

  void Forget() {
    MOZ_ASSERT(NS_IsMainThread());
    mDecoder = nullptr;
  }

private:
  void DoNotifyFinished() {
    MOZ_ASSERT(NS_IsMainThread());
    if (!mDecoder) {
      return;
    }

    
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    auto& streams = mDecoder->OutputStreams();
    
    
    for (int32_t i = streams.Length() - 1; i >= 0; --i) {
      auto& os = streams[i];
      MediaStream* p = os.mStream.get();
      if (p == mStream.get()) {
        if (os.mPort) {
          os.mPort->Destroy();
          os.mPort = nullptr;
        }
        streams.RemoveElementAt(i);
        break;
      }
    }
  }

  
  MediaDecoder* mDecoder;
  nsRefPtr<MediaStream> mStream;
};

void
MediaDecoder::OutputStreamData::Init(MediaDecoder* aDecoder,
                                     ProcessedMediaStream* aStream)
{
  mStream = aStream;
  mListener = new OutputStreamListener(aDecoder, aStream);
  aStream->AddListener(mListener);
}

MediaDecoder::OutputStreamData::~OutputStreamData()
{
  mListener->Forget();
}

void MediaDecoder::DestroyDecodedStream()
{
  MOZ_ASSERT(NS_IsMainThread());
  GetReentrantMonitor().AssertCurrentThreadIn();

  
  if (!GetDecodedStream()) {
    return;
  }

  
  
  for (int32_t i = mOutputStreams.Length() - 1; i >= 0; --i) {
    OutputStreamData& os = mOutputStreams[i];
    
    
    MOZ_ASSERT(os.mPort, "Double-delete of the ports!");
    os.mPort->Destroy();
    os.mPort = nullptr;
    
    
    
    if (os.mStream->IsDestroyed()) {
      
      mOutputStreams.RemoveElementAt(i);
    } else {
      os.mStream->ChangeExplicitBlockerCount(1);
    }
  }

  mDecodedStream = nullptr;
}

void MediaDecoder::UpdateStreamBlockingForStateMachinePlaying()
{
  GetReentrantMonitor().AssertCurrentThreadIn();
  if (!mDecodedStream) {
    return;
  }
  bool blockForStateMachineNotPlaying =
    mDecoderStateMachine && !mDecoderStateMachine->IsPlaying() &&
    mDecoderStateMachine->GetState() != MediaDecoderStateMachine::DECODER_STATE_COMPLETED;
  if (blockForStateMachineNotPlaying != mDecodedStream->mHaveBlockedForStateMachineNotPlaying) {
    mDecodedStream->mHaveBlockedForStateMachineNotPlaying = blockForStateMachineNotPlaying;
    int32_t delta = blockForStateMachineNotPlaying ? 1 : -1;
    if (NS_IsMainThread()) {
      mDecodedStream->mStream->ChangeExplicitBlockerCount(delta);
    } else {
      nsCOMPtr<nsIRunnable> runnable =
          NS_NewRunnableMethodWithArg<int32_t>(mDecodedStream->mStream.get(),
              &MediaStream::ChangeExplicitBlockerCount, delta);
      NS_DispatchToMainThread(runnable);
    }
  }
}

void MediaDecoder::RecreateDecodedStream(int64_t aStartTimeUSecs,
                                         MediaStreamGraph* aGraph)
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  DECODER_LOG("RecreateDecodedStream aStartTimeUSecs=%lld!", aStartTimeUSecs);

  if (!aGraph) {
    aGraph = mDecodedStream->mStream->Graph();
  }
  DestroyDecodedStream();

  mDecodedStream = new DecodedStreamData(this,
                                         aStartTimeUSecs,
                                         aGraph->CreateSourceStream(nullptr));

  
  
  
  for (int32_t i = mOutputStreams.Length() - 1; i >= 0; --i) {
    OutputStreamData& os = mOutputStreams[i];
    MOZ_ASSERT(!os.mStream->IsDestroyed(),
        "Should've been removed in DestroyDecodedStream()");
    ConnectDecodedStreamToOutputStream(&os);
  }
  UpdateStreamBlockingForStateMachinePlaying();

  mDecodedStream->mHaveBlockedForPlayState = mPlayState != PLAY_STATE_PLAYING;
  if (mDecodedStream->mHaveBlockedForPlayState) {
    mDecodedStream->mStream->ChangeExplicitBlockerCount(1);
  }
}

void MediaDecoder::AddOutputStream(ProcessedMediaStream* aStream,
                                   bool aFinishWhenEnded)
{
  MOZ_ASSERT(NS_IsMainThread());
  DECODER_LOG("AddOutputStream aStream=%p!", aStream);

  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    if (mDecoderStateMachine) {
      mDecoderStateMachine->SetAudioCaptured();
    }
    if (!GetDecodedStream()) {
      int64_t t = mDecoderStateMachine ?
                  mDecoderStateMachine->GetCurrentTimeUs() : 0;
      RecreateDecodedStream(t, aStream->Graph());
    }
    OutputStreamData* os = mOutputStreams.AppendElement();
    os->Init(this, aStream);
    ConnectDecodedStreamToOutputStream(os);
    if (aFinishWhenEnded) {
      
      aStream->SetAutofinish(true);
    }
  }

  
  
  
  if (mDecoderStateMachine) {
    
    
    ScheduleStateMachineThread();
  }
}

double MediaDecoder::GetDuration()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mInfiniteStream) {
    return std::numeric_limits<double>::infinity();
  }
  if (mDuration >= 0) {
     return static_cast<double>(mDuration) / static_cast<double>(USECS_PER_S);
  }
  return std::numeric_limits<double>::quiet_NaN();
}

int64_t MediaDecoder::GetMediaDuration()
{
  NS_ENSURE_TRUE(GetStateMachine(), -1);
  return GetStateMachine()->GetDuration();
}

void MediaDecoder::SetInfinite(bool aInfinite)
{
  MOZ_ASSERT(NS_IsMainThread());
  mInfiniteStream = aInfinite;
}

bool MediaDecoder::IsInfinite()
{
  MOZ_ASSERT(NS_IsMainThread());
  return mInfiniteStream;
}

MediaDecoder::MediaDecoder() :
  mDecoderPosition(0),
  mPlaybackPosition(0),
  mCurrentTime(0.0),
  mInitialVolume(0.0),
  mInitialPlaybackRate(1.0),
  mInitialPreservesPitch(true),
  mDuration(-1),
  mMediaSeekable(true),
  mSameOriginMedia(false),
  mReentrantMonitor("media.decoder"),
  mPlayState(PLAY_STATE_LOADING),
  mNextState(PLAY_STATE_PAUSED),
  mIgnoreProgressData(false),
  mInfiniteStream(false),
  mOwner(nullptr),
  mPlaybackStatistics(new MediaChannelStatistics()),
  mPinnedForSeek(false),
  mShuttingDown(false),
  mPausedForPlaybackRateNull(false),
  mMinimizePreroll(false),
  mMediaTracksConstructed(false),
  mIsDormant(false),
  mWasEndedWhenEnteredDormant(false),
  mIsHeuristicDormantSupported(
    Preferences::GetBool("media.decoder.heuristic.dormant.enabled", false)),
  mHeuristicDormantTimeout(
    Preferences::GetInt("media.decoder.heuristic.dormant.timeout",
                        DEFAULT_HEURISTIC_DORMANT_TIMEOUT_MSECS)),
  mIsHeuristicDormant(false)
{
  MOZ_COUNT_CTOR(MediaDecoder);
  MOZ_ASSERT(NS_IsMainThread());
  MediaMemoryTracker::AddMediaDecoder(this);
  AbstractThread::EnsureMainThreadSingleton();
#ifdef PR_LOGGING
  if (!gMediaDecoderLog) {
    gMediaDecoderLog = PR_NewLogModule("MediaDecoder");
  }
#endif
  mAudioChannel = AudioChannelService::GetDefaultAudioChannel();
}

bool MediaDecoder::Init(MediaDecoderOwner* aOwner)
{
  MOZ_ASSERT(NS_IsMainThread());
  mOwner = aOwner;
  mVideoFrameContainer = aOwner->GetVideoFrameContainer();
  MediaShutdownManager::Instance().Register(this);
  return true;
}

void MediaDecoder::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mShuttingDown)
    return;

  mShuttingDown = true;

  
  
  
  if (mDecoderStateMachine) {
    mDecoderStateMachine->DispatchShutdown();
  }

  
  
  if (mResource) {
    mResource->Close();
  }

  CancelDormantTimer();

  ChangeState(PLAY_STATE_SHUTDOWN);

  mOwner = nullptr;

  MediaShutdownManager::Instance().Unregister(this);
}

MediaDecoder::~MediaDecoder()
{
  MOZ_ASSERT(NS_IsMainThread());
  {
    
    
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    DestroyDecodedStream();
  }
  MediaMemoryTracker::RemoveMediaDecoder(this);
  UnpinForSeek();
  MOZ_COUNT_DTOR(MediaDecoder);
}

nsresult MediaDecoder::OpenResource(nsIStreamListener** aStreamListener)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (aStreamListener) {
    *aStreamListener = nullptr;
  }

  {
    
    
    
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

    nsresult rv = mResource->Open(aStreamListener);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult MediaDecoder::Load(nsIStreamListener** aStreamListener,
                            MediaDecoder* aCloneDonor)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv = OpenResource(aStreamListener);
  NS_ENSURE_SUCCESS(rv, rv);

  mDecoderStateMachine = CreateStateMachine();
  NS_ENSURE_TRUE(mDecoderStateMachine, NS_ERROR_FAILURE);

  return InitializeStateMachine(aCloneDonor);
}

nsresult MediaDecoder::InitializeStateMachine(MediaDecoder* aCloneDonor)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ASSERTION(mDecoderStateMachine, "Cannot initialize null state machine!");

  MediaDecoder* cloneDonor = static_cast<MediaDecoder*>(aCloneDonor);
  nsresult rv = mDecoderStateMachine->Init(
      cloneDonor ? cloneDonor->mDecoderStateMachine : nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  SetStateMachineParameters();

  return ScheduleStateMachineThread();
}

void MediaDecoder::SetStateMachineParameters()
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  mDecoderStateMachine->SetDuration(mDuration);
  mDecoderStateMachine->SetVolume(mInitialVolume);
  if (GetDecodedStream()) {
    mDecoderStateMachine->SetAudioCaptured();
  }
  SetPlaybackRate(mInitialPlaybackRate);
  mDecoderStateMachine->SetPreservesPitch(mInitialPreservesPitch);
  if (mMinimizePreroll) {
    mDecoderStateMachine->SetMinimizePrerollUntilPlaybackStarts();
  }
}

void MediaDecoder::SetMinimizePrerollUntilPlaybackStarts()
{
  MOZ_ASSERT(NS_IsMainThread());
  mMinimizePreroll = true;
}

nsresult MediaDecoder::ScheduleStateMachineThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ASSERTION(mDecoderStateMachine,
               "Must have state machine to start state machine thread");
  NS_ENSURE_STATE(mDecoderStateMachine);

  if (mShuttingDown)
    return NS_OK;
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  mDecoderStateMachine->ScheduleStateMachine();
  return NS_OK;
}

nsresult MediaDecoder::Play()
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  UpdateDormantState(false , true );

  NS_ASSERTION(mDecoderStateMachine != nullptr, "Should have state machine.");
  if (mPausedForPlaybackRateNull) {
    return NS_OK;
  }
  ScheduleStateMachineThread();
  if (IsEnded()) {
    return Seek(0, SeekTarget::PrevSyncPoint);
  } else if (mPlayState == PLAY_STATE_LOADING || mPlayState == PLAY_STATE_SEEKING) {
    mNextState = PLAY_STATE_PLAYING;
    return NS_OK;
  }

  ChangeState(PLAY_STATE_PLAYING);
  return NS_OK;
}

nsresult MediaDecoder::Seek(double aTime, SeekTarget::Type aSeekType)
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  UpdateDormantState(false , true );

  MOZ_ASSERT(aTime >= 0.0, "Cannot seek to a negative value.");

  int64_t timeUsecs = 0;
  nsresult rv = SecondsToUsecs(aTime, timeUsecs);
  NS_ENSURE_SUCCESS(rv, rv);

  mRequestedSeekTarget = SeekTarget(timeUsecs, aSeekType);
  mCurrentTime = aTime;
  mWasEndedWhenEnteredDormant = false;

  
  if (mPlayState != PLAY_STATE_LOADING) {
    mSeekRequest.DisconnectIfExists();
    bool paused = false;
    if (mOwner) {
      paused = mOwner->GetPaused();
    }
    mNextState = paused ? PLAY_STATE_PAUSED : PLAY_STATE_PLAYING;
    PinForSeek();
    ChangeState(PLAY_STATE_SEEKING);
  }

  return ScheduleStateMachineThread();
}

bool MediaDecoder::IsLogicallyPlaying()
{
  GetReentrantMonitor().AssertCurrentThreadIn();
  return mPlayState == PLAY_STATE_PLAYING ||
         mNextState == PLAY_STATE_PLAYING;
}

double MediaDecoder::GetCurrentTime()
{
  MOZ_ASSERT(NS_IsMainThread());
  return mCurrentTime;
}

already_AddRefed<nsIPrincipal> MediaDecoder::GetCurrentPrincipal()
{
  MOZ_ASSERT(NS_IsMainThread());
  return mResource ? mResource->GetCurrentPrincipal() : nullptr;
}

void MediaDecoder::QueueMetadata(int64_t aPublishTime,
                                 nsAutoPtr<MediaInfo> aInfo,
                                 nsAutoPtr<MetadataTags> aTags)
{
  MOZ_ASSERT(OnDecodeTaskQueue());
  GetReentrantMonitor().AssertCurrentThreadIn();
  mDecoderStateMachine->QueueMetadata(aPublishTime, aInfo, aTags);
}

bool
MediaDecoder::IsExpectingMoreData()
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  
  if (!mResource) {
    return true;
  }

  
  if (mResource->IsDataCachedToEndOfResource(mDecoderPosition)) {
    return false;
  }

  
  return !mResource->IsSuspended();
}

void MediaDecoder::MetadataLoaded(nsAutoPtr<MediaInfo> aInfo,
                                  nsAutoPtr<MetadataTags> aTags,
                                  MediaDecoderEventVisibility aEventVisibility)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mShuttingDown) {
    return;
  }

  DECODER_LOG("MetadataLoaded, channels=%u rate=%u hasAudio=%d hasVideo=%d",
              aInfo->mAudio.mChannels, aInfo->mAudio.mRate,
              aInfo->HasAudio(), aInfo->HasVideo());

  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    mDuration = mDecoderStateMachine ? mDecoderStateMachine->GetDuration() : -1;
    
    UpdatePlaybackRate();
  }

  if (mDuration == -1) {
    SetInfinite(true);
  }

  mInfo = aInfo.forget();
  ConstructMediaTracks();

  if (mOwner) {
    
    
    Invalidate();
    if (aEventVisibility != MediaDecoderEventVisibility::Suppressed) {
      mOwner->MetadataLoaded(mInfo, nsAutoPtr<const MetadataTags>(aTags.forget()));
    }
  }
}

const char*
MediaDecoder::PlayStateStr()
{
  switch (mPlayState) {
    case PLAY_STATE_START: return "PLAY_STATE_START";
    case PLAY_STATE_LOADING: return "PLAY_STATE_LOADING";
    case PLAY_STATE_PAUSED: return "PLAY_STATE_PAUSED";
    case PLAY_STATE_PLAYING: return "PLAY_STATE_PLAYING";
    case PLAY_STATE_SEEKING: return "PLAY_STATE_SEEKING";
    case PLAY_STATE_ENDED: return "PLAY_STATE_ENDED";
    case PLAY_STATE_SHUTDOWN: return "PLAY_STATE_SHUTDOWN";
    default: return "INVALID_PLAY_STATE";
  }
}

void MediaDecoder::FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo,
                                    MediaDecoderEventVisibility aEventVisibility)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mShuttingDown) {
    return;
  }

  DECODER_LOG("FirstFrameLoaded, channels=%u rate=%u hasAudio=%d hasVideo=%d mPlayState=%s mIsDormant=%d",
              aInfo->mAudio.mChannels, aInfo->mAudio.mRate,
              aInfo->HasAudio(), aInfo->HasVideo(), PlayStateStr(), mIsDormant);

  mInfo = aInfo.forget();

  if (mOwner) {
    Invalidate();
    if (aEventVisibility != MediaDecoderEventVisibility::Suppressed) {
      mOwner->FirstFrameLoaded();
    }
  }

  
  mResource->EnsureCacheUpToDate();

  
  
  
  
  if (mPlayState == PLAY_STATE_LOADING && !mIsDormant) {
    if (mRequestedSeekTarget.IsValid()) {
      ChangeState(PLAY_STATE_SEEKING);
    }
    else {
      ChangeState(mNextState);
    }
  }

  
  
  NotifySuspendedStatusChanged();
}

void MediaDecoder::ResetConnectionState()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mShuttingDown)
    return;

  if (mOwner) {
    
    mOwner->ResetConnectionState();
  }

  
  
  
  Shutdown();
}

void MediaDecoder::NetworkError()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mShuttingDown)
    return;

  if (mOwner)
    mOwner->NetworkError();

  Shutdown();
}

void MediaDecoder::DecodeError()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mShuttingDown)
    return;

  if (mOwner)
    mOwner->DecodeError();

  Shutdown();
}

void MediaDecoder::UpdateSameOriginStatus(bool aSameOrigin)
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  mSameOriginMedia = aSameOrigin;
}

bool MediaDecoder::IsSameOriginMedia()
{
  GetReentrantMonitor().AssertCurrentThreadIn();
  return mSameOriginMedia;
}

bool MediaDecoder::IsSeeking() const
{
  MOZ_ASSERT(NS_IsMainThread());
  return !mIsDormant && (mPlayState == PLAY_STATE_SEEKING ||
    (mPlayState == PLAY_STATE_LOADING && mRequestedSeekTarget.IsValid()));
}

bool MediaDecoder::IsEndedOrShutdown() const
{
  MOZ_ASSERT(NS_IsMainThread());
  return IsEnded() || mPlayState == PLAY_STATE_SHUTDOWN;
}

bool MediaDecoder::IsEnded() const
{
  return mPlayState == PLAY_STATE_ENDED ||
         (mWasEndedWhenEnteredDormant && (mPlayState != PLAY_STATE_SHUTDOWN));
}

void MediaDecoder::PlaybackEnded()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mShuttingDown ||
      mPlayState == PLAY_STATE_SEEKING ||
      mPlayState == PLAY_STATE_LOADING) {
    return;
  }

  PlaybackPositionChanged();
  ChangeState(PLAY_STATE_ENDED);
  InvalidateWithFlags(VideoFrameContainer::INVALIDATE_FORCE);

  UpdateReadyStateForData();
  if (mOwner)  {
    mOwner->PlaybackEnded();
  }

  
  
  if (IsInfinite()) {
    SetInfinite(false);
  }
}

NS_IMETHODIMP MediaDecoder::Observe(nsISupports *aSubjet,
                                        const char *aTopic,
                                        const char16_t *someData)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    Shutdown();
  }

  return NS_OK;
}

MediaDecoder::Statistics
MediaDecoder::GetStatistics()
{
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

double MediaDecoder::ComputePlaybackRate(bool* aReliable)
{
  GetReentrantMonitor().AssertCurrentThreadIn();
  MOZ_ASSERT(NS_IsMainThread() || OnStateMachineTaskQueue() || OnDecodeTaskQueue());

  int64_t length = mResource ? mResource->GetLength() : -1;
  if (mDuration >= 0 && length >= 0) {
    *aReliable = true;
    return length * static_cast<double>(USECS_PER_S) / mDuration;
  }
  return mPlaybackStatistics->GetRateAtLastStop(aReliable);
}

void MediaDecoder::UpdatePlaybackRate()
{
  MOZ_ASSERT(NS_IsMainThread() || OnStateMachineTaskQueue());
  GetReentrantMonitor().AssertCurrentThreadIn();
  if (!mResource)
    return;
  bool reliable;
  uint32_t rate = uint32_t(ComputePlaybackRate(&reliable));
  if (reliable) {
    
    rate = std::max(rate, 1u);
  }
  else {
    
    
    rate = std::max(rate, 10000u);
  }
  mResource->SetPlaybackRate(rate);
}

void MediaDecoder::NotifySuspendedStatusChanged()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mResource && mOwner) {
    bool suspended = mResource->IsSuspendedByCache();
    mOwner->NotifySuspendedByCache(suspended);
    UpdateReadyStateForData();
  }
}

void MediaDecoder::NotifyBytesDownloaded()
{
  MOZ_ASSERT(NS_IsMainThread());
  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    UpdatePlaybackRate();
  }
  if (mOwner) {
    mOwner->DownloadProgressed();
  }
}

void MediaDecoder::NotifyDownloadEnded(nsresult aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());

  DECODER_LOG("NotifyDownloadEnded, status=%x", aStatus);

  if (aStatus == NS_BINDING_ABORTED) {
    
    if (mOwner) {
      mOwner->LoadAborted();
    }
    return;
  }

  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    UpdatePlaybackRate();
  }

  if (NS_SUCCEEDED(aStatus)) {
    
    
    
    
  } else if (aStatus != NS_BASE_STREAM_CLOSED) {
    NetworkError();
  }
}

void MediaDecoder::NotifyPrincipalChanged()
{
  if (mOwner) {
    mOwner->NotifyDecoderPrincipalChanged();
  }
}

void MediaDecoder::NotifyBytesConsumed(int64_t aBytes, int64_t aOffset)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mShuttingDown) {
    return;
  }

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  MOZ_ASSERT(mDecoderStateMachine);
  if (mIgnoreProgressData) {
    return;
  }
  if (aOffset >= mDecoderPosition) {
    mPlaybackStatistics->AddBytes(aBytes);
  }
  mDecoderPosition = aOffset + aBytes;
}

void MediaDecoder::UpdateReadyStateForData()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mOwner || mShuttingDown || !mDecoderStateMachine) {
    return;
  }
  MediaDecoderOwner::NextFrameStatus frameStatus =
    mDecoderStateMachine->GetNextFrameStatus();
  mOwner->UpdateReadyStateForData(frameStatus);
}

void MediaDecoder::OnSeekResolvedInternal(bool aAtEnd, MediaDecoderEventVisibility aEventVisibility)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mShuttingDown)
    return;

  bool fireEnded = false;
  bool seekWasAborted = false;
  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

    
    
    if (mRequestedSeekTarget.IsValid()) {
      ChangeState(PLAY_STATE_SEEKING);
      seekWasAborted = true;
    } else {
      UnpinForSeek();
      fireEnded = aAtEnd;
      if (aAtEnd) {
        ChangeState(PLAY_STATE_ENDED);
      } else if (aEventVisibility != MediaDecoderEventVisibility::Suppressed) {
        ChangeState(aAtEnd ? PLAY_STATE_ENDED : mNextState);
      }
    }
  }

  PlaybackPositionChanged(aEventVisibility);

  if (mOwner) {
    UpdateReadyStateForData();
    if (!seekWasAborted && (aEventVisibility != MediaDecoderEventVisibility::Suppressed)) {
      mOwner->SeekCompleted();
      if (fireEnded) {
        mOwner->PlaybackEnded();
      }
    }
  }
}

void MediaDecoder::SeekingStarted(MediaDecoderEventVisibility aEventVisibility)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mShuttingDown)
    return;

  if (mOwner) {
    UpdateReadyStateForData();
    if (aEventVisibility != MediaDecoderEventVisibility::Suppressed) {
      mOwner->SeekStarted();
    }
  }
}

void MediaDecoder::ChangeState(PlayState aState)
{
  MOZ_ASSERT(NS_IsMainThread());
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

  DECODER_LOG("ChangeState %s => %s",
              gPlayStateStr[mPlayState], gPlayStateStr[aState]);
  mPlayState = aState;

  if (mPlayState == PLAY_STATE_PLAYING) {
    ConstructMediaTracks();
  } else if (IsEnded()) {
    RemoveMediaTracks();
  }

  ApplyStateToStateMachine(mPlayState);

  CancelDormantTimer();
  
  StartDormantTimer();

  GetReentrantMonitor().NotifyAll();
}

void MediaDecoder::ApplyStateToStateMachine(PlayState aState)
{
  MOZ_ASSERT(NS_IsMainThread());
  GetReentrantMonitor().AssertCurrentThreadIn();

  if (mDecoderStateMachine) {
    switch (aState) {
      case PLAY_STATE_PLAYING:
        mDecoderStateMachine->Play();
        break;
      case PLAY_STATE_SEEKING:
        mSeekRequest.Begin(ProxyMediaCall(mDecoderStateMachine->TaskQueue(),
                                          mDecoderStateMachine.get(), __func__,
                                          &MediaDecoderStateMachine::Seek, mRequestedSeekTarget)
          ->RefableThen(AbstractThread::MainThread(), __func__, this,
                        &MediaDecoder::OnSeekResolved, &MediaDecoder::OnSeekRejected));
        mRequestedSeekTarget.Reset();
        break;
      default:
        
        
        ScheduleStateMachineThread();
        break;
    }
  }
}

void MediaDecoder::PlaybackPositionChanged(MediaDecoderEventVisibility aEventVisibility)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mShuttingDown)
    return;

  double lastTime = mCurrentTime;

  
  
  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    if (mDecoderStateMachine) {
      
      
      
      if (!IsSeeking() && mPlayState != PLAY_STATE_PAUSED) {
        
        
        
        
        
        
        
        mCurrentTime = mDecoderStateMachine->GetCurrentTime();
      }
      mDecoderStateMachine->ClearPositionChangeFlag();
    }
  }

  
  
  
  
  Invalidate();

  if (mOwner &&
      (aEventVisibility != MediaDecoderEventVisibility::Suppressed) &&
      lastTime != mCurrentTime) {
    FireTimeUpdate();
  }
}

void MediaDecoder::DurationChanged()
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  int64_t oldDuration = mDuration;
  mDuration = mDecoderStateMachine ? mDecoderStateMachine->GetDuration() : -1;
  
  UpdatePlaybackRate();

  SetInfinite(mDuration == -1);

  if (mOwner && oldDuration != mDuration && !IsInfinite()) {
    DECODER_LOG("Duration changed to %lld", mDuration);
    mOwner->DispatchAsyncEvent(NS_LITERAL_STRING("durationchange"));
  }
}

void MediaDecoder::SetDuration(double aDuration)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mozilla::IsInfinite(aDuration)) {
    SetInfinite(true);
  } else if (IsNaN(aDuration)) {
    mDuration = -1;
    SetInfinite(true);
  } else {
    mDuration = static_cast<int64_t>(NS_round(aDuration * static_cast<double>(USECS_PER_S)));
  }

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (mDecoderStateMachine) {
    mDecoderStateMachine->SetDuration(mDuration);
  }

  
  UpdatePlaybackRate();
}

void MediaDecoder::SetMediaDuration(int64_t aDuration)
{
  NS_ENSURE_TRUE_VOID(GetStateMachine());
  GetStateMachine()->SetDuration(aDuration);
}

void MediaDecoder::UpdateEstimatedMediaDuration(int64_t aDuration)
{
  if (mPlayState <= PLAY_STATE_LOADING) {
    return;
  }
  NS_ENSURE_TRUE_VOID(GetStateMachine());
  GetStateMachine()->UpdateEstimatedDuration(aDuration);
}

void MediaDecoder::SetMediaSeekable(bool aMediaSeekable) {
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  mMediaSeekable = aMediaSeekable;
}

bool
MediaDecoder::IsTransportSeekable()
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  return GetResource()->IsTransportSeekable();
}

bool MediaDecoder::IsMediaSeekable()
{
  NS_ENSURE_TRUE(GetStateMachine(), false);
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  return mMediaSeekable;
}

nsresult MediaDecoder::GetSeekable(dom::TimeRanges* aSeekable)
{
  double initialTime = 0.0;

  
  
  
  if (!IsMediaSeekable()) {
    return NS_OK;
  } else if (!IsTransportSeekable()) {
    return GetBuffered(aSeekable);
  } else {
    double end = IsInfinite() ? std::numeric_limits<double>::infinity()
                              : initialTime + GetDuration();
    aSeekable->Add(initialTime, end);
    return NS_OK;
  }
}

void MediaDecoder::SetFragmentEndTime(double aTime)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mDecoderStateMachine) {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    mDecoderStateMachine->SetFragmentEndTime(static_cast<int64_t>(aTime * USECS_PER_S));
  }
}

void MediaDecoder::SetMediaEndTime(int64_t aTime)
{
  NS_ENSURE_TRUE_VOID(GetStateMachine());
  GetStateMachine()->SetMediaEndTime(aTime);
}

void MediaDecoder::Suspend()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mResource) {
    mResource->Suspend(true);
  }
}

void MediaDecoder::Resume(bool aForceBuffering)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mResource) {
    mResource->Resume();
  }
  if (aForceBuffering) {
    if (mDecoderStateMachine) {
      mDecoderStateMachine->DispatchStartBuffering();
    }
  }
}

void MediaDecoder::StopProgressUpdates()
{
  MOZ_ASSERT(OnStateMachineTaskQueue() || OnDecodeTaskQueue());
  GetReentrantMonitor().AssertCurrentThreadIn();
  mIgnoreProgressData = true;
  if (mResource) {
    mResource->SetReadMode(MediaCacheStream::MODE_METADATA);
  }
}

void MediaDecoder::StartProgressUpdates()
{
  MOZ_ASSERT(OnStateMachineTaskQueue() || OnDecodeTaskQueue());
  GetReentrantMonitor().AssertCurrentThreadIn();
  mIgnoreProgressData = false;
  if (mResource) {
    mResource->SetReadMode(MediaCacheStream::MODE_PLAYBACK);
  }
}

void MediaDecoder::SetLoadInBackground(bool aLoadInBackground)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mResource) {
    mResource->SetLoadInBackground(aLoadInBackground);
  }
}

void MediaDecoder::UpdatePlaybackOffset(int64_t aOffset)
{
  GetReentrantMonitor().AssertCurrentThreadIn();
  mPlaybackPosition = aOffset;
}

bool MediaDecoder::OnStateMachineTaskQueue() const
{
  return mDecoderStateMachine->OnTaskQueue();
}

void MediaDecoder::SetPlaybackRate(double aPlaybackRate)
{
  if (aPlaybackRate == 0.0) {
    mPausedForPlaybackRateNull = true;
    mInitialPlaybackRate = aPlaybackRate;
    Pause();
    return;
  } else if (mPausedForPlaybackRateNull) {
    
    mPausedForPlaybackRateNull = false;
    
    
    if (mOwner && !mOwner->GetPaused()) {
      Play();
    }
  }

  if (mDecoderStateMachine) {
    mDecoderStateMachine->SetPlaybackRate(aPlaybackRate);
  } else {
    mInitialPlaybackRate = aPlaybackRate;
  }
}

void MediaDecoder::SetPreservesPitch(bool aPreservesPitch)
{
  if (mDecoderStateMachine) {
    mDecoderStateMachine->SetPreservesPitch(aPreservesPitch);
  } else {
    mInitialPreservesPitch = aPreservesPitch;
  }
}

bool MediaDecoder::OnDecodeTaskQueue() const {
  NS_WARN_IF_FALSE(mDecoderStateMachine, "mDecoderStateMachine is null");
  return mDecoderStateMachine ? mDecoderStateMachine->OnDecodeTaskQueue() : false;
}

ReentrantMonitor& MediaDecoder::GetReentrantMonitor() {
  return mReentrantMonitor;
}

ImageContainer* MediaDecoder::GetImageContainer()
{
  return mVideoFrameContainer ? mVideoFrameContainer->GetImageContainer() : nullptr;
}

void MediaDecoder::InvalidateWithFlags(uint32_t aFlags)
{
  if (mVideoFrameContainer) {
    mVideoFrameContainer->InvalidateWithFlags(aFlags);
  }
}

void MediaDecoder::Invalidate()
{
  if (mVideoFrameContainer) {
    mVideoFrameContainer->Invalidate();
  }
}



nsresult MediaDecoder::GetBuffered(dom::TimeRanges* aBuffered) {
  NS_ENSURE_TRUE(mDecoderStateMachine && !mShuttingDown, NS_ERROR_FAILURE);
  return mDecoderStateMachine->GetBuffered(aBuffered);
}

size_t MediaDecoder::SizeOfVideoQueue() {
  if (mDecoderStateMachine) {
    return mDecoderStateMachine->SizeOfVideoQueue();
  }
  return 0;
}

size_t MediaDecoder::SizeOfAudioQueue() {
  if (mDecoderStateMachine) {
    return mDecoderStateMachine->SizeOfAudioQueue();
  }
  return 0;
}

void MediaDecoder::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) {
  if (mDecoderStateMachine) {
    mDecoderStateMachine->NotifyDataArrived(aBuffer, aLength, aOffset);
  }
  UpdateReadyStateForData();
}


MediaDecoderStateMachine* MediaDecoder::GetStateMachine() const {
  return mDecoderStateMachine;
}

void
MediaDecoder::NotifyWaitingForResourcesStatusChanged()
{
  if (mDecoderStateMachine) {
    RefPtr<nsRunnable> task =
      NS_NewRunnableMethod(mDecoderStateMachine,
                           &MediaDecoderStateMachine::NotifyWaitingForResourcesStatusChanged);
    mDecoderStateMachine->TaskQueue()->Dispatch(task.forget());
  }
}

bool MediaDecoder::IsShutdown() const {
  NS_ENSURE_TRUE(GetStateMachine(), true);
  return GetStateMachine()->IsShutdown();
}


void MediaDecoder::BreakCycles() {
  mDecoderStateMachine = nullptr;
}

MediaDecoderOwner* MediaDecoder::GetMediaOwner() const
{
  return mOwner;
}

void MediaDecoder::FireTimeUpdate()
{
  if (!mOwner)
    return;
  mOwner->FireTimeUpdate(true);
}

void MediaDecoder::PinForSeek()
{
  MediaResource* resource = GetResource();
  if (!resource || mPinnedForSeek) {
    return;
  }
  mPinnedForSeek = true;
  resource->Pin();
}

void MediaDecoder::UnpinForSeek()
{
  MediaResource* resource = GetResource();
  if (!resource || !mPinnedForSeek) {
    return;
  }
  mPinnedForSeek = false;
  resource->Unpin();
}

bool MediaDecoder::CanPlayThrough()
{
  Statistics stats = GetStatistics();

  NS_ASSERTION(mDecoderStateMachine, "CanPlayThrough should have state machine!");
  if (mDecoderStateMachine->IsRealTime() ||
      (stats.mTotalBytes < 0 && stats.mDownloadRateReliable) ||
      (stats.mTotalBytes >= 0 && stats.mTotalBytes == stats.mDownloadPosition)) {
    return true;
  }
  if (!stats.mDownloadRateReliable || !stats.mPlaybackRateReliable) {
    return false;
  }
  int64_t bytesToDownload = stats.mTotalBytes - stats.mDownloadPosition;
  int64_t bytesToPlayback = stats.mTotalBytes - stats.mPlaybackPosition;
  double timeToDownload = bytesToDownload / stats.mDownloadRate;
  double timeToPlay = bytesToPlayback / stats.mPlaybackRate;

  if (timeToDownload > timeToPlay) {
    
    
    return false;
  }

  
  
  
  
  
  
  
  int64_t readAheadMargin =
    static_cast<int64_t>(stats.mPlaybackRate * CAN_PLAY_THROUGH_MARGIN);
  return stats.mDownloadPosition > stats.mPlaybackPosition + readAheadMargin;
}

#ifdef MOZ_EME
nsresult
MediaDecoder::SetCDMProxy(CDMProxy* aProxy)
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  MOZ_ASSERT(NS_IsMainThread());
  mProxy = aProxy;
  
  NotifyWaitingForResourcesStatusChanged();
  return NS_OK;
}

CDMProxy*
MediaDecoder::GetCDMProxy()
{
  GetReentrantMonitor().AssertCurrentThreadIn();
  return mProxy;
}
#endif

#ifdef MOZ_RAW
bool
MediaDecoder::IsRawEnabled()
{
  return Preferences::GetBool("media.raw.enabled");
}
#endif

bool
MediaDecoder::IsOpusEnabled()
{
  return Preferences::GetBool("media.opus.enabled");
}

bool
MediaDecoder::IsOggEnabled()
{
  return Preferences::GetBool("media.ogg.enabled");
}

#ifdef MOZ_WAVE
bool
MediaDecoder::IsWaveEnabled()
{
  return Preferences::GetBool("media.wave.enabled");
}
#endif

#ifdef MOZ_WEBM
bool
MediaDecoder::IsWebMEnabled()
{
  return Preferences::GetBool("media.webm.enabled");
}
#endif

#ifdef NECKO_PROTOCOL_rtsp
bool
MediaDecoder::IsRtspEnabled()
{
  
  return (Preferences::GetBool("media.rtsp.enabled", false) && IsOmxEnabled());
}
#endif

#ifdef MOZ_GSTREAMER
bool
MediaDecoder::IsGStreamerEnabled()
{
  return Preferences::GetBool("media.gstreamer.enabled");
}
#endif

#ifdef MOZ_OMX_DECODER
bool
MediaDecoder::IsOmxEnabled()
{
  return Preferences::GetBool("media.omx.enabled", false);
}

bool
MediaDecoder::IsOmxAsyncEnabled()
{
#if ANDROID_VERSION >= 16
  return Preferences::GetBool("media.omx.async.enabled", false);
#else
  return false;
#endif
}
#endif

#ifdef MOZ_ANDROID_OMX
bool
MediaDecoder::IsAndroidMediaEnabled()
{
  return Preferences::GetBool("media.plugins.enabled");
}
#endif

#ifdef MOZ_WMF
bool
MediaDecoder::IsWMFEnabled()
{
  return WMFDecoder::IsEnabled();
}
#endif

#ifdef MOZ_APPLEMEDIA
bool
MediaDecoder::IsAppleMP3Enabled()
{
  return Preferences::GetBool("media.apple.mp3.enabled");
}
#endif

NS_IMETHODIMP
MediaMemoryTracker::CollectReports(nsIHandleReportCallback* aHandleReport,
                                   nsISupports* aData, bool aAnonymize)
{
  int64_t video = 0, audio = 0;
  size_t resources = 0;
  DecodersArray& decoders = Decoders();
  for (size_t i = 0; i < decoders.Length(); ++i) {
    MediaDecoder* decoder = decoders[i];
    video += decoder->SizeOfVideoQueue();
    audio += decoder->SizeOfAudioQueue();

    if (decoder->GetResource()) {
      resources += decoder->GetResource()->SizeOfIncludingThis(MallocSizeOf);
    }
  }

#define REPORT(_path, _amount, _desc)                                         \
  do {                                                                        \
      nsresult rv;                                                            \
      rv = aHandleReport->Callback(EmptyCString(), NS_LITERAL_CSTRING(_path), \
                                   KIND_HEAP, UNITS_BYTES, _amount,           \
                                   NS_LITERAL_CSTRING(_desc), aData);         \
      NS_ENSURE_SUCCESS(rv, rv);                                              \
  } while (0)

  REPORT("explicit/media/decoded/video", video,
         "Memory used by decoded video frames.");

  REPORT("explicit/media/decoded/audio", audio,
         "Memory used by decoded audio chunks.");

  REPORT("explicit/media/resources", resources,
         "Memory used by media resources including streaming buffers, caches, "
         "etc.");

#undef REPORT

  return NS_OK;
}

MediaDecoderOwner*
MediaDecoder::GetOwner()
{
  MOZ_ASSERT(NS_IsMainThread());
  return mOwner;
}

void
MediaDecoder::ConstructMediaTracks()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mMediaTracksConstructed) {
    return;
  }

  if (!mOwner || !mInfo) {
    return;
  }

  HTMLMediaElement* element = mOwner->GetMediaElement();
  if (!element) {
    return;
  }

  mMediaTracksConstructed = true;

  AudioTrackList* audioList = element->AudioTracks();
  if (audioList && mInfo->HasAudio()) {
    TrackInfo info = mInfo->mAudio.mTrackInfo;
    nsRefPtr<AudioTrack> track = MediaTrackList::CreateAudioTrack(
    info.mId, info.mKind, info.mLabel, info.mLanguage, info.mEnabled);

    audioList->AddTrack(track);
  }

  VideoTrackList* videoList = element->VideoTracks();
  if (videoList && mInfo->HasVideo()) {
    TrackInfo info = mInfo->mVideo.mTrackInfo;
    nsRefPtr<VideoTrack> track = MediaTrackList::CreateVideoTrack(
    info.mId, info.mKind, info.mLabel, info.mLanguage);

    videoList->AddTrack(track);
    track->SetEnabledInternal(info.mEnabled, MediaTrack::FIRE_NO_EVENTS);
  }
}

void
MediaDecoder::RemoveMediaTracks()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mOwner) {
    return;
  }

  HTMLMediaElement* element = mOwner->GetMediaElement();
  if (!element) {
    return;
  }

  AudioTrackList* audioList = element->AudioTracks();
  if (audioList) {
    audioList->RemoveTracks();
  }

  VideoTrackList* videoList = element->VideoTracks();
  if (videoList) {
    videoList->RemoveTracks();
  }

  mMediaTracksConstructed = false;
}

MediaMemoryTracker::MediaMemoryTracker()
{
}

void
MediaMemoryTracker::InitMemoryReporter()
{
  RegisterWeakMemoryReporter(this);
}

MediaMemoryTracker::~MediaMemoryTracker()
{
  UnregisterWeakMemoryReporter(this);
}

} 


#undef DECODER_LOG
