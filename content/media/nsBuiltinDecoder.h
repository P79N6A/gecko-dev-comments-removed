


































































































































































































#if !defined(nsBuiltinDecoder_h_)
#define nsBuiltinDecoder_h_

#include "nsMediaDecoder.h"

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"
#include "nsIChannel.h"
#include "nsIObserver.h"
#include "nsIFrame.h"
#include "nsAutoPtr.h"
#include "nsSize.h"
#include "prlog.h"
#include "gfxContext.h"
#include "gfxRect.h"
#include "nsMediaStream.h"
#include "nsMediaDecoder.h"
#include "nsHTMLMediaElement.h"
#include "mozilla/Monitor.h"

class nsAudioStream;

static inline PRBool IsCurrentThread(nsIThread* aThread) {
  return NS_GetCurrentThread() == aThread;
}



class nsDecoderStateMachine : public nsRunnable
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

  
  
  virtual nsresult Init() = 0;

  
  
  virtual State GetState() = 0;

  
  
  virtual void SetVolume(float aVolume) = 0;

  virtual void Shutdown() = 0;

  
  
  virtual PRInt64 GetDuration() = 0;

  
  
  
  
  virtual void SetDuration(PRInt64 aDuration) = 0;

  
  
  virtual PRBool OnDecodeThread() = 0;

  virtual nsHTMLMediaElement::NextFrameStatus GetNextFrameStatus() = 0;

  
  
  
  virtual void Decode() = 0;

  
  virtual void Seek(float aTime) = 0;

  
  
  
  virtual float GetCurrentTime() = 0;

  
  
  
  virtual void ClearPositionChangeFlag() = 0;

  
  
  virtual void SetSeekable(PRBool aSeekable) = 0;

  
  
  
  
  
  virtual void UpdatePlaybackPosition(PRInt64 aTime) = 0;

  virtual nsresult GetBuffered(nsHTMLTimeRanges* aBuffered) = 0;
  
  
  
  
  
  virtual void StartBuffering() = 0;
};

class nsBuiltinDecoder : public nsMediaDecoder
{
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

 public:
  typedef mozilla::Monitor Monitor;

  
  enum PlayState {
    PLAY_STATE_START,
    PLAY_STATE_LOADING,
    PLAY_STATE_PAUSED,
    PLAY_STATE_PLAYING,
    PLAY_STATE_SEEKING,
    PLAY_STATE_ENDED,
    PLAY_STATE_SHUTDOWN
  };

  nsBuiltinDecoder();
  ~nsBuiltinDecoder();
  
  virtual PRBool Init(nsHTMLMediaElement* aElement);

  
  
  virtual void Shutdown();
  
  virtual float GetCurrentTime();

  virtual nsresult Load(nsMediaStream* aStream,
                        nsIStreamListener** aListener);

  virtual nsDecoderStateMachine* CreateStateMachine() = 0;

  
  
  virtual nsresult Play();

  
  virtual nsresult Seek(float time);

  virtual nsresult PlaybackRateChanged();

  virtual void Pause();
  virtual void SetVolume(float volume);
  virtual float GetDuration();

  virtual nsMediaStream* GetCurrentStream();
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  virtual void NotifySuspendedStatusChanged();
  virtual void NotifyBytesDownloaded();
  virtual void NotifyDownloadEnded(nsresult aStatus);
  
  
  void NotifyBytesConsumed(PRInt64 aBytes);

  
  
  void ResourceLoaded();

  
  
  virtual void NetworkError();

  
  
  virtual PRBool IsSeeking() const;

  
  
  virtual PRBool IsEnded() const;

  
  
  
  virtual void SetDuration(PRInt64 aDuration);

  
  virtual void SetSeekable(PRBool aSeekable);

  
  virtual PRBool GetSeekable();

  virtual Statistics GetStatistics();

  
  
  
  virtual void Suspend();

  
  
  
  virtual void Resume(PRBool aForceBuffering);

  
  virtual void MoveLoadsToBackground();

  
  
  void Stop();

  
  
  void DurationChanged();

  PRBool OnStateMachineThread() {
    return IsCurrentThread(mStateMachineThread);
  }

  PRBool OnDecodeThread() {
    return mDecoderStateMachine->OnDecodeThread();
  }

  
  
  Monitor& GetMonitor() { 
    return mMonitor; 
  }

  
  
  virtual nsresult GetBuffered(nsHTMLTimeRanges* aBuffered) {
    return mDecoderStateMachine->GetBuffered(aBuffered);
  }

 public:
  
  
  PlayState GetState() {
    return mPlayState;
  }

  
  
  
  void StopProgressUpdates();

  
  
  void StartProgressUpdates();

  
  
  void UpdatePlaybackRate();

  
  double ComputePlaybackRate(PRPackedBool* aReliable);

  
  
  
  
  void UpdatePlaybackPosition(PRInt64 aTime)
  {
    mDecoderStateMachine->UpdatePlaybackPosition(aTime);
  }

  




  
  
  
  void ChangeState(PlayState aState);

  
  
  void MetadataLoaded();

  
  
  void FirstFrameLoaded();

  
  
  void PlaybackEnded();

  
  
  void SeekingStopped();

  
  
  void SeekingStoppedAtEnd();

  
  
  void SeekingStarted();

  
  
  
  void PlaybackPositionChanged();

  
  
  void NextFrameUnavailableBuffering();
  void NextFrameAvailable();
  void NextFrameUnavailable();

  
  
  void UpdateReadyStateForData();

  
  
  PRInt64 GetDownloadPosition();

  
  
  void UpdatePlaybackOffset(PRInt64 aOffset);

  
  nsDecoderStateMachine* GetStateMachine() { return mDecoderStateMachine; }

  
  
  nsDecoderStateMachine::State GetDecodeState() { return mDecoderStateMachine->GetState(); }

public:
  
  void DecodeError();

  



  
  
  
  
  PRInt64 mDecoderPosition;
  
  
  
  
  PRInt64 mPlaybackPosition;
  
  
  
  nsChannelStatistics mPlaybackStatistics;

  
  nsCOMPtr<nsIThread> mStateMachineThread;

  
  
  
  
  float mCurrentTime;

  
  
  float mInitialVolume;

  
  
  
  
  
  float mRequestedSeekTime;

  
  
  
  PRInt64 mDuration;

  
  
  PRPackedBool mSeekable;

  



  
  
  
  
  
  nsCOMPtr<nsDecoderStateMachine> mDecoderStateMachine;

  
  nsAutoPtr<nsMediaStream> mStream;

  
  
  
  Monitor mMonitor;

  
  
  
  
  PlayState mPlayState;

  
  
  
  
  PlayState mNextState;	

  
  
  
  PRPackedBool mResourceLoaded;

  
  
  
  
  
  PRPackedBool mIgnoreProgressData;
};

#endif
