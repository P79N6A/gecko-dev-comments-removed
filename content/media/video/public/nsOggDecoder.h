
























































































































































































































































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
  PRBool Init();

  
  
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
  virtual float GetVolume();
  virtual void SetVolume(float volume);
  virtual float GetDuration();

  virtual void GetCurrentURI(nsIURI** aURI);
  virtual nsIPrincipal* GetCurrentPrincipal();

  virtual void UpdateBytesDownloaded(PRUint64 aBytes);

  
  
  void ResourceLoaded();

  
  
  void NetworkError();

  
  
  virtual PRBool IsSeeking() const;

  
  virtual void SetTotalBytes(PRInt64 aBytes);

  
  virtual void SetSeekable(PRBool aSeekable);

  
  virtual PRBool GetSeekable();

protected:
  
  
  
  void ChangeState(PlayState aState);

  
  
  PRMonitor* GetMonitor() 
  { 
    return mMonitor; 
  }

  
  
  PlayState GetState()
  {
    return mPlayState;
  }

  




  
  
  void MetadataLoaded();

  
  
  void FirstFrameLoaded();

  
  
  void PlaybackEnded();

  
  
  virtual PRUint64 GetBytesLoaded();

  
  
  virtual PRInt64 GetTotalBytes();

  
  
  void BufferingStopped();

  
  
  void BufferingStarted();

  
  
  void SeekingStopped();

  
  
  void SeekingStarted();

  
  
  
  void PlaybackPositionChanged();

private:
  
  
  void RegisterShutdownObserver();
  void UnregisterShutdownObserver();

  


  
  PRUint64 mBytesDownloaded;

  
  nsCOMPtr<nsIURI> mURI;

  
  nsCOMPtr<nsIThread> mDecodeThread;

  
  
  
  
  float mCurrentTime;

  
  
  
  
  float mInitialVolume;

  
  
  
  
  
  float mRequestedSeekTime;

  
  
  
  PRInt64 mContentLength;

  
  PRPackedBool mNotifyOnShutdown;

  
  
  PRPackedBool mSeekable;

  



  
  
  
  
  
  
  nsCOMPtr<nsOggDecodeStateMachine> mDecodeStateMachine;

  
  
  
  
  
  
  nsChannelReader* mReader;

  
  
  
  PRMonitor* mMonitor;

  
  
  
  
  PlayState mPlayState;

  
  
  
  
  PlayState mNextState;
};

#endif
