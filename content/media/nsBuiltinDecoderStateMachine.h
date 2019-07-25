











































































































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













class nsBuiltinDecoderStateMachine : public nsDecoderStateMachine
{
public:
  typedef mozilla::ReentrantMonitor ReentrantMonitor;
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  nsBuiltinDecoderStateMachine(nsBuiltinDecoder* aDecoder, nsBuiltinDecoderReader* aReader);
  ~nsBuiltinDecoderStateMachine();

  
  virtual nsresult Init(nsDecoderStateMachine* aCloneDonor);
  State GetState()
  { 
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mState; 
  }
  virtual void SetVolume(double aVolume);
  virtual void Shutdown();
  virtual PRInt64 GetDuration();
  virtual void SetDuration(PRInt64 aDuration);
  void SetEndTime(PRInt64 aEndTime);
  virtual PRBool OnDecodeThread() const {
    return IsCurrentThread(mDecodeThread);
  }

  virtual nsHTMLMediaElement::NextFrameStatus GetNextFrameStatus();
  virtual void Play();
  virtual void Seek(double aTime);
  virtual double GetCurrentTime() const;
  virtual void ClearPositionChangeFlag();
  virtual void SetSeekable(PRBool aSeekable);
  virtual void UpdatePlaybackPosition(PRInt64 aTime);
  virtual void StartBuffering();

  
  NS_IMETHOD Run();

  
  
  PRBool HasAudio() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mInfo.mHasAudio;
  }

  
  
  PRBool HasVideo() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mInfo.mHasVideo;
  }

  
  PRBool HaveNextFrameData() const;

  
  PRBool IsBuffering() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

    return mState == nsBuiltinDecoderStateMachine::DECODER_STATE_BUFFERING;
  }

  
  PRBool IsSeeking() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

    return mState == nsBuiltinDecoderStateMachine::DECODER_STATE_SEEKING;
  }

  
  
  PRBool OnAudioThread() const {
    return IsCurrentThread(mAudioThread);
  }

  PRBool OnStateMachineThread() const {
    return IsCurrentThread(GetStateMachineThread());
  }
 
  
  
  
  
  
  
  
  
  
  nsRefPtr<nsBuiltinDecoder> mDecoder;

  
  
  
  
  State mState;

  nsresult GetBuffered(nsTimeRanges* aBuffered);

  void NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRUint32 aOffset) {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    mReader->NotifyDataArrived(aBuffer, aLength, aOffset);
  }

  PRInt64 GetEndMediaTime() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mEndTime;
  }

  PRBool GetSeekable() {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mSeekable;
  }

  
  
  virtual void SetFrameBufferLength(PRUint32 aLength);

  
  static nsIThread* GetStateMachineThread();

  
  
  
  
  nsresult ScheduleStateMachine();

  
  
  
  nsresult ScheduleStateMachine(PRInt64 aUsecs);

  
  void TimeoutExpired();

protected:

  
  
  PRBool HasLowDecodedData(PRInt64 aAudioUsecs) const;

  
  
  PRBool HasLowUndecodedData() const;

  
  
  PRInt64 GetUndecodedData() const;

  
  
  
  
  PRInt64 AudioDecodedUsecs() const;

  
  
  PRBool HasFutureAudio() const;

  
  PRBool JustExitedQuickBuffering();

  
  
  
  
  
  
  
  
  void Wait(PRInt64 aUsecs);

  
  void UpdateReadyState();

  
  void ResetPlayback();

  
  
  PRInt64 GetAudioClock();

  
  
  
  
  VideoData* FindStartTime();

  
  
  
  
  void UpdatePlaybackPositionInternal(PRInt64 aTime);

  
  
  void RenderVideoFrame(VideoData* aData, TimeStamp aTarget);
 
  
  
  
  
  
  void AdvanceFrame();

  
  
  
  
  
  
  
  PRUint32 PlaySilence(PRUint32 aSamples,
                       PRUint32 aChannels,
                       PRUint64 aSampleOffset);

  
  
  
  PRUint32 PlayFromAudioQueue(PRUint64 aSampleOffset, PRUint32 aChannels);

  
  
  void StopDecodeThread();

  
  
  void StopAudioThread();

  
  
  nsresult StartDecodeThread();

  
  
  nsresult StartAudioThread();

  
  
  
  void AudioLoop();

  
  
  
  void StopPlayback();

  
  
  
  void StartPlayback();

  
  
  void StartDecoding();

  
  
  PRBool IsPlaying();

  
  
  
  
  
  PRInt64 GetMediaTime() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return mStartTime + mCurrentFrameTime;
  }

  
  
  
  
  
  
  
  PRInt64 GetDecodedAudioDuration();

  
  
  nsresult DecodeMetadata();

  
  
  void DecodeSeek();

  
  
  void DecodeLoop();

  
  
  void DecodeThreadRun();

  
  nsresult CallRunStateMachine();

  
  
  
  nsresult RunStateMachine();

  PRBool IsStateMachineScheduled() const {
    mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
    return !mTimeout.IsNull() || mRunAgain;
  }

  
  
  
  PRBool IsPausedAndDecoderWaiting();

  
  
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

  
  
  
  
  nsRefPtr<nsAudioStream> mAudioStream;

  
  
  
  nsAutoPtr<nsBuiltinDecoderReader> mReader;

  
  
  
  
  PRInt64 mCurrentFrameTime;

  
  
  
  
  PRInt64 mAudioStartTime;

  
  
  
  PRInt64 mAudioEndTime;

  
  
  PRInt64 mVideoFrameEndTime;
  
  
  
  
  double mVolume;

  
  TimeStamp mDecodeStartTime;

  
  
  PRPackedBool mSeekable;

  
  
  
  
  
  PRPackedBool mPositionChangeQueued;

  
  
  
  
  
  PRPackedBool mAudioCompleted;

  
  
  PRPackedBool mGotDurationFromMetaData;
    
  
  
  PRPackedBool mStopDecodeThread;

  
  
  
  
  
  PRPackedBool mDecodeThreadIdle;

  
  
  PRPackedBool mStopAudioThread;

  
  
  
  
  
  PRPackedBool mQuickBuffering;

  
  
  PRPackedBool mIsRunning;

  
  
  PRPackedBool mRunAgain;

  
  
  
  
  
  PRPackedBool mDispatchedRunEvent;

  
  
  
  PRPackedBool mDecodeThreadWaiting;

private:
  
  
  
  nsAudioAvailableEventManager mEventManager;

  
  
  nsVideoInfo mInfo;
};

#endif
