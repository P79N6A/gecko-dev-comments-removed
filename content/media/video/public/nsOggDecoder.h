
























































































































































































































































#if !defined(nsOggDecoder_h_)
#define nsOggDecoder_h_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"
#include "nsIChannel.h"
#include "nsChannelReader.h"
#include "nsIObserver.h"
#include "nsIFrame.h"
#include "nsAutoPtr.h"
#include "nsSize.h"
#include "prlog.h"
#include "prmon.h"
#include "gfxContext.h"
#include "gfxRect.h"
#include "oggplay/oggplay.h"
#include "nsMediaDecoder.h"

class nsAudioStream;
class nsOggDecodeStateMachine;

class nsOggDecoder : public nsMediaDecoder
{
  friend class nsOggDecodeStateMachine;

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

 public:
  
  enum PlayState {
    PLAY_STATE_START,
    PLAY_STATE_LOADING,
    PLAY_STATE_PAUSED,
    PLAY_STATE_PLAYING,
    PLAY_STATE_SEEKING,
    PLAY_STATE_ENDED,
    PLAY_STATE_SHUTDOWN
  };

  nsOggDecoder();
  ~nsOggDecoder();
  virtual PRBool Init(nsHTMLMediaElement* aElement);

  
  
  virtual void Shutdown();
  
  virtual float GetCurrentTime();

  virtual nsresult Load(nsIURI* aURI,
                        nsIChannel* aChannel,
                        nsIStreamListener **aListener);

  
  
  virtual nsresult Play();

  
  virtual void Stop();

  
  virtual nsresult Seek(float time);

  virtual nsresult PlaybackRateChanged();

  virtual void Pause();
  virtual void SetVolume(float volume);
  virtual float GetDuration();

  virtual void GetCurrentURI(nsIURI** aURI);
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

  
  nsChannelReader* GetReader() { return mReader; }

  virtual Statistics GetStatistics();

  
  
  
  virtual void Suspend();

  
  
  
  virtual void Resume();

  
  virtual void MoveLoadsToBackground();

protected:

  
  
  PRMonitor* GetMonitor() 
  { 
    return mMonitor; 
  }

  
  
  PlayState GetState()
  {
    return mPlayState;
  }

  
  
  
  void StopProgressUpdates();

  
  
  void StartProgressUpdates();

  
  
  void UpdatePlaybackRate();

  
  double ComputePlaybackRate(PRPackedBool* aReliable);

  




  
  
  
  void ChangeState(PlayState aState);

  
  
  void MetadataLoaded();

  
  
  void FirstFrameLoaded();

  
  
  void PlaybackEnded();

  
  
  void SeekingStopped();

  
  
  void SeekingStarted();

  
  
  
  void PlaybackPositionChanged();

  
  
  void UpdateReadyStateForData();

  
  
  PRInt64 GetDownloadPosition();

private:
  
  
  void RegisterShutdownObserver();
  void UnregisterShutdownObserver();

  



  
  
  
  
  PRInt64 mDecoderPosition;
  
  
  
  
  PRInt64 mPlaybackPosition;
  
  
  
  nsChannelStatistics mPlaybackStatistics;

  
  nsCOMPtr<nsIURI> mURI;

  
  nsCOMPtr<nsIThread> mDecodeThread;

  
  
  
  
  float mCurrentTime;

  
  
  
  
  float mInitialVolume;

  
  
  
  
  
  float mRequestedSeekTime;

  
  
  
  PRInt64 mDuration;

  
  PRPackedBool mNotifyOnShutdown;

  
  
  PRPackedBool mSeekable;

  



  
  
  
  
  
  
  nsCOMPtr<nsOggDecodeStateMachine> mDecodeStateMachine;

  
  
  
  
  
  
  nsAutoPtr<nsChannelReader> mReader;

  
  
  
  PRMonitor* mMonitor;

  
  
  
  
  PlayState mPlayState;

  
  
  
  
  PlayState mNextState;	

  
  
  
  PRPackedBool mResourceLoaded;

  
  
  
  
  
  PRPackedBool mIgnoreProgressData;
};

#endif
