
















































































#if !defined(MediaDecoderStateMachine_h__)
#define MediaDecoderStateMachine_h__

#include "mozilla/Attributes.h"
#include "nsThreadUtils.h"
#include "MediaDecoder.h"
#include "mozilla/ReentrantMonitor.h"
#include "MediaDecoderReader.h"
#include "MediaDecoderOwner.h"
#include "MediaMetadataManager.h"
#include "MediaDecoderStateMachineScheduler.h"

class nsITimer;

namespace mozilla {

class AudioSegment;
class VideoSegment;
class MediaTaskQueue;
class SharedThreadPool;
class AudioSink;
class MediaDecoderStateMachineScheduler;




#ifdef GetCurrentTime
#undef GetCurrentTime
#endif













class MediaDecoderStateMachine
{
  friend class AudioSink;
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaDecoderStateMachine)
public:
  typedef MediaDecoder::DecodedStreamData DecodedStreamData;
  MediaDecoderStateMachine(MediaDecoder* aDecoder,
                               MediaDecoderReader* aReader,
                               bool aRealTime = false);

  nsresult Init(MediaDecoderStateMachine* aCloneDonor);

  
  enum State {
    DECODER_STATE_DECODING_NONE,
    DECODER_STATE_DECODING_METADATA,
    DECODER_STATE_WAIT_FOR_RESOURCES,
    DECODER_STATE_DECODING_FIRSTFRAME,
    DECODER_STATE_DORMANT,
    DECODER_STATE_DECODING,
    DECODER_STATE_SEEKING,
    DECODER_STATE_BUFFERING,
    DECODER_STATE_COMPLETED,
    DECODER_STATE_SHUTDOWN
  };

  State GetState() {
    AssertCurrentThreadInMonitor();
    return mState;
  }

  
  
  void SetVolume(double aVolume);
  void SetAudioCaptured();

  
  bool IsDormantNeeded();
  
  void SetDormant(bool aDormant);
  void Shutdown();
  void ShutdownReader();
  void FinishShutdown();

  bool IsRealTime() const;

  
  
  int64_t GetDuration();

  
  
  
  
  int64_t GetEndTime();

  
  
  
  
  
  
  void SetDuration(int64_t aDuration);

  
  
  
  void SetMediaEndTime(int64_t aEndTime);

  
  
  
  
  
  void UpdateEstimatedDuration(int64_t aDuration);

  
  
  bool OnDecodeThread() const;
  bool OnStateMachineThread() const;

  MediaDecoderOwner::NextFrameStatus GetNextFrameStatus();

  
  
  
  void Play()
  {
    MOZ_ASSERT(NS_IsMainThread());
    nsRefPtr<nsRunnable> r = NS_NewRunnableMethod(this, &MediaDecoderStateMachine::PlayInternal);
    GetStateMachineThread()->Dispatch(r, NS_DISPATCH_NORMAL);
  }

private:
  
  
  void PlayInternal();
public:

  
  
  void Seek(const SeekTarget& aTarget);

  
  
  void EnqueueStartQueuedSeekTask();

  
  
  void StartQueuedSeek();

  
  
  
  void StartSeek(const SeekTarget& aTarget);

  
  
  
  double GetCurrentTime() const;
  int64_t GetCurrentTimeUs() const;

  
  
  
  void ClearPositionChangeFlag();

  
  
  
  
  
  void UpdatePlaybackPosition(int64_t aTime);

  
  
  
  
  void StartBuffering();

  
  
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
    AssertCurrentThreadInMonitor();

    return mState == DECODER_STATE_BUFFERING;
  }

  
  bool IsSeeking() const {
    AssertCurrentThreadInMonitor();

    return mState == DECODER_STATE_SEEKING;
  }

  nsresult GetBuffered(dom::TimeRanges* aBuffered) {
    
    
    
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    if (mStartTime < 0) {
      return NS_OK;
    }

    return mReader->GetBuffered(aBuffered);
  }

  void SetPlaybackRate(double aPlaybackRate);
  void SetPreservesPitch(bool aPreservesPitch);

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

  
  nsIEventTarget* GetStateMachineThread() const;

  
  
  void ScheduleStateMachineWithLockAndWakeDecoder();

  
  
  
  nsresult ScheduleStateMachine(int64_t aUsecs = 0);

  
  
  static nsresult TimeoutExpired(void* aClosure);

  
  void SetFragmentEndTime(int64_t aEndTime);

  
  void BreakCycles() {
    if (mReader) {
      mReader->BreakCycles();
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

  
  
  
  
  
  void SetMinimizePrerollUntilPlaybackStarts();

  void OnAudioDecoded(AudioData* aSample);
  void OnVideoDecoded(VideoData* aSample);
  void OnNotDecoded(MediaData::Type aType, MediaDecoderReader::NotDecodedReason aReason);
  void OnAudioNotDecoded(MediaDecoderReader::NotDecodedReason aReason)
  {
    OnNotDecoded(MediaData::AUDIO_DATA, aReason);
  }
  void OnVideoNotDecoded(MediaDecoderReader::NotDecodedReason aReason)
  {
    OnNotDecoded(MediaData::VIDEO_DATA, aReason);
  }

  void OnSeekCompleted(int64_t aTime);
  void OnSeekFailed(nsresult aResult);

  void OnWaitForDataResolved(MediaData::Type aType)
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    WaitRequestRef(aType).Complete();
    DispatchDecodeTasksIfNeeded();
  }

  void OnWaitForDataRejected(WaitForDataRejectValue aRejection)
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    WaitRequestRef(aRejection.mType).Complete();
  }

  
  void ResetDecode();

private:
  void AcquireMonitorAndInvokeDecodeError();

protected:
  virtual ~MediaDecoderStateMachine();

  void AssertCurrentThreadInMonitor() const { mDecoder->GetReentrantMonitor().AssertCurrentThreadIn(); }

  void SetState(State aState);

  
  
  void Push(AudioData* aSample);
  void Push(VideoData* aSample);

  class WakeDecoderRunnable : public nsRunnable {
  public:
    explicit WakeDecoderRunnable(MediaDecoderStateMachine* aSM)
      : mMutex("WakeDecoderRunnable"), mStateMachine(aSM) {}
    NS_IMETHOD Run() MOZ_OVERRIDE
    {
      nsRefPtr<MediaDecoderStateMachine> stateMachine;
      {
        
        
        MutexAutoLock lock(mMutex);
        if (!mStateMachine)
          return NS_OK;
        stateMachine = mStateMachine;
      }
      stateMachine->ScheduleStateMachineWithLockAndWakeDecoder();
      return NS_OK;
    }
    void Revoke()
    {
      MutexAutoLock lock(mMutex);
      mStateMachine = nullptr;
    }

    Mutex mMutex;
    
    
    
    
    
    
    MediaDecoderStateMachine* mStateMachine;
  };
  WakeDecoderRunnable* GetWakeDecoderRunnable();

  MediaQueue<AudioData>& AudioQueue() { return mAudioQueue; }
  MediaQueue<VideoData>& VideoQueue() { return mVideoQueue; }

  nsresult FinishDecodeFirstFrame();

  nsAutoPtr<MetadataTags> mMetadataTags;

  
  
  bool NeedToDecodeAudio();

  
  
  bool NeedToDecodeVideo();

  
  
  
  
  bool HasLowDecodedData(int64_t aAudioUsecs);

  bool OutOfDecodedAudio();

  bool OutOfDecodedVideo()
  {
    
    int emptyVideoSize = mState == DECODER_STATE_BUFFERING ? 1 : 0;
    return IsVideoDecoding() && !VideoQueue().IsFinished() && VideoQueue().GetSize() <= emptyVideoSize;
  }


  
  
  bool HasLowUndecodedData();

  
  bool HasLowUndecodedData(int64_t aUsecs);

  
  
  
  
  int64_t AudioDecodedUsecs();

  
  
  bool HasFutureAudio();

  
  bool JustExitedQuickBuffering();

  
  void UpdateReadyState();

  
  void ResetPlayback();

  
  
  
  void FlushDecoding();

  
  
  
  void ResyncAudioClock();

  
  
  int64_t GetAudioClock() const;

  
  
  
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

  
  
  void StopPlayback();

  
  
  
  void MaybeStartPlayback();

  
  
  void StartDecoding();

  
  
  
  
  void DecodeError();

  void StartWaitForResources();

  
  
  
  nsresult EnqueueDecodeMetadataTask();

  
  
  
  void EnqueueLoadedMetadataEvent();

  void EnqueueFirstFrameLoadedEvent();

  
  
  
  nsresult EnqueueDecodeFirstFrameTask();

  
  
  nsresult EnqueueDecodeSeekTask();

  nsresult DispatchAudioDecodeTaskIfNeeded();

  
  
  
  
  
  nsresult EnsureAudioDecodeTaskQueued();

  nsresult DispatchVideoDecodeTaskIfNeeded();

  
  
  
  
  nsresult EnsureVideoDecodeTaskQueued();

  
  
  void SetReaderIdle();

  
  
  
  
  void DispatchDecodeTasksIfNeeded();
  void AcquireMonitorAndInvokeDispatchDecodeTasksIfNeeded();

  
  
  
  
  
  int64_t GetMediaTime() const {
    AssertCurrentThreadInMonitor();
    return mStartTime + mCurrentFrameTime;
  }

  
  
  
  
  
  
  
  int64_t GetDecodedAudioDuration();

  
  
  nsresult DecodeMetadata();

  
  void CallDecodeMetadata();

  
  
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

  
  nsresult CallRunStateMachine();

  
  
  
  nsresult RunStateMachine();

  bool IsStateMachineScheduled() const;

  
  
  
  bool IsPausedAndDecoderWaiting();

  
  
  bool IsAudioDecoding();
  bool IsVideoDecoding();

  
  
  void SetPlayStartTime(const TimeStamp& aTimeStamp);

  
  void OnAudioEndTimeUpdate(int64_t aAudioEndTime);

  
  void OnPlaybackOffsetUpdate(int64_t aPlaybackOffset);

  
  
  void OnAudioSinkComplete();

  
  void OnAudioSinkError();

  
  
  void DoNotifyWaitingForResourcesStatusChanged();

  
  
  bool NeedToSkipToNextKeyframe();

  
  
  
  
  
  
  
  
  
  nsRefPtr<MediaDecoder> mDecoder;

  
  
  const nsRefPtr<MediaDecoderStateMachineScheduler> mScheduler;

  
  
  
  TimeStamp mVideoDecodeStartTime;

  
  
  MediaQueue<AudioData> mAudioQueue;

  
  
  MediaQueue<VideoData> mVideoQueue;

  
  
  
  
  State mState;

  
  
  
  MediaTaskQueue* DecodeTaskQueue() const { return mReader->GetTaskQueue(); }

  
  
  
  TimeStamp mPlayStartTime;

  
  
  
  
  
  int64_t mPlayDuration;

  
  
  
  TimeStamp mBufferingStart;

  
  
  
  
  int64_t mStartTime;

  
  
  
  
  int64_t mEndTime;

  
  
  
  bool mDurationSet;

  
  
  
  SeekTarget mSeekTarget;

  
  
  
  SeekTarget mQueuedSeekTarget;

  
  
  
  
  
  
  SeekTarget mCurrentSeekTarget;

  
  int64_t mFragmentEndTime;

  
  RefPtr<AudioSink> mAudioSink;

  
  
  nsRefPtr<MediaDecoderReader> mReader;

  
  
  
  
  
  nsRevocableEventPtr<WakeDecoderRunnable> mPendingWakeDecoder;

  
  
  
  
  int64_t mCurrentFrameTime;

  
  
  
  
  int64_t mAudioStartTime;

  
  
  
  int64_t mAudioEndTime;

  
  
  int64_t mDecodedAudioEndTime;

  
  
  int64_t mVideoFrameEndTime;

  
  
  int64_t mDecodedVideoEndTime;

  
  
  
  double mVolume;

  
  
  double mPlaybackRate;

  
  bool mPreservesPitch;

  
  TimeStamp mDecodeStartTime;

  
  
  uint32_t mBufferingWait;
  int64_t  mLowDataThresholdUsecs;

  
  
  
  
  uint32_t GetAmpleVideoFrames() const;

  
  
  
  
  
  
  
  int64_t mLowAudioThresholdUsecs;

  
  
  
  
  
  
  int64_t mAmpleAudioThresholdUsecs;

  
  
  int64_t mQuickBufferingLowDataThresholdUsecs;

  
  
  
  
  
  
  uint32_t AudioPrerollUsecs() const
  {
    if (mScheduler->IsRealTime()) {
      return 0;
    }

    uint32_t result = mLowAudioThresholdUsecs * 2;
    MOZ_ASSERT(result <= mAmpleAudioThresholdUsecs, "Prerolling will never finish");
    return result;
  }

  uint32_t VideoPrerollFrames() const
  {
    return mScheduler->IsRealTime() ? 0 : GetAmpleVideoFrames() / 2;
  }

  bool DonePrerollingAudio()
  {
    AssertCurrentThreadInMonitor();
    return !IsAudioDecoding() || GetDecodedAudioDuration() >= AudioPrerollUsecs() * mPlaybackRate;
  }

  bool DonePrerollingVideo()
  {
    AssertCurrentThreadInMonitor();
    return !IsVideoDecoding() ||
           static_cast<uint32_t>(VideoQueue().GetSize()) >= VideoPrerollFrames() * mPlaybackRate;
  }

  void StopPrerollingAudio()
  {
    AssertCurrentThreadInMonitor();
    if (mIsAudioPrerolling) {
      mIsAudioPrerolling = false;
      ScheduleStateMachine();
    }
  }

  void StopPrerollingVideo()
  {
    AssertCurrentThreadInMonitor();
    if (mIsVideoPrerolling) {
      mIsVideoPrerolling = false;
      ScheduleStateMachine();
    }
  }

  
  
  
  
  nsRefPtr<VideoData> mFirstVideoFrameAfterSeek;

  
  
  
  
  
  
  
  
  bool mIsAudioPrerolling;
  bool mIsVideoPrerolling;

  
  

  MediaPromiseConsumerHolder<MediaDecoderReader::AudioDataPromise> mAudioDataRequest;
  MediaPromiseConsumerHolder<MediaDecoderReader::WaitForDataPromise> mAudioWaitRequest;
  const char* AudioRequestStatus()
  {
    if (mAudioDataRequest.Exists()) {
      MOZ_DIAGNOSTIC_ASSERT(!mAudioWaitRequest.Exists());
      return "pending";
    } else if (mAudioWaitRequest.Exists()) {
      return "waiting";
    }
    return "idle";
  }

  MediaPromiseConsumerHolder<MediaDecoderReader::WaitForDataPromise> mVideoWaitRequest;
  MediaPromiseConsumerHolder<MediaDecoderReader::VideoDataPromise> mVideoDataRequest;
  const char* VideoRequestStatus()
  {
    if (mVideoDataRequest.Exists()) {
      MOZ_DIAGNOSTIC_ASSERT(!mVideoWaitRequest.Exists());
      return "pending";
    } else if (mVideoWaitRequest.Exists()) {
      return "waiting";
    }
    return "idle";
  }

  MediaPromiseConsumerHolder<MediaDecoderReader::WaitForDataPromise>& WaitRequestRef(MediaData::Type aType)
  {
    return aType == MediaData::AUDIO_DATA ? mAudioWaitRequest : mVideoWaitRequest;
  }

  
  
  
  bool mAudioCaptured;

  
  
  
  
  
  bool mPositionChangeQueued;

  
  
  
  
  
  
  
  bool mAudioCompleted;

  
  
  bool mGotDurationFromMetaData;

  
  
  
  bool mDispatchedEventToDecode;

  
  
  bool mStopAudioThread;

  
  
  
  
  
  bool mQuickBuffering;

  
  
  
  
  
  
  
  
  
  
  bool mMinimizePreroll;

  
  
  
  bool mDecodeThreadWaiting;

  
  
  
  
  bool mDropAudioUntilNextDiscontinuity;
  bool mDropVideoUntilNextDiscontinuity;

  
  
  bool mDecodeToSeekTarget;

  
  
  
  bool mWaitingForDecoderSeek;

  
  
  bool mCancelingSeek;

  
  
  
  int64_t mCurrentTimeBeforeSeek;

  
  
  MediaInfo mInfo;

  mozilla::MediaMetadataManager mMetadataManager;

  MediaDecoderOwner::NextFrameStatus mLastFrameStatus;

  
  
  bool mDecodingFrozenAtStateDecoding;

  
  
  bool mSentLoadedMetadataEvent;
  
  
  
  
  
  bool mSentFirstFrameLoadedEvent;
};

} 
#endif
