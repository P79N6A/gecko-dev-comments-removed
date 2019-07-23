




































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

  virtual void GetCurrentURI(nsIURI** aURI);
  virtual nsIPrincipal* GetCurrentPrincipal();

  
  virtual float GetCurrentTime();

  
  virtual float GetDuration();

  
  virtual void SetVolume(float aVolume);

  virtual nsresult Play();
  virtual void Stop();
  virtual void Pause();

  
  
  
  virtual nsresult Seek(float aTime);

  
  virtual PRBool IsSeeking() const;

  
  virtual PRBool IsEnded() const;

  
  
  virtual nsresult Load(nsIURI* aURI, nsIChannel* aChannel, nsIStreamListener** aStreamListener);

  
  
  virtual void ResourceLoaded();

  
  
  virtual void NetworkError();

  
  virtual nsresult PlaybackRateChanged();

  virtual void NotifyBytesDownloaded(PRInt64 aBytes);
  virtual void NotifyDownloadSeeked(PRInt64 aOffset);
  virtual void NotifyDownloadEnded(nsresult aStatus);
  virtual void NotifyBytesConsumed(PRInt64 aBytes);

  virtual Statistics GetStatistics();

  virtual void SetTotalBytes(PRInt64 aBytes);

  void PlaybackPositionChanged();

  
  
  virtual void SetDuration(PRInt64 aDuration);

  
  virtual void SetSeekable(PRBool aSeekable);
  virtual PRBool GetSeekable();

  
  virtual void Shutdown();

  
  
  
  virtual void Suspend();

  
  
  
  virtual void Resume();

  
  void UpdateReadyStateForData();

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

  
  
  
  
  
  float mTimeOffset;

  
  
  
  
  float mCurrentTime;

  
  
  
  float mEndedCurrentTime;
  float mEndedDuration;
  PRPackedBool mEnded;

  
  PRPackedBool mNotifyOnShutdown;

  
  PRPackedBool mSeekable;

  
  
  PRPackedBool mResourceLoaded;

  
  PRPackedBool mMetadataLoadedReported;

  
  PRPackedBool mResourceLoadedReported;
};

#endif
