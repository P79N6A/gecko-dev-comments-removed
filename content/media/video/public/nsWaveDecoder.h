




































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

  
  virtual float GetVolume();

  
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

  
  virtual PRInt64 GetTotalBytes();
  virtual void SetTotalBytes(PRInt64 aBytes);

  
  virtual void SetSeekable(PRBool aSeekable);
  virtual PRBool GetSeekable();

  
  virtual PRUint64 GetBytesLoaded();
  virtual void UpdateBytesDownloaded(PRUint64 aBytes);

  
  virtual void Shutdown();

private:
  
  void BufferingStarted();

  
  void BufferingStopped();

  
  void SeekingStarted();

  
  void SeekingStopped();

  
  
  void MetadataLoaded();

  
  void PlaybackEnded();

  
  void MediaErrorDecode();

  void RegisterShutdownObserver();
  void UnregisterShutdownObserver();

  
  PRInt64 mContentLength;

  
  PRUint64 mBytesDownloaded;

  
  float mInitialVolume;

  
  nsCOMPtr<nsIURI> mURI;

  
  nsCOMPtr<nsIThread> mPlaybackThread;

  
  
  nsCOMPtr<nsWaveStateMachine> mPlaybackStateMachine;

  
  
  nsAutoPtr<nsMediaStream> mStream;

  
  
  
  
  
  float mTimeOffset;

  
  
  
  float mEndedCurrentTime;
  float mEndedDuration;
  PRPackedBool mEnded;

  
  PRPackedBool mNotifyOnShutdown;

  
  PRPackedBool mSeekable;
};

#endif
