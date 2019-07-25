


















































































































































































































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
#include "mozilla/ReentrantMonitor.h"

class nsAudioStream;

static inline bool IsCurrentThread(nsIThread* aThread) {
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

  
  
  virtual nsresult Init(nsDecoderStateMachine* aCloneDonor) = 0;

  
  
  virtual State GetState() = 0;

  
  
  virtual void SetVolume(double aVolume) = 0;

  virtual void Shutdown() = 0;

  
  
  virtual PRInt64 GetDuration() = 0;

  
  
  
  
  
  virtual void SetDuration(PRInt64 aDuration) = 0;

  
  
  
  virtual void SetEndTime(PRInt64 aEndTime) = 0;

  
  virtual void SetFragmentEndTime(PRInt64 aEndTime) = 0;

  
  
  virtual bool OnDecodeThread() const = 0;

  
  virtual bool OnStateMachineThread() const = 0;

  virtual nsHTMLMediaElement::NextFrameStatus GetNextFrameStatus() = 0;

  
  
  
  virtual void Play() = 0;

  
  virtual void Seek(double aTime) = 0;

  
  
  
  virtual double GetCurrentTime() const = 0;

  
  
  
  virtual void ClearPositionChangeFlag() = 0;

  
  
  
  virtual void SetSeekable(bool aSeekable) = 0;

  
  
  
  virtual bool IsSeekable() = 0;

  
  
  
  
  
  virtual void UpdatePlaybackPosition(PRInt64 aTime) = 0;

  virtual nsresult GetBuffered(nsTimeRanges* aBuffered) = 0;

  virtual PRInt64 VideoQueueMemoryInUse() = 0;
  virtual PRInt64 AudioQueueMemoryInUse() = 0;

  virtual void NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRUint32 aOffset) = 0;

  
  
  
  
  virtual void StartBuffering() = 0;

  
  
  virtual void SetFrameBufferLength(PRUint32 aLength) = 0;
};

class nsBuiltinDecoder : public nsMediaDecoder
{
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

 public:
  typedef mozilla::ReentrantMonitor ReentrantMonitor;

  
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
  
  virtual bool Init(nsHTMLMediaElement* aElement);

  
  
  virtual void Shutdown();
  
  virtual double GetCurrentTime();

  virtual nsresult Load(nsMediaStream* aStream,
                        nsIStreamListener** aListener,
                        nsMediaDecoder* aCloneDonor);

  virtual nsDecoderStateMachine* CreateStateMachine() = 0;

  
  
  virtual nsresult Play();

  
  virtual nsresult Seek(double aTime);

  virtual nsresult PlaybackRateChanged();

  virtual void Pause();
  virtual void SetVolume(double aVolume);
  virtual double GetDuration();

  virtual void SetInfinite(bool aInfinite);
  virtual bool IsInfinite();

  virtual nsMediaStream* GetCurrentStream();
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  virtual void NotifySuspendedStatusChanged();
  virtual void NotifyBytesDownloaded();
  virtual void NotifyDownloadEnded(nsresult aStatus);
  
  
  void NotifyBytesConsumed(PRInt64 aBytes);

  
  
  void ResourceLoaded();

  
  
  virtual void NetworkError();

  
  
  virtual bool IsSeeking() const;

  
  
  virtual bool IsEnded() const;

  
  
  
  virtual void SetDuration(double aDuration);

  
  virtual void SetSeekable(bool aSeekable);

  
  virtual bool IsSeekable();

  virtual nsresult GetSeekable(nsTimeRanges* aSeekable);

  
  
  virtual void SetEndTime(double aTime);

  virtual Statistics GetStatistics();

  
  
  
  virtual void Suspend();

  
  
  
  virtual void Resume(bool aForceBuffering);

  
  virtual void MoveLoadsToBackground();

  void AudioAvailable(float* aFrameBuffer, PRUint32 aFrameBufferLength, float aTime);

  
  
  void DurationChanged();

  bool OnStateMachineThread() const;

  bool OnDecodeThread() const {
    return mDecoderStateMachine->OnDecodeThread();
  }

  
  
  ReentrantMonitor& GetReentrantMonitor() { 
    return mReentrantMonitor; 
  }

  
  
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered) {
    if (mDecoderStateMachine) {
      return mDecoderStateMachine->GetBuffered(aBuffered);
    }
    return NS_ERROR_FAILURE;
  }

  virtual PRInt64 VideoQueueMemoryInUse() {
    if (mDecoderStateMachine) {
      return mDecoderStateMachine->VideoQueueMemoryInUse();
    }
    return 0;
  }

  virtual PRInt64 AudioQueueMemoryInUse() {
    if (mDecoderStateMachine) {
      return mDecoderStateMachine->AudioQueueMemoryInUse();
    }
    return 0;
  }

  virtual void NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRUint32 aOffset) {
    if (mDecoderStateMachine) {
      mDecoderStateMachine->NotifyDataArrived(aBuffer, aLength, aOffset);
    }
  }

  
  
  virtual nsresult RequestFrameBufferLength(PRUint32 aLength);

 public:
  
  
  PlayState GetState() {
    return mPlayState;
  }

  
  
  
  void StopProgressUpdates();

  
  
  void StartProgressUpdates();

  
  
  void UpdatePlaybackRate();

  
  double ComputePlaybackRate(bool* aReliable);

  
  
  
  
  void UpdatePlaybackPosition(PRInt64 aTime)
  {
    mDecoderStateMachine->UpdatePlaybackPosition(aTime);
  }

  




  
  
  
  void ChangeState(PlayState aState);

  
  
  void MetadataLoaded(PRUint32 aChannels,
                      PRUint32 aRate);

  
  
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

  
  void ReleaseStateMachine() { mDecoderStateMachine = nsnull; }

public:
  
  void DecodeError();

  
  
  nsresult ScheduleStateMachineThread();

  



  
  
  
  
  PRInt64 mDecoderPosition;
  
  
  
  
  PRInt64 mPlaybackPosition;
  
  
  
  nsChannelStatistics mPlaybackStatistics;

  
  
  
  
  double mCurrentTime;

  
  
  double mInitialVolume;

  
  
  
  
  
  double mRequestedSeekTime;

  
  
  
  PRInt64 mDuration;

  
  
  bool mSeekable;

  



  
  
  
  
  
  nsCOMPtr<nsDecoderStateMachine> mDecoderStateMachine;

  
  nsAutoPtr<nsMediaStream> mStream;

  
  
  
  ReentrantMonitor mReentrantMonitor;

  
  
  
  
  PlayState mPlayState;

  
  
  
  
  PlayState mNextState;	

  
  
  
  bool mResourceLoaded;

  
  
  
  
  
  bool mIgnoreProgressData;

  
  bool mInfiniteStream;
};

#endif
