










































































#if !defined(MediaDecoderStateMachine_h__)
#define MediaDecoderStateMachine_h__

#include "mozilla/Attributes.h"
#include "nsThreadUtils.h"
#include "MediaDecoder.h"
#include "mozilla/ReentrantMonitor.h"
#include "MediaDecoderReader.h"
#include "MediaDecoderOwner.h"
#include "MediaMetadataManager.h"

class nsITimer;

namespace mozilla {

class AudioSegment;
class VideoSegment;
class MediaTaskQueue;
class SharedThreadPool;




#ifdef GetCurrentTime
#undef GetCurrentTime
#endif













class MediaDecoderStateMachine
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaDecoderStateMachine)
public:
  typedef MediaDecoder::DecodedStreamData DecodedStreamData;
  MediaDecoderStateMachine(MediaDecoder* aDecoder,
                               MediaDecoderReader* aReader,
                               bool aRealTime = false);

  nsresult Init(MediaDecoderStateMachine* aCloneDonor);

  
  enum State {
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
  bool OnAudioThread() const {
    return IsCurrentThread(mAudioThread);
  }

  MediaDecoderOwner::NextFrameStatus GetNextFrameStatus();

  
  
  
  void Play();

  
  void Seek(const SeekTarget& aTarget);

  
  
  
  double GetCurrentTime() const;

  
  
  
  void ClearPositionChangeFlag();

  
  
  
  void SetTransportSeekable(bool aSeekable);

  
  
  
  
  void SetMediaSeekable(bool aSeekable);

  
  
  
  
  
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

  bool IsTransportSeekable() {
    AssertCurrentThreadInMonitor();
    return mTransportSeekable;
  }

  bool IsMediaSeekable() {
    AssertCurrentThreadInMonitor();
    return mMediaSeekable;
  }

  
  nsIEventTarget* GetStateMachineThread();

  
  
  void ScheduleStateMachineWithLockAndWakeDecoder();

  
  
  
  nsresult ScheduleStateMachine(int64_t aUsecs = 0);

  
  nsresult TimeoutExpired(int aGeneration);

  
  void SetFragmentEndTime(int64_t aEndTime);

  
  void ReleaseDecoder() {
    MOZ_ASSERT(mReader);
    if (mReader) {
      mReader->ReleaseDecoder();
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

  void QueueMetadata(int64_t aPublishTime, int aChannels, int aRate, bool aHasAudio, bool aHasVideo, MetadataTags* aTags);

  
  
  bool IsPlaying();

  
  
  
  void NotifyWaitingForResourcesStatusChanged();

  
  
  
  
  
  void SetMinimizePrerollUntilPlaybackStarts();

protected:
  virtual ~MediaDecoderStateMachine();

  void AssertCurrentThreadInMonitor() const { mDecoder->GetReentrantMonitor().AssertCurrentThreadIn(); }

  class WakeDecoderRunnable : public nsRunnable {
  public:
    WakeDecoderRunnable(MediaDecoderStateMachine* aSM)
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

  MediaQueue<AudioData>& AudioQueue() { return mReader->AudioQueue(); }
  MediaQueue<VideoData>& VideoQueue() { return mReader->VideoQueue(); }

  
  
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

  
  
  
  
  
  
  
  
  void Wait(int64_t aUsecs);

  
  void UpdateReadyState();

  
  void ResetPlayback();

  
  
  int64_t GetAudioClock();

  
  
  
  int64_t GetVideoStreamPosition();

  
  
  
  int64_t GetClock();

  
  
  
  
  VideoData* FindStartTime();

  
  
  
  
  void UpdatePlaybackPositionInternal(int64_t aTime);

  
  
  void RenderVideoFrame(VideoData* aData, TimeStamp aTarget);

  
  
  
  
  
  void AdvanceFrame();

  
  
  
  
  
  
  
  uint32_t PlaySilence(uint32_t aFrames,
                       uint32_t aChannels,
                       uint64_t aFrameOffset);

  
  
  uint32_t PlayFromAudioQueue(uint64_t aFrameOffset, uint32_t aChannels);

  
  
  void StopAudioThread();

  
  
  nsresult StartAudioThread();

  
  
  
  void AudioLoop();

  
  
  void StopPlayback();

  
  
  void StartPlayback();

  
  
  void StartDecoding();

  
  
  
  
  void DecodeError();

  void StartWaitForResources();

  
  
  
  nsresult EnqueueDecodeMetadataTask();

  nsresult DispatchAudioDecodeTaskIfNeeded();

  
  
  
  
  
  nsresult EnsureAudioDecodeTaskQueued();

  nsresult DispatchVideoDecodeTaskIfNeeded();

  
  
  
  
  nsresult EnsureVideoDecodeTaskQueued();

  
  
  nsresult EnqueueDecodeSeekTask();

  
  
  void SetReaderIdle();

  
  
  
  
  void DispatchDecodeTasksIfNeeded();

  
  
  
  
  void CheckIfDecodeComplete();

  
  
  
  
  
  int64_t GetMediaTime() const {
    AssertCurrentThreadInMonitor();
    return mStartTime + mCurrentFrameTime;
  }

  
  
  
  
  
  
  
  int64_t GetDecodedAudioDuration();

  
  
  nsresult DecodeMetadata();

  
  
  void DecodeSeek();

  
  
  void DecodeLoop();

  void CallDecodeMetadata();

  
  
  void SendStreamAudio(AudioData* aAudio, DecodedStreamData* aStream,
                       AudioSegment* aOutput);

  
  nsresult CallRunStateMachine();

  
  
  
  nsresult RunStateMachine();

  bool IsStateMachineScheduled() const {
    AssertCurrentThreadInMonitor();
    return !mTimeout.IsNull();
  }

  
  
  
  bool IsPausedAndDecoderWaiting();

  
  
  
  
  
  
  
  
  
  nsRefPtr<MediaDecoder> mDecoder;

  
  
  
  
  State mState;

  
  
  nsCOMPtr<nsIThread> mAudioThread;

  
  
  
  RefPtr<MediaTaskQueue> mDecodeTaskQueue;

  RefPtr<SharedThreadPool> mStateMachineThreadPool;

  
  
  nsCOMPtr<nsITimer> mTimer;

  
  
  TimeStamp mTimeout;

  
  DebugOnly<bool> mInRunningStateMachine;

  
  
  
  TimeStamp mPlayStartTime;

  
  
  
  StreamTime mSyncPointInMediaStream;
  int64_t mSyncPointInDecodedStream; 

  
  
  
  bool mResetPlayStartTime;

  
  
  
  
  int64_t mPlayDuration;

  
  
  
  TimeStamp mBufferingStart;

  
  
  
  
  int64_t mStartTime;

  
  
  
  int64_t mEndTime;

  
  
  
  SeekTarget mSeekTarget;

  
  int64_t mFragmentEndTime;

  
  
  
  
  RefPtr<AudioStream> mAudioStream;

  
  
  
  nsAutoPtr<MediaDecoderReader> mReader;

  
  
  
  
  
  nsRevocableEventPtr<WakeDecoderRunnable> mPendingWakeDecoder;

  
  
  
  
  int64_t mCurrentFrameTime;

  
  
  
  
  int64_t mAudioStartTime;

  
  
  
  int64_t mAudioEndTime;

  
  
  int64_t mVideoFrameEndTime;

  
  
  
  double mVolume;

  
  
  double mPlaybackRate;

  
  bool mPreservesPitch;

  
  
  
  int64_t mBasePosition;

  
  TimeStamp mDecodeStartTime;

  
  
  uint32_t mBufferingWait;
  int64_t  mLowDataThresholdUsecs;

  
  
  
  uint32_t mAmpleVideoFrames;

  
  
  
  
  
  
  
  int64_t mLowAudioThresholdUsecs;

  
  
  
  
  
  
  int64_t mAmpleAudioThresholdUsecs;

  
  
  
  
  
  
  uint32_t mAudioPrerollUsecs;
  uint32_t mVideoPrerollFrames;

  
  
  
  
  
  
  
  
  bool mIsAudioPrerolling;
  bool mIsVideoPrerolling;

  
  
  bool mIsAudioDecoding;

  
  
  bool mIsVideoDecoding;

  
  
  bool mDispatchedAudioDecodeTask;

  
  
  bool mDispatchedVideoDecodeTask;

  
  
  
  
  bool mSkipToNextKeyFrame;

  
  
  
  bool mAudioCaptured;

  
  
  bool mTransportSeekable;

  
  
  bool mMediaSeekable;

  
  
  
  
  
  bool mPositionChangeQueued;

  
  
  
  
  
  
  
  bool mAudioCompleted;

  
  
  bool mGotDurationFromMetaData;

  
  
  
  bool mDispatchedEventToDecode;

  
  
  bool mStopAudioThread;

  
  
  
  
  
  bool mQuickBuffering;

  
  
  
  
  
  
  
  
  
  
  bool mMinimizePreroll;

  
  
  
  bool mDecodeThreadWaiting;

  
  bool mRealTime;

  
  
  
  bool mDispatchedDecodeMetadataTask;

  
  
  
  bool mDispatchedDecodeSeekTask;

  
  
  MediaInfo mInfo;

  mozilla::MediaMetadataManager mMetadataManager;

  MediaDecoderOwner::NextFrameStatus mLastFrameStatus;

  
  int mTimerId;
};

} 
#endif
