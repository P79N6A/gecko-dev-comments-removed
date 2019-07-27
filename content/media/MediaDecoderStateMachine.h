
















































































#if !defined(MediaDecoderStateMachine_h__)
#define MediaDecoderStateMachine_h__

#include "mozilla/Attributes.h"
#include "nsThreadUtils.h"
#include "MediaDecoder.h"
#include "mozilla/ReentrantMonitor.h"
#include "MediaDecoderReader.h"
#include "MediaDecoderOwner.h"
#include "MediaMetadataManager.h"
#include "MediaDataDecodedListener.h"

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
  void SetAudioCaptured(bool aCapture);

  
  bool IsDormantNeeded();
  
  void SetDormant(bool aDormant);
  void Shutdown();

  
  
  int64_t GetDuration();

  
  
  
  
  
  void SetDuration(int64_t aDuration);

  
  
  
  void SetMediaEndTime(int64_t aEndTime);

  
  
  
  
  
  void UpdateEstimatedDuration(int64_t aDuration);

  
  
  bool OnDecodeThread() const;
  bool OnStateMachineThread() const;

  MediaDecoderOwner::NextFrameStatus GetNextFrameStatus();

  
  
  
  void Play();

  
  void Seek(const SeekTarget& aTarget);

  
  
  
  double GetCurrentTime() const;

  
  
  
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

  nsresult GetBuffered(dom::TimeRanges* aBuffered);

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

  int64_t GetEndMediaTime() const {
    AssertCurrentThreadInMonitor();
    return mEndTime;
  }

  
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

  
  
  
  
  
  void SetSyncPointForMediaStream();
  int64_t GetCurrentTimeViaMediaStreamSync();

  
  
  void SendStreamData();
  void FinishStreamData();
  bool HaveEnoughDecodedAudio(int64_t aAmpleAudioUSecs);
  bool HaveEnoughDecodedVideo();

  
  
  bool IsShutdown();

  void QueueMetadata(int64_t aPublishTime, MediaInfo* aInfo, MetadataTags* aTags);

  
  
  bool IsPlaying();

  
  
  
  void NotifyWaitingForResourcesStatusChanged();

  
  
  
  
  
  void SetMinimizePrerollUntilPlaybackStarts();

  void OnAudioDecoded(AudioData* aSample);
  void OnAudioEOS();
  void OnVideoDecoded(VideoData* aSample);
  void OnVideoEOS();
  void OnDecodeError();

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

  nsresult FinishDecodeMetadata();

  RefPtr<MediaDataDecodedListener<MediaDecoderStateMachine>> mMediaDecodedListener;

  nsAutoPtr<MetadataTags> mMetadataTags;

  
  
  bool NeedToDecodeAudio();

  
  void DecodeAudio();

  
  
  bool NeedToDecodeVideo();

  
  void DecodeVideo();

  
  
  bool HasLowDecodedData(int64_t aAudioUsecs);

  
  
  bool HasLowUndecodedData();

  
  bool HasLowUndecodedData(double aUsecs);

  
  
  
  
  int64_t AudioDecodedUsecs();

  
  
  bool HasFutureAudio();

  
  bool JustExitedQuickBuffering();

  
  void UpdateReadyState();

  
  void ResetPlayback();

  
  
  
  void FlushDecoding();

  
  
  int64_t GetAudioClock();

  
  
  
  int64_t GetVideoStreamPosition();

  
  
  
  int64_t GetClock();

  nsresult DropAudioUpToSeekTarget(AudioData* aSample);
  nsresult DropVideoUpToSeekTarget(VideoData* aSample);

  void SetStartTime(int64_t aStartTimeUsecs);

  
  
  
  
  void UpdatePlaybackPositionInternal(int64_t aTime);

  
  
  void RenderVideoFrame(VideoData* aData, TimeStamp aTarget);

  
  
  
  
  
  void AdvanceFrame();

  
  
  void StopAudioThread();

  
  
  nsresult StartAudioThread();

  
  
  void StopPlayback();

  
  
  void StartPlayback();

  
  
  void StartDecoding();

  
  
  
  
  void DecodeError();

  void StartWaitForResources();

  
  
  
  nsresult EnqueueDecodeMetadataTask();

  
  
  nsresult EnqueueDecodeSeekTask();

  nsresult DispatchAudioDecodeTaskIfNeeded();

  
  
  
  
  
  nsresult EnsureAudioDecodeTaskQueued();

  nsresult DispatchVideoDecodeTaskIfNeeded();

  
  
  
  
  nsresult EnsureVideoDecodeTaskQueued();

  
  
  void SetReaderIdle();

  
  
  
  
  void DispatchDecodeTasksIfNeeded();

  
  
  
  
  
  int64_t GetMediaTime() const {
    AssertCurrentThreadInMonitor();
    return mStartTime + mCurrentFrameTime;
  }

  
  
  
  
  
  
  
  int64_t GetDecodedAudioDuration();

  
  
  nsresult DecodeMetadata();

  
  void CallDecodeMetadata();

  
  
  void MaybeFinishDecodeMetadata();

  
  
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

  
  
  
  
  
  
  
  
  
  nsRefPtr<MediaDecoder> mDecoder;

  
  
  const nsAutoPtr<MediaDecoderStateMachineScheduler> mScheduler;

  
  
  
  TimeStamp mVideoDecodeStartTime;

  
  
  MediaQueue<AudioData> mAudioQueue;

  
  
  MediaQueue<VideoData> mVideoQueue;

  
  
  
  
  State mState;

  
  
  
  RefPtr<MediaTaskQueue> mDecodeTaskQueue;

  
  
  
  TimeStamp mPlayStartTime;

  
  
  
  int64_t mSyncPointInMediaStream; 
  int64_t mSyncPointInDecodedStream; 

  
  
  
  
  int64_t mPlayDuration;

  
  
  
  TimeStamp mBufferingStart;

  
  
  
  
  int64_t mStartTime;

  
  
  
  int64_t mEndTime;

  
  
  
  SeekTarget mSeekTarget;

  
  
  
  
  
  
  SeekTarget mCurrentSeekTarget;

  
  int64_t mFragmentEndTime;

  
  RefPtr<AudioSink> mAudioSink;

  
  
  nsRefPtr<MediaDecoderReader> mReader;

  
  
  
  
  
  nsRevocableEventPtr<WakeDecoderRunnable> mPendingWakeDecoder;

  
  
  
  
  int64_t mCurrentFrameTime;

  
  
  
  
  int64_t mAudioStartTime;

  
  
  
  int64_t mAudioEndTime;

  
  
  int64_t mVideoFrameEndTime;

  
  
  
  double mVolume;

  
  
  double mPlaybackRate;

  
  bool mPreservesPitch;

  
  TimeStamp mDecodeStartTime;

  
  
  uint32_t mBufferingWait;
  int64_t  mLowDataThresholdUsecs;

  
  
  
  uint32_t mAmpleVideoFrames;

  
  
  
  
  
  
  
  int64_t mLowAudioThresholdUsecs;

  
  
  
  
  
  
  int64_t mAmpleAudioThresholdUsecs;

  
  
  
  
  
  
  uint32_t mAudioPrerollUsecs;
  uint32_t mVideoPrerollFrames;

  
  
  
  
  nsAutoPtr<VideoData> mFirstVideoFrameAfterSeek;

  
  
  
  
  
  
  
  
  bool mIsAudioPrerolling;
  bool mIsVideoPrerolling;

  
  
  
  bool mAudioRequestPending;
  bool mVideoRequestPending;

  
  
  
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

  
  
  
  int64_t mCurrentTimeBeforeSeek;

  
  
  MediaInfo mInfo;

  mozilla::MediaMetadataManager mMetadataManager;

  MediaDecoderOwner::NextFrameStatus mLastFrameStatus;
};

} 
#endif
