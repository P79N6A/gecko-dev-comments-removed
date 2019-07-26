










































































#if !defined(nsBuiltinDecoderStateMachine_h__)
#define nsBuiltinDecoderStateMachine_h__

#include "prmem.h"
#include "nsThreadUtils.h"
#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderReader.h"
#include "nsAudioAvailableEventManager.h"
#include "nsHTMLMediaElement.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsITimer.h"
#include "AudioSegment.h"
#include "VideoSegment.h"













class nsBuiltinDecoderStateMachine : public nsDecoderStateMachine
{
public:
  typedef mozilla::ReentrantMonitor ReentrantMonitor;
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;
  typedef mozilla::VideoFrameContainer VideoFrameContainer;
  typedef nsBuiltinDecoder::OutputMediaStream OutputMediaStream;
  typedef mozilla::SourceMediaStream SourceMediaStream;
  typedef mozilla::AudioSegment AudioSegment;
  typedef mozilla::VideoSegment VideoSegment;

  nsBuiltinDecoderStateMachine(nsBuiltinDecoder* aDecoder, nsBuiltinDecoderReader* aReader, bool aRealTime = false);
  ~nsBuiltinDecoderStateMachine();

  
  virtual nsresult Init(nsDecoderStateMachine* aCloneDonor);
  State GetState()
  { 
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mState; 
  }
  virtual void SetVolume(double aVolume);
  virtual void SetAudioCaptured(bool aCapture);
  virtual void Shutdown();
  virtual int64_t GetDuration();
  virtual void SetDuration(int64_t aDuration);
  void SetEndTime(int64_t aEndTime);
  virtual bool OnDecodeThread() const {
    return IsCurrentThread(mDecodeThread);
  }

  virtual nsHTMLMediaElement::NextFrameStatus GetNextFrameStatus();
  virtual void Play();
  virtual void Seek(double aTime);
  virtual double GetCurrentTime() const;
  virtual void ClearPositionChangeFlag();
  virtual void SetSeekable(bool aSeekable);
  virtual void UpdatePlaybackPosition(int64_t aTime);
  virtual void StartBuffering();

  
  NS_IMETHOD Run();

  
  
  bool HasAudio() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mInfo.mHasAudio;
  }

  
  
  bool HasVideo() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mInfo.mHasVideo;
  }

  
  bool HaveNextFrameData() const;

  
  bool IsBuffering() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

    return mState == nsBuiltinDecoderStateMachine::DECODER_STATE_BUFFERING;
  }

  
  bool IsSeeking() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

    return mState == nsBuiltinDecoderStateMachine::DECODER_STATE_SEEKING;
  }

  
  
  bool OnAudioThread() const {
    return IsCurrentThread(mAudioThread);
  }

  bool OnStateMachineThread() const;
 
  nsresult GetBuffered(nsTimeRanges* aBuffered);

  int64_t VideoQueueMemoryInUse() {
    if (mReader) {
      return mReader->VideoQueueMemoryInUse();
    }
    return 0;
  }

  int64_t AudioQueueMemoryInUse() {
    if (mReader) {
      return mReader->AudioQueueMemoryInUse();
    }
    return 0;
  }

  void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);

  int64_t GetEndMediaTime() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mEndTime;
  }

  bool IsSeekable() {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mSeekable;
  }

  bool IsSeekableInBufferedRanges() {
    if (mReader) {
      return mReader->IsSeekableInBufferedRanges();
    }
    return false;
  }

  
  
  virtual void SetFrameBufferLength(uint32_t aLength);

  
  static nsIThread* GetStateMachineThread();

  
  
  
  
  nsresult ScheduleStateMachine();

  
  
  void ScheduleStateMachineWithLockAndWakeDecoder();

  
  
  
  nsresult ScheduleStateMachine(int64_t aUsecs);

  
  
  
  
  nsresult StartDecodeThread();

  
  void TimeoutExpired();

  
  void SetFragmentEndTime(int64_t aEndTime);

  
  void ReleaseDecoder() { mDecoder = nullptr; }

   
   
   void NotifyAudioAvailableListener();

  
  
  void SendOutputStreamData();
  void FinishOutputStreams();
  bool HaveEnoughDecodedAudio(int64_t aAmpleAudioUSecs);
  bool HaveEnoughDecodedVideo();

protected:

  
  
  bool HasLowDecodedData(int64_t aAudioUsecs) const;

  
  
  bool HasLowUndecodedData() const;

  
  
  int64_t GetUndecodedData() const;

  
  
  
  
  int64_t AudioDecodedUsecs() const;

  
  
  bool HasFutureAudio() const;

  
  bool JustExitedQuickBuffering();

  
  
  
  
  
  
  
  
  void Wait(int64_t aUsecs);

  
  void UpdateReadyState();

  
  void ResetPlayback();

  
  
  int64_t GetAudioClock();

  
  
  
  
  VideoData* FindStartTime();

  
  
  
  
  void UpdatePlaybackPositionInternal(int64_t aTime);

  
  
  void RenderVideoFrame(VideoData* aData, TimeStamp aTarget);
 
  
  
  
  
  
  void AdvanceFrame();

  
  
  
  
  
  
  
  uint32_t PlaySilence(uint32_t aFrames,
                       uint32_t aChannels,
                       uint64_t aFrameOffset);

  
  
  
  uint32_t PlayFromAudioQueue(uint64_t aFrameOffset, uint32_t aChannels);

  
  
  
  void StopDecodeThread();

  
  
  void StopAudioThread();

  
  
  
  
  nsresult ScheduleDecodeThread();

  
  
  nsresult StartAudioThread();

  
  
  
  void AudioLoop();

  
  
  
  void StopPlayback();

  
  
  
  void StartPlayback();

  
  
  void StartDecoding();

  
  
  bool IsPlaying();

  
  
  
  
  
  int64_t GetMediaTime() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mStartTime + mCurrentFrameTime;
  }

  
  
  
  
  
  
  
  int64_t GetDecodedAudioDuration();

  
  
  nsresult DecodeMetadata();

  
  
  void DecodeSeek();

  
  
  void DecodeLoop();

  
  
  void DecodeThreadRun();

  
  
  void SendOutputStreamAudio(AudioData* aAudio, OutputMediaStream* aStream,
                             AudioSegment* aOutput);

  
  nsresult CallRunStateMachine();

  
  
  
  nsresult RunStateMachine();

  bool IsStateMachineScheduled() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return !mTimeout.IsNull() || mRunAgain;
  }

  
  
  
  bool IsPausedAndDecoderWaiting();

  
  
  
  
  
  
  
  
  
  nsRefPtr<nsBuiltinDecoder> mDecoder;

  
  
  
  
  State mState;

  
  
  nsCOMPtr<nsIThread> mAudioThread;

  
  nsCOMPtr<nsIThread> mDecodeThread;

  
  
  nsCOMPtr<nsITimer> mTimer;

  
  
  
  TimeStamp mTimeout;

  
  
  
  TimeStamp mPlayStartTime;

  
  
  
  
  int64_t mPlayDuration;

  
  
  
  TimeStamp mBufferingStart;

  
  
  
  
  int64_t mStartTime;

  
  
  
  int64_t mEndTime;

  
  
  
  int64_t mSeekTime;

  
  int64_t mFragmentEndTime;

  
  
  
  
  nsRefPtr<nsAudioStream> mAudioStream;

  
  
  
  nsAutoPtr<nsBuiltinDecoderReader> mReader;

  
  
  
  
  int64_t mCurrentFrameTime;

  
  
  
  
  int64_t mAudioStartTime;

  
  
  
  int64_t mAudioEndTime;

  
  
  int64_t mVideoFrameEndTime;
  
  
  
  
  double mVolume;

  
  TimeStamp mDecodeStartTime;

  
  
  uint32_t mBufferingWait;
  int64_t  mLowDataThresholdUsecs;

  
  
  bool mAudioCaptured;

  
  
  bool mSeekable;

  
  
  
  
  
  bool mPositionChangeQueued;

  
  
  
  
  
  bool mAudioCompleted;

  
  
  bool mGotDurationFromMetaData;
    
  
  
  bool mStopDecodeThread;

  
  
  
  
  
  bool mDecodeThreadIdle;

  
  
  bool mStopAudioThread;

  
  
  
  
  
  bool mQuickBuffering;

  
  
  bool mIsRunning;

  
  
  bool mRunAgain;

  
  
  
  
  
  bool mDispatchedRunEvent;

  
  
  
  bool mDecodeThreadWaiting;

  
  bool mRealTime;

  
  
  
  bool mDidThrottleAudioDecoding;
  bool mDidThrottleVideoDecoding;

  
  
  bool mRequestedNewDecodeThread;
  
private:
  
  
  
  nsAudioAvailableEventManager mEventManager;

  
  
  nsVideoInfo mInfo;
};

#endif
