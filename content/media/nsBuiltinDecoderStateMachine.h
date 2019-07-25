














































































































#if !defined(nsBuiltinDecoderStateMachine_h__)
#define nsBuiltinDecoderStateMachine_h__

#include "prmem.h"
#include "nsThreadUtils.h"
#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderReader.h"
#include "nsAudioAvailableEventManager.h"
#include "nsHTMLMediaElement.h"
#include "mozilla/Monitor.h"


















class nsBuiltinDecoderStateMachine : public nsDecoderStateMachine
{
public:
  typedef mozilla::Monitor Monitor;
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  nsBuiltinDecoderStateMachine(nsBuiltinDecoder* aDecoder, nsBuiltinDecoderReader* aReader);
  ~nsBuiltinDecoderStateMachine();

  
  virtual nsresult Init();
  State GetState()
  { 
    mDecoder->GetMonitor().AssertCurrentThreadIn();
    return mState; 
  }
  virtual void SetVolume(float aVolume);
  virtual void Shutdown();
  virtual PRInt64 GetDuration();
  virtual void SetDuration(PRInt64 aDuration);
  virtual PRBool OnDecodeThread() {
    return IsCurrentThread(mDecodeThread);
  }

  virtual nsHTMLMediaElement::NextFrameStatus GetNextFrameStatus();
  virtual void Decode();
  virtual void Seek(float aTime);
  virtual float GetCurrentTime();
  virtual void ClearPositionChangeFlag();
  virtual void SetSeekable(PRBool aSeekable);
  virtual void UpdatePlaybackPosition(PRInt64 aTime);
  virtual void StartBuffering();


  
  
  virtual void LoadMetadata();

  
  
  NS_IMETHOD Run();

  
  
  PRBool HasAudio() const {
    mDecoder->GetMonitor().AssertCurrentThreadIn();
    return mReader->GetInfo().mHasAudio;
  }

  
  
  PRBool HasVideo() const {
    mDecoder->GetMonitor().AssertCurrentThreadIn();
    return mReader->GetInfo().mHasVideo;
  }

  
  PRBool HaveNextFrameData() const;

  
  PRBool IsBuffering() const {
    mDecoder->GetMonitor().AssertCurrentThreadIn();

    return mState == nsBuiltinDecoderStateMachine::DECODER_STATE_BUFFERING;
  }

  
  PRBool IsSeeking() const {
    mDecoder->GetMonitor().AssertCurrentThreadIn();

    return mState == nsBuiltinDecoderStateMachine::DECODER_STATE_SEEKING;
  }

  
  
  PRBool OnAudioThread() {
    return IsCurrentThread(mAudioThread);
  }

  PRBool OnStateMachineThread() {
    return mDecoder->OnStateMachineThread();
  }

  
  void DecodeLoop();

  
  
  
  nsBuiltinDecoder* mDecoder;

  
  
  
  
  State mState;

  nsresult GetBuffered(nsTimeRanges* aBuffered) {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    return mReader->GetBuffered(aBuffered, mStartTime);
  }

  void NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRUint32 aOffset) {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    mReader->NotifyDataArrived(aBuffer, aLength, aOffset);
  }

protected:

  
  
  
  PRInt64 AudioDecodedMs() const;

  
  PRBool HasLowDecodedData() const;

  
  PRBool HasAmpleDecodedData() const;

  
  
  PRBool HasFutureAudio() const;

  
  
  
  
  
  
  void Wait(PRUint32 aMs);

  
  void UpdateReadyState();

  
  
  void ResetPlayback();

  
  
  PRInt64 GetAudioClock();

  
  
  
  
  VideoData* FindStartTime();

  
  
  
  void FindEndTime();

  
  
  void RenderVideoFrame(VideoData* aData);

  
  
  
  
  
  void AdvanceFrame();

  
  
  
  
  
  
  
  PRUint32 PlaySilence(PRUint32 aSamples, PRUint32 aChannels,
                       PRUint64 aSampleOffset);

  
  
  
  PRUint32 PlayFromAudioQueue(PRUint64 aSampleOffset, PRUint32 aChannels);

  
  
  void StopDecodeThreads();

  
  
  nsresult StartDecodeThreads();

  
  
  
  void AudioLoop();

  
  
  
  
  
  
  
  
  
  
  
  enum eStopMode {AUDIO_PAUSE, AUDIO_SHUTDOWN};
  void StopPlayback(eStopMode aMode);

  
  
  
  void StartPlayback();

  
  
  PRBool IsPlaying();

  
  
  
  
  
  PRInt64 GetMediaTime() const {
    mDecoder->GetMonitor().AssertCurrentThreadIn();
    return mStartTime + mCurrentFrameTime;
  }

  
  
  
  
  Monitor mAudioMonitor;

  
  
  PRUint32 mCbCrSize;

  
  nsAutoArrayPtr<unsigned char> mCbCrBuffer;

  
  
  nsCOMPtr<nsIThread> mAudioThread;

  
  nsCOMPtr<nsIThread> mDecodeThread;

  
  
  
  TimeStamp mPlayStartTime;

  
  
  
  TimeDuration mPlayDuration;

  
  
  TimeStamp mBufferingStart;

  
  
  PRInt64 mBufferingEndOffset;

  
  
  
  
  PRInt64 mStartTime;

  
  
  
  PRInt64 mEndTime;

  
  
  
  PRInt64 mSeekTime;

  
  
  
  nsAutoPtr<nsAudioStream> mAudioStream;

  
  
  
  nsAutoPtr<nsBuiltinDecoderReader> mReader;

  
  
  
  
  PRInt64 mCurrentFrameTime;

  
  
  
  PRInt64 mAudioStartTime;

  
  
  
  PRInt64 mAudioEndTime;

  
  
  PRInt64 mVideoFrameEndTime;
  
  
  
  
  float mVolume;

  
  
  PRPackedBool mSeekable;

  
  
  
  
  
  PRPackedBool mPositionChangeQueued;

  
  
  
  
  
  PRPackedBool mAudioCompleted;

  
  
  
  PRPackedBool mBufferExhausted;

  
  
  PRPackedBool mGotDurationFromHeader;
    
  
  
  PRPackedBool mStopDecodeThreads;

private:
  
  
  
  nsAudioAvailableEventManager mEventManager;
};

#endif
