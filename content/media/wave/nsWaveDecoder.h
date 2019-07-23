




































#if !defined(nsWaveDecoder_h_)
#define nsWaveDecoder_h_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsMediaDecoder.h"
#include "nsMediaStream.h"



























































































class nsWaveStateMachine;

class nsWaveDecoder : public nsMediaDecoder
{
  friend class nsWaveStateMachine;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

 public:
  nsWaveDecoder();
  ~nsWaveDecoder();

  virtual PRBool Init(nsHTMLMediaElement* aElement);

  virtual void GetCurrentURI(nsIURI** aURI);
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  
  virtual float GetCurrentTime();

  
  virtual float GetDuration();

  
  virtual void SetVolume(float aVolume);

  virtual nsresult Play();
  virtual void Pause();

  
  
  
  virtual nsresult Seek(float aTime);

  
  virtual PRBool IsSeeking() const;

  
  virtual PRBool IsEnded() const;

  
  
  virtual nsresult Load(nsIURI* aURI, nsIChannel* aChannel, nsIStreamListener** aStreamListener);

  
  
  virtual void ResourceLoaded();

  
  
  virtual void NetworkError();

  
  virtual nsresult PlaybackRateChanged();

  virtual void NotifySuspendedStatusChanged();
  virtual void NotifyBytesDownloaded();
  virtual void NotifyDownloadEnded(nsresult aStatus);

  virtual Statistics GetStatistics();

  void PlaybackPositionChanged();

  
  
  virtual void SetDuration(PRInt64 aDuration);

  
  virtual void SetSeekable(PRBool aSeekable);
  virtual PRBool GetSeekable();

  
  virtual void Shutdown();

  
  
  
  virtual void Suspend();

  
  
  
  virtual void Resume();

  
  void UpdateReadyStateForData();

  
  virtual void MoveLoadsToBackground();

  
  void Stop();

private:
  
  void SeekingStarted();

  
  void SeekingStopped();

  
  
  void MetadataLoaded();

  
  void PlaybackEnded();

  
  void MediaErrorDecode();

  void RegisterShutdownObserver();
  void UnregisterShutdownObserver();

  
  float mInitialVolume;

  
  nsCOMPtr<nsIURI> mURI;

  
  nsCOMPtr<nsIThread> mPlaybackThread;

  
  
  nsCOMPtr<nsWaveStateMachine> mPlaybackStateMachine;

  
  
  nsAutoPtr<nsMediaStream> mStream;

  
  
  
  
  float mCurrentTime;

  
  
  
  float mEndedDuration;
  PRPackedBool mEnded;

  
  PRPackedBool mSeekable;

  
  
  PRPackedBool mResourceLoaded;

  
  PRPackedBool mMetadataLoadedReported;

  
  PRPackedBool mResourceLoadedReported;
};

#endif
