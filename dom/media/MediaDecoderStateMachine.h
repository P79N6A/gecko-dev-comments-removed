
















































































#if !defined(MediaDecoderStateMachine_h__)
#define MediaDecoderStateMachine_h__

#include "mozilla/Attributes.h"
#include "nsThreadUtils.h"
#include "MediaDecoder.h"
#include "mozilla/ReentrantMonitor.h"
#include "MediaDecoderReader.h"
#include "MediaDecoderOwner.h"
#include "MediaMetadataManager.h"
#include "mozilla/RollingMean.h"
#include "MediaTimer.h"
#include "StateMirroring.h"
#include "DecodedStream.h"

namespace mozilla {

class AudioSegment;
class MediaTaskQueue;
class AudioSink;

extern PRLogModuleInfo* gMediaDecoderLog;
extern PRLogModuleInfo* gMediaSampleLog;













class MediaDecoderStateMachine
{
  friend class AudioSink;
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaDecoderStateMachine)
public:
  typedef MediaDecoderReader::AudioDataPromise AudioDataPromise;
  typedef MediaDecoderReader::VideoDataPromise VideoDataPromise;
  typedef MediaDecoderOwner::NextFrameStatus NextFrameStatus;
  MediaDecoderStateMachine(MediaDecoder* aDecoder,
                           MediaDecoderReader* aReader,
                           bool aRealTime = false);

  nsresult Init(MediaDecoderStateMachine* aCloneDonor);

  
  enum State {
    DECODER_STATE_DECODING_NONE,
    DECODER_STATE_DECODING_METADATA,
    DECODER_STATE_WAIT_FOR_RESOURCES,
    DECODER_STATE_WAIT_FOR_CDM,
    DECODER_STATE_DECODING_FIRSTFRAME,
    DECODER_STATE_DORMANT,
    DECODER_STATE_DECODING,
    DECODER_STATE_SEEKING,
    DECODER_STATE_BUFFERING,
    DECODER_STATE_COMPLETED,
    DECODER_STATE_SHUTDOWN,
    DECODER_STATE_ERROR
  };

  State GetState() {
    AssertCurrentThreadInMonitor();
    return mState;
  }

  DecodedStreamData* GetDecodedStream() const;

  void AddOutputStream(ProcessedMediaStream* aStream, bool aFinishWhenEnded);

  
  bool IsDormantNeeded();
  
  void SetDormant(bool aDormant);

private:
  
  
  
  void InitializationTask();

  void DispatchAudioCaptured();

  
  
  
  void RecreateDecodedStream(MediaStreamGraph* aGraph);

  void Shutdown();
public:

  void DispatchShutdown()
  {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::Shutdown);
    TaskQueue()->Dispatch(runnable.forget());
  }

  void FinishShutdown();

  bool IsRealTime() const;

  
  
  int64_t GetDuration();

  
  
  
  
  int64_t GetEndTime();

  
  
  
  
  
  
  void SetDuration(media::TimeUnit aDuration);

  
  
  bool OnDecodeTaskQueue() const;
  bool OnTaskQueue() const;

  
  
  nsRefPtr<MediaDecoder::SeekPromise> Seek(SeekTarget aTarget);

  
  
  
  void ClearPositionChangeFlag();

  
  
  
  
  
  void UpdatePlaybackPosition(int64_t aTime);

private:
  
  
  
  void StartBuffering();
public:

  void DispatchStartBuffering()
  {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::StartBuffering);
    TaskQueue()->Dispatch(runnable.forget());
  }

  
  
  bool HasAudio() const {
    AssertCurrentThreadInMonitor();
    return mInfo.HasAudio();
  }

  
  
  bool HasVideo() const {
    AssertCurrentThreadInMonitor();
    return mInfo.HasVideo();
  }

  
  bool HaveNextFrameData();

  
  bool IsBuffering() const {
    MOZ_ASSERT(OnTaskQueue());
    AssertCurrentThreadInMonitor();
    return mState == DECODER_STATE_BUFFERING;
  }

  
  bool IsSeeking() const {
    MOZ_ASSERT(OnTaskQueue());
    AssertCurrentThreadInMonitor();
    return mState == DECODER_STATE_SEEKING;
  }

  media::TimeIntervals GetBuffered() {
    
    
    
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    if (mStartTime < 0) {
      return media::TimeIntervals();
    }

    return mReader->GetBuffered();
  }

  size_t SizeOfVideoQueue() {
    if (mReader) {
      return mReader->SizeOfVideoQueueInBytes();
    }
    return 0;
  }

  size_t SizeOfAudioQueue() {
    if (mReader) {
      return mReader->SizeOfAudioQueueInBytes();
    }
    return 0;
  }

  void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);

  
  MediaTaskQueue* TaskQueue() const { return mTaskQueue; }

  
  
  void ScheduleStateMachineWithLockAndWakeDecoder();

  
  
  
  
  
  void ScheduleStateMachine();
  void ScheduleStateMachineCrossThread()
  {
    nsCOMPtr<nsIRunnable> task =
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::RunStateMachine);
    TaskQueue()->Dispatch(task.forget());
  }

  
  
  
  void ScheduleStateMachineIn(int64_t aMicroseconds);

  void OnDelayedSchedule()
  {
    MOZ_ASSERT(OnTaskQueue());
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDelayedScheduler.CompleteRequest();
    ScheduleStateMachine();
  }

  void NotReached() { MOZ_DIAGNOSTIC_ASSERT(false); }

  
  void SetFragmentEndTime(int64_t aEndTime);

  
  void BreakCycles() {
    MOZ_ASSERT(NS_IsMainThread());
    if (mReader) {
      mReader->BreakCycles();
    }
    {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      mDecodedStream.DestroyData();
    }
    mDecoder = nullptr;
  }

  
  
  void SendStreamData();
  void FinishStreamData();
  bool HaveEnoughDecodedAudio(int64_t aAmpleAudioUSecs);
  bool HaveEnoughDecodedVideo();

  
  
  bool IsShutdown();

  void QueueMetadata(int64_t aPublishTime,
                     nsAutoPtr<MediaInfo> aInfo,
                     nsAutoPtr<MetadataTags> aTags);

  
  
  bool IsPlaying() const;

  
  
  void NotifyWaitingForResourcesStatusChanged();

  
  
  
  
  
  void DispatchMinimizePrerollUntilPlaybackStarts()
  {
    nsRefPtr<MediaDecoderStateMachine> self = this;
    nsCOMPtr<nsIRunnable> r = NS_NewRunnableFunction([self] () -> void
    {
      MOZ_ASSERT(self->OnTaskQueue());
      ReentrantMonitorAutoEnter mon(self->mDecoder->GetReentrantMonitor());
      self->mMinimizePreroll = true;

      
      
      MOZ_DIAGNOSTIC_ASSERT(self->mPlayState == MediaDecoder::PLAY_STATE_LOADING);
    });
    TaskQueue()->Dispatch(r.forget());
  }

  void OnAudioDecoded(AudioData* aSample);
  void OnVideoDecoded(VideoData* aSample);
  void OnNotDecoded(MediaData::Type aType, MediaDecoderReader::NotDecodedReason aReason);
  void OnAudioNotDecoded(MediaDecoderReader::NotDecodedReason aReason)
  {
    MOZ_ASSERT(OnTaskQueue());
    OnNotDecoded(MediaData::AUDIO_DATA, aReason);
  }
  void OnVideoNotDecoded(MediaDecoderReader::NotDecodedReason aReason)
  {
    MOZ_ASSERT(OnTaskQueue());
    OnNotDecoded(MediaData::VIDEO_DATA, aReason);
  }

  
  
  void Reset();

protected:
  virtual ~MediaDecoderStateMachine();

  void AssertCurrentThreadInMonitor() const { mDecoder->GetReentrantMonitor().AssertCurrentThreadIn(); }

  void SetState(State aState);

  
  
  void Push(AudioData* aSample);
  void Push(VideoData* aSample);
  void PushFront(AudioData* aSample);
  void PushFront(VideoData* aSample);

  void OnAudioPopped();
  void OnVideoPopped();

  void VolumeChanged();
  void LogicalPlaybackRateChanged();
  void PreservesPitchChanged();

  MediaQueue<AudioData>& AudioQueue() { return mAudioQueue; }
  MediaQueue<VideoData>& VideoQueue() { return mVideoQueue; }

  nsresult FinishDecodeFirstFrame();

  
  
  bool NeedToDecodeAudio();

  
  
  bool NeedToDecodeVideo();

  
  
  
  
  bool HasLowDecodedData(int64_t aAudioUsecs);

  bool OutOfDecodedAudio();

  bool OutOfDecodedVideo()
  {
    MOZ_ASSERT(OnTaskQueue());
    
    int emptyVideoSize = mState == DECODER_STATE_BUFFERING ? 1 : 0;
    return IsVideoDecoding() && !VideoQueue().IsFinished() && VideoQueue().GetSize() <= emptyVideoSize;
  }


  
  
  bool HasLowUndecodedData();

  
  bool HasLowUndecodedData(int64_t aUsecs);

  
  
  
  
  int64_t AudioDecodedUsecs();

  
  
  bool HasFutureAudio();

  
  bool JustExitedQuickBuffering();

  
  
  void UpdateNextFrameStatus();

  
  
  
  void ResyncAudioClock();

  
  
  int64_t GetAudioClock() const;

  int64_t GetStreamClock() const;

  
  
  
  int64_t GetVideoStreamPosition() const;

  
  
  
  int64_t GetClock() const;

  nsresult DropAudioUpToSeekTarget(AudioData* aSample);
  nsresult DropVideoUpToSeekTarget(VideoData* aSample);

  void SetStartTime(int64_t aStartTimeUsecs);

  
  
  
  
  void UpdatePlaybackPositionInternal(int64_t aTime);

  
  
  void RenderVideoFrame(VideoData* aData, TimeStamp aTarget);

  
  
  
  
  
  void AdvanceFrame();

  
  
  void StopAudioThread();

  
  
  nsresult StartAudioThread();

  
  void PlayStateChanged();

  
  void LogicallySeekingChanged();

  
  
  void StopPlayback();

  
  
  
  void MaybeStartPlayback();

  
  
  void StartDecoding();

  
  
  
  
  void DecodeError();

  
  
  
  nsresult EnqueueDecodeMetadataTask();

  
  
  
  void EnqueueLoadedMetadataEvent();

  void EnqueueFirstFrameLoadedEvent();

  
  
  
  nsresult EnqueueDecodeFirstFrameTask();

  
  
  void InitiateSeek();

  nsresult DispatchAudioDecodeTaskIfNeeded();

  
  
  
  
  
  nsresult EnsureAudioDecodeTaskQueued();

  nsresult DispatchVideoDecodeTaskIfNeeded();

  
  
  
  
  nsresult EnsureVideoDecodeTaskQueued();

  
  
  
  
  void DispatchDecodeTasksIfNeeded();

  
  
  
  
  
  int64_t GetMediaTime() const {
    AssertCurrentThreadInMonitor();
    return mStartTime + mCurrentPosition;
  }

  
  
  
  
  
  
  
  int64_t GetDecodedAudioDuration();

  
  void OnMetadataRead(MetadataHolder* aMetadata);
  void OnMetadataNotRead(ReadMetadataFailureReason aReason);

  
  
  nsresult DecodeFirstFrame();

  
  void CallDecodeFirstFrame();

  
  
  void MaybeFinishDecodeFirstFrame();

  
  
  void DecodeSeek();

  void CheckIfSeekComplete();
  bool IsAudioSeekComplete();
  bool IsVideoSeekComplete();

  
  void SeekCompleted();

  
  
  
  
  void CheckIfDecodeComplete();

  
  
  void SendStreamAudio(AudioData* aAudio, DecodedStreamData* aStream,
                       AudioSegment* aOutput);

  
  
  
  nsresult RunStateMachine();

  bool IsStateMachineScheduled() const;

  
  
  
  bool IsPausedAndDecoderWaiting();

  
  
  bool IsAudioDecoding();
  bool IsVideoDecoding();

  
  
  void SetPlayStartTime(const TimeStamp& aTimeStamp);

public:
  
  void OnAudioEndTimeUpdate(int64_t aAudioEndTime);

private:
  
  void OnPlaybackOffsetUpdate(int64_t aPlaybackOffset);
public:
  void DispatchOnPlaybackOffsetUpdate(int64_t aPlaybackOffset)
  {
    RefPtr<nsRunnable> r =
      NS_NewRunnableMethodWithArg<int64_t>(this, &MediaDecoderStateMachine::OnPlaybackOffsetUpdate, aPlaybackOffset);
    TaskQueue()->Dispatch(r.forget());
  }

private:
  
  
  void OnAudioSinkComplete();
public:
  void DispatchOnAudioSinkComplete()
  {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::OnAudioSinkComplete);
    TaskQueue()->Dispatch(runnable.forget());
  }

  
  void OnAudioSinkError();

  void DispatchOnAudioSinkError()
  {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::OnAudioSinkError);
    TaskQueue()->Dispatch(runnable.forget());
  }

  
  
  bool NeedToSkipToNextKeyframe();

  
  
  
  
  
  
  
  
  
  nsRefPtr<MediaDecoder> mDecoder;

  
  nsRefPtr<MediaTaskQueue> mTaskQueue;

  
  WatchManager<MediaDecoderStateMachine> mWatchManager;

  
  bool mRealTime;

  
  
  bool mDispatchedStateMachine;

  
  class DelayedScheduler {
  public:
    explicit DelayedScheduler(MediaDecoderStateMachine* aSelf)
      : mSelf(aSelf), mMediaTimer(new MediaTimer()) {}

    bool IsScheduled() const { return !mTarget.IsNull(); }

    void Reset()
    {
      MOZ_ASSERT(mSelf->OnTaskQueue(), "Must be on state machine queue to disconnect");
      if (IsScheduled()) {
        mRequest.Disconnect();
        mTarget = TimeStamp();
      }
    }

    void Ensure(mozilla::TimeStamp& aTarget)
    {
      MOZ_ASSERT(mSelf->OnTaskQueue());
      if (IsScheduled() && mTarget <= aTarget) {
        return;
      }
      Reset();
      mTarget = aTarget;
      mRequest.Begin(mMediaTimer->WaitUntil(mTarget, __func__)->Then(
        mSelf->TaskQueue(), __func__, mSelf,
        &MediaDecoderStateMachine::OnDelayedSchedule,
        &MediaDecoderStateMachine::NotReached));
    }

    void CompleteRequest()
    {
      MOZ_ASSERT(mSelf->OnTaskQueue());
      mRequest.Complete();
      mTarget = TimeStamp();
    }

  private:
    MediaDecoderStateMachine* mSelf;
    nsRefPtr<MediaTimer> mMediaTimer;
    MediaPromiseRequestHolder<mozilla::MediaTimerPromise> mRequest;
    TimeStamp mTarget;

  } mDelayedScheduler;

  
  
  
  class StartTimeRendezvous {
  public:
    typedef MediaDecoderReader::AudioDataPromise AudioDataPromise;
    typedef MediaDecoderReader::VideoDataPromise VideoDataPromise;
    typedef MediaPromise<bool, bool,  false> HaveStartTimePromise;

    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(StartTimeRendezvous);
    StartTimeRendezvous(AbstractThread* aOwnerThread, bool aHasAudio, bool aHasVideo,
                        bool aForceZeroStartTime)
      : mOwnerThread(aOwnerThread)
    {
      if (aForceZeroStartTime) {
        mAudioStartTime.emplace(0);
        mVideoStartTime.emplace(0);
        return;
      }

      if (!aHasAudio) {
        mAudioStartTime.emplace(INT64_MAX);
      }

      if (!aHasVideo) {
        mVideoStartTime.emplace(INT64_MAX);
      }
    }

    void Destroy() { mHaveStartTimePromise.RejectIfExists(false, __func__); }

    nsRefPtr<HaveStartTimePromise> AwaitStartTime()
    {
      if (HaveStartTime()) {
        return HaveStartTimePromise::CreateAndResolve(true, __func__);
      }
      return mHaveStartTimePromise.Ensure(__func__);
    }

    template<typename PromiseType>
    struct PromiseSampleType {
      typedef typename PromiseType::ResolveValueType::element_type Type;
    };

    template<typename PromiseType>
    nsRefPtr<PromiseType> ProcessFirstSample(typename PromiseSampleType<PromiseType>::Type* aData)
    {
      typedef typename PromiseSampleType<PromiseType>::Type DataType;
      typedef typename PromiseType::Private PromisePrivate;
      MOZ_ASSERT(mOwnerThread->IsCurrentThreadIn());

      MaybeSetChannelStartTime<DataType>(aData->mTime);

      nsRefPtr<PromisePrivate> p = new PromisePrivate(__func__);
      nsRefPtr<DataType> data = aData;
      nsRefPtr<StartTimeRendezvous> self = this;
      AwaitStartTime()->Then(mOwnerThread, __func__,
                             [p, data, self] () -> void {
                               MOZ_ASSERT(self->mOwnerThread->IsCurrentThreadIn());
                               p->Resolve(data, __func__);
                             },
                             [p] () -> void { p->Reject(MediaDecoderReader::CANCELED, __func__); });

      return p.forget();
    }

    template<typename SampleType>
    void FirstSampleRejected(MediaDecoderReader::NotDecodedReason aReason)
    {
      MOZ_ASSERT(mOwnerThread->IsCurrentThreadIn());
      if (aReason == MediaDecoderReader::DECODE_ERROR) {
        mHaveStartTimePromise.RejectIfExists(false, __func__);
      } else if (aReason == MediaDecoderReader::END_OF_STREAM) {
        MOZ_LOG(gMediaDecoderLog, LogLevel::Debug,
                ("StartTimeRendezvous=%p %s Has no samples.", this, SampleType::sTypeName));
        MaybeSetChannelStartTime<SampleType>(INT64_MAX);
      }
    }

    bool HaveStartTime() { return mAudioStartTime.isSome() && mVideoStartTime.isSome(); }
    int64_t StartTime()
    {
      int64_t time = std::min(mAudioStartTime.ref(), mVideoStartTime.ref());
      return time == INT64_MAX ? 0 : time;
    }
  private:
    virtual ~StartTimeRendezvous() {}

    template<typename SampleType>
    void MaybeSetChannelStartTime(int64_t aStartTime)
    {
      if (ChannelStartTime(SampleType::sType).isSome()) {
        
        
        return;
      }

      MOZ_LOG(gMediaDecoderLog, LogLevel::Debug,
              ("StartTimeRendezvous=%p Setting %s start time to %lld",
               this, SampleType::sTypeName, aStartTime));

      ChannelStartTime(SampleType::sType).emplace(aStartTime);
      if (HaveStartTime()) {
        mHaveStartTimePromise.ResolveIfExists(true, __func__);
      }
    }

    Maybe<int64_t>& ChannelStartTime(MediaData::Type aType)
    {
      return aType == MediaData::AUDIO_DATA ? mAudioStartTime : mVideoStartTime;
    }

    MediaPromiseHolder<HaveStartTimePromise> mHaveStartTimePromise;
    nsRefPtr<AbstractThread> mOwnerThread;
    Maybe<int64_t> mAudioStartTime;
    Maybe<int64_t> mVideoStartTime;
  };
  nsRefPtr<StartTimeRendezvous> mStartTimeRendezvous;

  bool HaveStartTime() { return mStartTimeRendezvous && mStartTimeRendezvous->HaveStartTime(); }
  int64_t StartTime() { return mStartTimeRendezvous->StartTime(); }

  
  
  
  TimeStamp mVideoDecodeStartTime;

  
  
  MediaQueue<AudioData> mAudioQueue;

  
  
  MediaQueue<VideoData> mVideoQueue;

  
  
  
  
  Watchable<State> mState;

  
  
  
  MediaTaskQueue* DecodeTaskQueue() const { return mReader->TaskQueue(); }

  
  
  
  TimeStamp mPlayStartTime;

  
  
  
  
  
  int64_t mPlayDuration;

  
  
  
  TimeStamp mBufferingStart;

  
  
  
  
  int64_t mStartTime;

  
  
  
  
  int64_t mEndTime;

  
  void RecomputeDuration();

  
  
  
  bool mDurationSet;

  
  Mirror<media::NullableTimeUnit> mEstimatedDuration;

  
  Mirror<Maybe<double>> mExplicitDuration;

  
  
  Watchable<media::TimeUnit> mObservedDuration;

  
  Mirror<MediaDecoder::PlayState> mPlayState;
  Mirror<MediaDecoder::PlayState> mNextPlayState;
  Mirror<bool> mLogicallySeeking;

  
  
  
  
  
  bool IsLogicallyPlaying()
  {
    MOZ_ASSERT(OnTaskQueue());
    return mPlayState == MediaDecoder::PLAY_STATE_PLAYING ||
           mNextPlayState == MediaDecoder::PLAY_STATE_PLAYING;
  }

  
  
  Canonical<NextFrameStatus> mNextFrameStatus;
public:
  AbstractCanonical<NextFrameStatus>* CanonicalNextFrameStatus() { return &mNextFrameStatus; }
protected:

  struct SeekJob {
    void Steal(SeekJob& aOther)
    {
      MOZ_DIAGNOSTIC_ASSERT(!Exists());
      mTarget = aOther.mTarget;
      aOther.mTarget.Reset();
      mPromise = Move(aOther.mPromise);
    }

    bool Exists()
    {
      MOZ_ASSERT(mTarget.IsValid() == !mPromise.IsEmpty());
      return mTarget.IsValid();
    }

    void Resolve(bool aAtEnd, const char* aCallSite)
    {
      mTarget.Reset();
      MediaDecoder::SeekResolveValue val(aAtEnd, mTarget.mEventVisibility);
      mPromise.Resolve(val, aCallSite);
    }

    void RejectIfExists(const char* aCallSite)
    {
      mTarget.Reset();
      mPromise.RejectIfExists(true, aCallSite);
    }

    ~SeekJob()
    {
      MOZ_DIAGNOSTIC_ASSERT(!mTarget.IsValid());
      MOZ_DIAGNOSTIC_ASSERT(mPromise.IsEmpty());
    }

    SeekTarget mTarget;
    MediaPromiseHolder<MediaDecoder::SeekPromise> mPromise;
  };

  
  SeekJob mQueuedSeek;

  
  SeekJob mPendingSeek;

  
  SeekJob mCurrentSeek;

  
  int64_t mFragmentEndTime;

  
  RefPtr<AudioSink> mAudioSink;

  
  
  nsRefPtr<MediaDecoderReader> mReader;

  
  
  
  Canonical<int64_t> mCurrentPosition;
public:
  AbstractCanonical<int64_t>* CanonicalCurrentPosition() { return &mCurrentPosition; }
protected:
  
  
  int64_t mStreamStartTime;

  
  
  
  
  int64_t mAudioStartTime;

  
  
  
  int64_t mAudioEndTime;

  
  
  int64_t mDecodedAudioEndTime;

  
  
  int64_t mVideoFrameEndTime;

  
  
  int64_t mDecodedVideoEndTime;

  
  Mirror<double> mVolume;

  
  
  
  
  
  double mPlaybackRate;
  Mirror<double> mLogicalPlaybackRate;

  
  Mirror<bool> mPreservesPitch;

  
  TimeStamp mDecodeStartTime;

  
  
  uint32_t mBufferingWait;
  int64_t  mLowDataThresholdUsecs;

  
  
  
  
  uint32_t GetAmpleVideoFrames() const;

  
  
  
  
  
  
  
  int64_t mLowAudioThresholdUsecs;

  
  
  
  
  
  
  int64_t mAmpleAudioThresholdUsecs;

  
  
  int64_t mQuickBufferingLowDataThresholdUsecs;

  
  
  
  
  
  
  uint32_t AudioPrerollUsecs() const
  {
    MOZ_ASSERT(OnTaskQueue());
    if (IsRealTime()) {
      return 0;
    }

    uint32_t result = mLowAudioThresholdUsecs * 2;
    MOZ_ASSERT(result <= mAmpleAudioThresholdUsecs, "Prerolling will never finish");
    return result;
  }

  uint32_t VideoPrerollFrames() const
  {
    MOZ_ASSERT(OnTaskQueue());
    return IsRealTime() ? 0 : GetAmpleVideoFrames() / 2;
  }

  bool DonePrerollingAudio()
  {
    MOZ_ASSERT(OnTaskQueue());
    AssertCurrentThreadInMonitor();
    return !IsAudioDecoding() || GetDecodedAudioDuration() >= AudioPrerollUsecs() * mPlaybackRate;
  }

  bool DonePrerollingVideo()
  {
    MOZ_ASSERT(OnTaskQueue());
    AssertCurrentThreadInMonitor();
    return !IsVideoDecoding() ||
           static_cast<uint32_t>(VideoQueue().GetSize()) >= VideoPrerollFrames() * mPlaybackRate;
  }

  void StopPrerollingAudio()
  {
    MOZ_ASSERT(OnTaskQueue());
    AssertCurrentThreadInMonitor();
    if (mIsAudioPrerolling) {
      mIsAudioPrerolling = false;
      ScheduleStateMachine();
    }
  }

  void StopPrerollingVideo()
  {
    MOZ_ASSERT(OnTaskQueue());
    AssertCurrentThreadInMonitor();
    if (mIsVideoPrerolling) {
      mIsVideoPrerolling = false;
      ScheduleStateMachine();
    }
  }

  
  
  
  
  nsRefPtr<VideoData> mFirstVideoFrameAfterSeek;

  
  
  
  
  
  
  
  
  bool mIsAudioPrerolling;
  bool mIsVideoPrerolling;

  
  

  MediaPromiseRequestHolder<MediaDecoderReader::AudioDataPromise> mAudioDataRequest;
  MediaPromiseRequestHolder<MediaDecoderReader::WaitForDataPromise> mAudioWaitRequest;
  const char* AudioRequestStatus()
  {
    MOZ_ASSERT(OnTaskQueue());
    if (mAudioDataRequest.Exists()) {
      MOZ_DIAGNOSTIC_ASSERT(!mAudioWaitRequest.Exists());
      return "pending";
    } else if (mAudioWaitRequest.Exists()) {
      return "waiting";
    }
    return "idle";
  }

  MediaPromiseRequestHolder<MediaDecoderReader::WaitForDataPromise> mVideoWaitRequest;
  MediaPromiseRequestHolder<MediaDecoderReader::VideoDataPromise> mVideoDataRequest;
  const char* VideoRequestStatus()
  {
    MOZ_ASSERT(OnTaskQueue());
    if (mVideoDataRequest.Exists()) {
      MOZ_DIAGNOSTIC_ASSERT(!mVideoWaitRequest.Exists());
      return "pending";
    } else if (mVideoWaitRequest.Exists()) {
      return "waiting";
    }
    return "idle";
  }

  MediaPromiseRequestHolder<MediaDecoderReader::WaitForDataPromise>& WaitRequestRef(MediaData::Type aType)
  {
    MOZ_ASSERT(OnTaskQueue());
    return aType == MediaData::AUDIO_DATA ? mAudioWaitRequest : mVideoWaitRequest;
  }

  
  
  
  bool mAudioCaptured;

  
  
  
  
  
  bool mPositionChangeQueued;

  
  
  
  
  
  
  
  Watchable<bool> mAudioCompleted;

  
  
  
  
  
  
  bool mNotifyMetadataBeforeFirstFrame;

  
  
  
  bool mDispatchedEventToDecode;

  
  
  
  
  
  bool mQuickBuffering;

  
  
  
  
  
  
  
  
  
  
  bool mMinimizePreroll;

  
  
  
  bool mDecodeThreadWaiting;

  
  
  
  
  bool mDropAudioUntilNextDiscontinuity;
  bool mDropVideoUntilNextDiscontinuity;

  
  
  bool mDecodeToSeekTarget;

  
  MediaPromiseRequestHolder<MediaDecoderReader::SeekPromise> mSeekRequest;

  
  
  
  int64_t mCurrentTimeBeforeSeek;

  
  MediaPromiseRequestHolder<MediaDecoderReader::MetadataPromise> mMetadataRequest;

  
  
  MediaInfo mInfo;

  nsAutoPtr<MetadataTags> mMetadataTags;

  mozilla::MediaMetadataManager mMetadataManager;

  mozilla::RollingMean<uint32_t, uint32_t> mCorruptFrames;

  bool mDisabledHardwareAcceleration;

  
  
  bool mDecodingFrozenAtStateDecoding;

  
  
  bool mSentLoadedMetadataEvent;
  
  
  
  
  
  bool mSentFirstFrameLoadedEvent;

  bool mSentPlaybackEndedEvent;

  
  
  
  
  
  DecodedStream mDecodedStream;
};

} 
#endif
