




















































































































































































































































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
class nsOggDisplayStateMachine;

class nsOggDecoder : public nsMediaDecoder
{
  friend class nsOggDecodeStateMachine;
  friend class nsOggDisplayStateMachine;

  
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
  void Shutdown();
  
  float GetCurrentTime();

  
  
  
  nsresult Load(nsIURI* aURI);

  
  
  nsresult Play();

  
  virtual void Stop();

  
  nsresult Seek(float time);

  nsresult PlaybackRateChanged();

  void Pause();
  float GetVolume();
  void SetVolume(float volume);
  float GetDuration();

  void GetCurrentURI(nsIURI** aURI);
  nsIPrincipal* GetCurrentPrincipal();

  virtual void UpdateBytesDownloaded(PRUint32 aBytes);

  
  
  void ResourceLoaded();

  
  
  virtual PRBool IsSeeking() const;

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

  
  
  virtual PRUint32 GetBytesLoaded();

  
  
  virtual PRUint32 GetTotalBytes();

  
  
  void BufferingStopped();

  
  
  void BufferingStarted();

  
  
  void SeekingStopped();

  
  
  void SeekingStarted();

private:
  
  
  void RegisterShutdownObserver();
  void UnregisterShutdownObserver();


  


  
  PRUint32 mBytesDownloaded;

  
  nsCOMPtr<nsIURI> mURI;

  
  nsCOMPtr<nsIThread> mDecodeThread;
  nsCOMPtr<nsIThread> mDisplayThread;

  
  
  
  
  float mInitialVolume;

  
  
  
  
  
  float mSeekTime;

  
  PRPackedBool mNotifyOnShutdown;

  



  
  
  
  
  
  
  nsCOMPtr<nsOggDecodeStateMachine> mDecodeStateMachine;

  
  
  
  
  
  nsCOMPtr<nsOggDisplayStateMachine> mDisplayStateMachine;
  
  
  
  
  
  
  
  nsChannelReader* mReader;

  
  
  
  PRMonitor* mMonitor;

  
  
  
  
  PlayState mPlayState;

  
  
  
  
  PlayState mNextState;
};

#endif
