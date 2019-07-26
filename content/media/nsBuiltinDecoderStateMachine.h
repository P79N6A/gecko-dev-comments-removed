










































































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
  virtual PRInt64 GetDuration();
  virtual void SetDuration(PRInt64 aDuration);
  void SetEndTime(PRInt64 aEndTime);
  virtual bool OnDecodeThread() const {
    return IsCurrentThread(mDecodeThread);
  }

  virtual nsHTMLMediaElement::NextFrameStatus GetNextFrameStatus();
  virtual void Play();
  virtual void Seek(double aTime);
  virtual double GetCurrentTime() const;
  virtual void ClearPositionChangeFlag();
  virtual void SetSeekable(bool aSeekable);
  virtual void UpdatePlaybackPosition(PRInt64 aTime);
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

  PRInt64 VideoQueueMemoryInUse() {
    if (mReader) {
      return mReader->VideoQueueMemoryInUse();
    }
    return 0;
  }

  PRInt64 AudioQueueMemoryInUse() {
    if (mReader) {
      return mReader->AudioQueueMemoryInUse();
    }
    return 0;
  }

  void NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRInt64 aOffset);

  PRInt64 GetEndMediaTime() const {
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

  
  
  virtual void SetFrameBufferLength(PRUint32 aLength);

  
  static nsIThread* GetStateMachineThread();

  
  
  
  
  nsresult ScheduleStateMachine();

  
  
  void ScheduleStateMachineWithLockAndWakeDecoder();

  
  
  
  nsresult ScheduleStateMachine(PRInt64 aUsecs);

  
  
  
  
  nsresult StartDecodeThread();

  
  void TimeoutExpired();

  
  void SetFragmentEndTime(PRInt64 aEndTime);

  
  void ReleaseDecoder() { mDecoder = nsnull; }

   
   
   void NotifyAudioAvailableListener();

  
  
  void SendOutputStreamData();
  void FinishOutputStreams();
  bool HaveEnoughDecodedAudio(PRInt64 aAmpleAudioUSecs);
  bool HaveEnoughDecodedVideo();

protected:

  
  
  bool HasLowDecodedData(PRInt64 aAudioUsecs) const;

  
  
  bool HasLowUndecodedData() const;

  
  
  PRInt64 GetUndecodedData() const;

  
  
  
  
  PRInt64 AudioDecodedUsecs() const;

  
  
  bool HasFutureAudio() const;

  
  bool JustExitedQuickBuffering();

  
  
  
  
  
  
  
  
  void Wait(PRInt64 aUsecs);

  
  void UpdateReadyState();

  
  void ResetPlayback();

  
  
  PRInt64 GetAudioClock();

  
  
  
  
  VideoData* FindStartTime();

  
  
  
  
  void UpdatePlaybackPositionInternal(PRInt64 aTime);

  
  
  void RenderVideoFrame(VideoData* aData, TimeStamp aTarget);
 
  
  
  
  
  
  void AdvanceFrame();

  
  
  
  
  
  
  
  PRUint32 PlaySilence(PRUint32 aFrames,
                       PRUint32 aChannels,
                       PRUint64 aFrameOffset);

  
  
  
  PRUint32 PlayFromAudioQueue(PRUint64 aFrameOffset, PRUint32 aChannels);

  
  
  
  void StopDecodeThread();

  
  
  void StopAudioThread();

  
  
  
  
  nsresult ScheduleDecodeThread();

  
  
  nsresult StartAudioThread();

  
  
  
  void AudioLoop();

  
  
  
  void StopPlayback();

  
  
  
  void StartPlayback();

  
  
  void StartDecoding();

  
  
  bool IsPlaying();

  
  
  
  
  
  PRInt64 GetMediaTime() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mStartTime + mCurrentFrameTime;
  }

  
  
  
  
  
  
  
  PRInt64 GetDecodedAudioDuration();

  
  
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

  
  
  PRUint32 mCbCrSize;

  
  nsAutoArrayPtr<unsigned char> mCbCrBuffer;

  
  
  nsCOMPtr<nsIThread> mAudioThread;

  
  nsCOMPtr<nsIThread> mDecodeThread;

  
  
  nsCOMPtr<nsITimer> mTimer;

  
  
  
  TimeStamp mTimeout;

  
  
  
  TimeStamp mPlayStartTime;

  
  
  
  
  PRInt64 mPlayDuration;

  
  
  
  TimeStamp mBufferingStart;

  
  
  
  
  PRInt64 mStartTime;

  
  
  
  PRInt64 mEndTime;

  
  
  
  PRInt64 mSeekTime;

  
  PRInt64 mFragmentEndTime;

  
  
  
  
  nsRefPtr<nsAudioStream> mAudioStream;

  
  
  
  nsAutoPtr<nsBuiltinDecoderReader> mReader;

  
  
  
  
  PRInt64 mCurrentFrameTime;

  
  
  
  
  PRInt64 mAudioStartTime;

  
  
  
  PRInt64 mAudioEndTime;

  
  
  PRInt64 mVideoFrameEndTime;
  
  
  
  
  double mVolume;

  
  TimeStamp mDecodeStartTime;

  
  
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
  
  PRUint32 mBufferingWait;
  PRInt64  mLowDataThresholdUsecs;

private:
  
  
  
  nsAudioAvailableEventManager mEventManager;

  
  
  nsVideoInfo mInfo;
};

#endif
