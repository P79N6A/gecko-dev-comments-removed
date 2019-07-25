

















































































































































































#if !defined(nsBuiltinDecoder_h_)
#define nsBuiltinDecoder_h_

#include "nsMediaDecoder.h"

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"
#include "nsIChannel.h"
#include "nsIObserver.h"
#include "nsAutoPtr.h"
#include "nsSize.h"
#include "prlog.h"
#include "gfxContext.h"
#include "gfxRect.h"
#include "MediaResource.h"
#include "nsMediaDecoder.h"
#include "nsHTMLMediaElement.h"
#include "mozilla/ReentrantMonitor.h"

namespace mozilla {
namespace layers {
class Image;
} 
} 

typedef mozilla::layers::Image Image;

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
  virtual void SetAudioCaptured(bool aCapture) = 0;

  virtual void Shutdown() = 0;

  
  
  virtual int64_t GetDuration() = 0;

  
  
  
  
  
  virtual void SetDuration(int64_t aDuration) = 0;

  
  
  
  virtual void SetEndTime(int64_t aEndTime) = 0;

  
  virtual void SetFragmentEndTime(int64_t aEndTime) = 0;

  
  
  virtual bool OnDecodeThread() const = 0;

  
  virtual bool OnStateMachineThread() const = 0;

  virtual nsHTMLMediaElement::NextFrameStatus GetNextFrameStatus() = 0;

  
  
  
  virtual void Play() = 0;

  
  virtual void Seek(double aTime) = 0;

  
  
  
  virtual double GetCurrentTime() const = 0;

  
  
  
  virtual void ClearPositionChangeFlag() = 0;

  
  
  
  virtual void SetSeekable(bool aSeekable) = 0;

  
  
  
  virtual bool IsSeekable() = 0;

  
  
  
  
  
  virtual void UpdatePlaybackPosition(int64_t aTime) = 0;

  virtual nsresult GetBuffered(nsTimeRanges* aBuffered) = 0;

  
  virtual bool IsSeekableInBufferedRanges() = 0;

  virtual int64_t VideoQueueMemoryInUse() = 0;
  virtual int64_t AudioQueueMemoryInUse() = 0;

  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) = 0;

  
  
  
  
  virtual void StartBuffering() = 0;

  
  
  virtual void SetFrameBufferLength(uint32_t aLength) = 0;

  
  
  virtual void NotifyAudioAvailableListener() = 0;
};

class nsBuiltinDecoder : public nsMediaDecoder
{
public:
  typedef mozilla::MediaChannelStatistics MediaChannelStatistics;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
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

  virtual nsresult Load(MediaResource* aResource,
                        nsIStreamListener** aListener,
                        nsMediaDecoder* aCloneDonor);

  virtual nsDecoderStateMachine* CreateStateMachine() = 0;

  
  
  virtual nsresult Play();

  
  virtual nsresult Seek(double aTime);

  virtual nsresult PlaybackRateChanged();

  virtual void Pause();
  virtual void SetVolume(double aVolume);
  virtual void SetAudioCaptured(bool aCaptured);

  virtual void AddOutputStream(SourceMediaStream* aStream, bool aFinishWhenEnded);
  
  struct OutputMediaStream {
    OutputMediaStream();
    ~OutputMediaStream();
    OutputMediaStream(const OutputMediaStream& rhs);

    void Init(int64_t aInitialTime, SourceMediaStream* aStream, bool aFinishWhenEnded);
    
    int64_t mLastAudioPacketTime; 
    int64_t mLastAudioPacketEndTime; 
    
    int64_t mAudioFramesWritten;
    
    int64_t mAudioFramesWrittenBaseTime; 
    
    
    
    int64_t mNextVideoTime; 
    
    
    nsRefPtr<Image> mLastVideoImage;
    nsRefPtr<SourceMediaStream> mStream;
    gfxIntSize mLastVideoImageDisplaySize;
    
    
    bool mStreamInitialized;
    bool mFinishWhenEnded;
    bool mHaveSentFinish;
    bool mHaveSentFinishAudio;
    bool mHaveSentFinishVideo;
  };
  nsTArray<OutputMediaStream>& OutputStreams()
  {
    GetReentrantMonitor().AssertCurrentThreadIn();
    return mOutputStreams;
  }

  virtual double GetDuration();

  virtual void SetInfinite(bool aInfinite);
  virtual bool IsInfinite();

  virtual MediaResource* GetResource() { return mResource; }
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  virtual void NotifySuspendedStatusChanged();
  virtual void NotifyBytesDownloaded();
  virtual void NotifyDownloadEnded(nsresult aStatus);
  virtual void NotifyPrincipalChanged();
  
  
  void NotifyBytesConsumed(int64_t aBytes);

  
  
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

  void AudioAvailable(float* aFrameBuffer, uint32_t aFrameBufferLength, float aTime);

  
  
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

  virtual int64_t VideoQueueMemoryInUse() {
    if (mDecoderStateMachine) {
      return mDecoderStateMachine->VideoQueueMemoryInUse();
    }
    return 0;
  }

  virtual int64_t AudioQueueMemoryInUse() {
    if (mDecoderStateMachine) {
      return mDecoderStateMachine->AudioQueueMemoryInUse();
    }
    return 0;
  }

  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) {
    if (mDecoderStateMachine) {
      mDecoderStateMachine->NotifyDataArrived(aBuffer, aLength, aOffset);
    }
  }

  
  
  virtual nsresult RequestFrameBufferLength(uint32_t aLength);

  
  
  PlayState GetState() {
    return mPlayState;
  }

  
  
  
  void StopProgressUpdates();

  
  
  void StartProgressUpdates();

  
  
  void UpdatePlaybackRate();

  
  double ComputePlaybackRate(bool* aReliable);

  
  
  
  
  void UpdatePlaybackPosition(int64_t aTime)
  {
    mDecoderStateMachine->UpdatePlaybackPosition(aTime);
  }

  




  
  
  
  void ChangeState(PlayState aState);

  
  
  void MetadataLoaded(uint32_t aChannels,
                      uint32_t aRate,
                      bool aHasAudio,
                      const nsHTMLMediaElement::MetadataTags* aTags);

  
  
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

  
  
  int64_t GetDownloadPosition();

  
  
  void UpdatePlaybackOffset(int64_t aOffset);

  
  nsDecoderStateMachine* GetStateMachine() { return mDecoderStateMachine; }

  
  
  nsDecoderStateMachine::State GetDecodeState() { return mDecoderStateMachine->GetState(); }

  
  void ReleaseStateMachine() { mDecoderStateMachine = nullptr; }

   
   
   virtual void NotifyAudioAvailableListener();

  
  void DecodeError();

  
  
  nsresult ScheduleStateMachineThread();

  



  
  
  
  
  int64_t mDecoderPosition;
  
  
  
  
  int64_t mPlaybackPosition;
  
  
  
  MediaChannelStatistics mPlaybackStatistics;

  
  
  
  
  double mCurrentTime;

  
  
  double mInitialVolume;

  
  
  
  
  
  double mRequestedSeekTime;

  
  
  
  int64_t mDuration;

  
  bool mInitialAudioCaptured;

  
  
  bool mSeekable;

  



  
  
  
  
  
  nsCOMPtr<nsDecoderStateMachine> mDecoderStateMachine;

  
  nsAutoPtr<MediaResource> mResource;

  
  
  
  ReentrantMonitor mReentrantMonitor;

  
  nsTArray<OutputMediaStream> mOutputStreams;

  
  
  
  
  
  
  PlayState mPlayState;

  
  
  
  
  
  
  PlayState mNextState;

  
  
  
  bool mResourceLoaded;

  
  
  
  
  
  bool mIgnoreProgressData;

  
  bool mInfiniteStream;
};

#endif
