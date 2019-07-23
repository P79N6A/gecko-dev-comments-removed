





































#if !defined(nsOggPlayStateMachine_h__)
#define nsOggPlayStateMachine_h__

#include "prmem.h"
#include "nsThreadUtils.h"
#include "nsOggReader.h"
#include "nsOggDecoder.h"
#include "nsHTMLMediaElement.h"
#include "mozilla/Monitor.h"

using mozilla::TimeDuration;
using mozilla::TimeStamp;

class nsOggDecoder;



static inline PRBool IsThread(nsIThread* aThread) {
  return NS_GetCurrentThread() == aThread;
}


















class nsOggPlayStateMachine : public nsRunnable
{
public:
  
  enum State {
    DECODER_STATE_DECODING_METADATA,
    DECODER_STATE_DECODING,
    DECODER_STATE_SEEKING,
    DECODER_STATE_BUFFERING,
    DECODER_STATE_COMPLETED,
    DECODER_STATE_SHUTDOWN
  };

  nsOggPlayStateMachine(nsOggDecoder* aDecoder);
  ~nsOggPlayStateMachine();

  
  
  nsresult Init();

  
  
  
  void Shutdown();
  void Decode();

  
  void Seek(float aTime);

  
  
  NS_IMETHOD Run();

  
  
  PRBool HasAudio() const {
    mDecoder->GetMonitor().AssertCurrentThreadIn();
    return mInfo.mHasAudio;
  }

  
  
  PRBool HasVideo() const {
    mDecoder->GetMonitor().AssertCurrentThreadIn();
    return mInfo.mHasVideo;
  }

  
  
  
  float GetCurrentTime();

  
  
  PRInt64 GetDuration();

  
  
  
  void SetDuration(PRInt64 aDuration);

  
  
  void SetSeekable(PRBool aSeekable);

  
  
  void SetVolume(float aVolume);

  
  
  
  void ClearPositionChangeFlag();

  
  PRBool HaveNextFrameData() const {
    PRUint32 audioQueueSize = mReader->mAudioQueue.GetSize();
    return (mReader->mVideoQueue.GetSize() > 0 &&
            (!HasAudio() || audioQueueSize > 0)) ||
           audioQueueSize > 0;
  }

  
  PRBool IsBuffering() const {
    mDecoder->GetMonitor().AssertCurrentThreadIn();

    return mState == nsOggPlayStateMachine::DECODER_STATE_BUFFERING;
  }

  
  PRBool IsSeeking() const {
    mDecoder->GetMonitor().AssertCurrentThreadIn();

    return mState == nsOggPlayStateMachine::DECODER_STATE_SEEKING;
  }

  
  
  PRBool OnStateMachineThread() {
    return IsThread(mDecoder->mStateMachineThread);
  }

  PRBool OnDecodeThread() {
    return IsThread(mDecodeThread);
  }

  PRBool OnAudioThread() {
    return IsThread(mAudioThread);
  }

  
  void DecodeLoop();

  
  
  
  nsOggDecoder* mDecoder;

  
  
  
  
  
  void UpdatePlaybackPosition(PRInt64 aTime);

  nsHTMLMediaElement::NextFrameStatus GetNextFrameStatus();

  
  
  
  
  State mState;

private:

  
  
  
  
  
  
  void Wait(PRUint32 aMs);

  
  void UpdateReadyState();

  
  
  void ResetPlayback();

  
  
  PRInt64 GetAudioClock();

  
  
  
  
  VideoData* FindStartTime();

  
  
  
  void FindEndTime();

  
  
  void RenderVideoFrame(VideoData* aData);

  
  
  
  
  
  void AdvanceFrame();

  
  
  void StopDecodeThreads();

  
  
  nsresult StartDecodeThreads();

  
  
  
  void LoadOggHeaders();

  
  
  
  void AudioLoop();

  
  
  
  
  
  
  
  
  
  
  
  enum eStopMode {AUDIO_PAUSE, AUDIO_SHUTDOWN};
  void StopPlayback(eStopMode aMode);

  
  
  
  void StartPlayback();

  
  
  PRBool IsPlaying();

  
  nsOggInfo mInfo;

  
  
  
  
  Monitor mAudioMonitor;

  
  
  
  nsAutoPtr<nsOggReader> mReader;

  
  
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

  
  
  
  
  PRInt64 mCurrentFrameTime;

  
  
  
  PRInt64 mAudioStartTime;

  
  
  
  PRInt64 mAudioEndTime;

  
  
  PRInt64 mVideoFrameTime;
  
  
  
  
  float mVolume;

  
  
  PRPackedBool mSeekable;

  
  
  
  
  
  PRPackedBool mPositionChangeQueued;

  
  
  
  
  
  PRPackedBool mAudioCompleted;

  
  
  
  PRPackedBool mBufferExhausted;

  
  
  PRPackedBool mGotDurationFromHeader;
    
  
  
  PRPackedBool mStopDecodeThreads;
};


#endif
