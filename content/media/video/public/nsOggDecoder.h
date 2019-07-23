































































































#if !defined(nsOggDecoder_h___)
#define nsOggDecoder_h___

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"
#include "nsIChannel.h"
#include "nsChannelReader.h"
#include "nsIObserver.h"
#include "nsIFrame.h"
#include "nsAutoPtr.h"
#include "nsSize.h"
#include "prlock.h"
#include "prcvar.h"
#include "prlog.h"
#include "gfxContext.h"
#include "gfxRect.h"
#include "oggplay/oggplay.h"
#include "nsVideoDecoder.h"

class nsAudioStream;
class nsVideoDecodeEvent;
class nsVideoPresentationEvent;
class nsChannelToPipeListener;

class nsOggDecoder : public nsVideoDecoder
{
  friend class nsVideoDecodeEvent;
  friend class nsVideoPresentationEvent;
  friend class nsChannelToPipeListener;

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

 public:
  nsOggDecoder();
  PRBool Init();
  void Shutdown();
  ~nsOggDecoder();
  
  
  
  nsIntSize GetVideoSize(nsIntSize defaultSize);
  double GetVideoFramerate();

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

protected:
  




  
  
  void DisplayFirstFrame();

  
  
  PRBool StepDisplay();

  
  
  void ProcessTrack(int aTrackNumber, OggPlayCallbackInfo* aTrackInfo);

  
  
  
  
  double GetSyncTime();

  
  
  PRBool IsPaused();

  
  void HandleVideoData(int track_num, OggPlayVideoData* video_data);
  void HandleAudioData(OggPlayAudioData* audio_data, int size);

  
  void DoPause();

  
  
  void OpenAudioStream();

  
  
  void CloseAudioStream();

  
  
  void StartPresentationThread();

  




  
  
  
  void LoadOggHeaders();

  
  
  void LoadFirstFrame();

  
  
  
  PRBool StepDecoding();

  
  
  
  void BufferData();

  




  
  
  void MetadataLoaded();

  
  
  void FirstFrameLoaded();

  
  
  void ResourceLoaded();

  
  
  void PlaybackCompleted();

  
  
  virtual PRUint32 GetBytesLoaded();

  
  
  virtual PRUint32 GetTotalBytes();

  
  
  void BufferingStopped();

  
  
  void BufferingStarted();

private:
  
  
  void StartPlaybackThreads();

  




  
  PRUint32 mBytesDownloaded;

  


  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsChannelToPipeListener> mListener;

  
  nsCOMPtr<nsIURI> mURI;

  
  
  nsAutoPtr<nsAudioStream> mAudioStream;

  
  
  
  double mVideoNextFrameTime;

  
  
  PRPackedBool mLoadInProgress;

  
  
  PRPackedBool mPlayAfterLoad;

  
  PRPackedBool mNotifyOnShutdown;

  


  
  
  
  
  nsCOMPtr<nsIThread> mDecodeThread;
  nsCOMPtr<nsIThread> mPresentationThread;

  
  
  
  
  nsCOMPtr<nsVideoDecodeEvent> mDecodeEvent;
  nsCOMPtr<nsVideoPresentationEvent> mPresentationEvent;

  
  
  
  
  float mVideoCurrentFrameTime;

  
  
  
  
  double mInitialVolume;

  
  
  
  PRInt32 mAudioRate;
  PRInt32 mAudioChannels;
  PRInt32 mAudioTrack;

  
  
  PRInt32 mVideoTrack;

  
  
  OggPlay* mPlayer;

  
  
  
  nsChannelReader* mReader;

  
  
  
  PRPackedBool mPaused;

  
  
  
  
  
  PRPackedBool mFirstFrameLoaded;
  PRCondVar* mFirstFrameCondVar;
  PRLock* mFirstFrameLock;

  
  
  
  
  double mSystemSyncSeconds;

  
  
  
  PRPackedBool mResourceLoaded;

  
  
  PRPackedBool mMetadataLoaded;
};

#endif
