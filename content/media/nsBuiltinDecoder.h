
















































































































































































#if !defined(nsBuiltinDecoder_h_)
#define nsBuiltinDecoder_h_

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
#include "mozilla/ReentrantMonitor.h"
#include "MediaStreamGraph.h"
#include "MediaDecoderOwner.h"

class nsIStreamListener;
class nsTimeRanges;
class nsIMemoryReporter;
class nsIPrincipal;
class nsITimer;

namespace mozilla {
namespace layers {
class Image;
} 

class MediaByteRange;
class VideoFrameContainer;
class nsAudioStream;
class nsBuiltinDecoderStateMachine;
class MediaDecoderOwner;




static const uint32_t FRAMEBUFFER_LENGTH_PER_CHANNEL = 1024;



static const uint32_t FRAMEBUFFER_LENGTH_MIN = 512;
static const uint32_t FRAMEBUFFER_LENGTH_MAX = 16384;

static inline bool IsCurrentThread(nsIThread* aThread) {
  return NS_GetCurrentThread() == aThread;
}

class nsBuiltinDecoder : public nsIObserver
{
public:
  typedef mozilla::layers::Image Image;
  class DecodedStreamMainThreadListener;

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
  virtual ~nsBuiltinDecoder();

  
  
  virtual nsBuiltinDecoder* Clone() = 0;
  
  
  virtual nsBuiltinDecoderStateMachine* CreateStateMachine() = 0;

  
  
  
  
  virtual bool Init(mozilla::MediaDecoderOwner* aOwner);

  
  
  virtual void Shutdown();

  
  
  
  
  
  virtual nsresult Load(MediaResource* aResource,
                        nsIStreamListener** aListener,
                        nsBuiltinDecoder* aCloneDonor);

  
  nsresult OpenResource(MediaResource* aResource,
                        nsIStreamListener** aStreamListener);

  
  virtual void ResourceLoaded();

  
  virtual void NetworkError();

  
  
  virtual MediaResource* GetResource() { return mResource; }

  
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  
  
  virtual double GetCurrentTime();

  
  virtual nsresult Seek(double aTime);

  
  
  
  virtual nsresult GetByteRangeForSeek(int64_t const aOffset,
                                       MediaByteRange &aByteRange) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsresult InitializeStateMachine(nsBuiltinDecoder* aCloneDonor);

  
  
  virtual nsresult Play();

  
  
  
  virtual nsresult PlaybackRateChanged();

  
  virtual void Pause();
  
  virtual void SetVolume(double aVolume);
  
  
  virtual void SetAudioCaptured(bool aCaptured);

  
  
  
  
  
  

  struct DecodedStreamData {
    DecodedStreamData(nsBuiltinDecoder* aDecoder,
                      int64_t aInitialTime, SourceMediaStream* aStream);
    ~DecodedStreamData();

    
    
    int64_t mLastAudioPacketTime; 
    int64_t mLastAudioPacketEndTime; 
    
    int64_t mAudioFramesWritten;
    
    
    int64_t mInitialTime; 
    
    
    
    int64_t mNextVideoTime; 
    
    
    nsRefPtr<Image> mLastVideoImage;
    gfxIntSize mLastVideoImageDisplaySize;
    
    
    bool mStreamInitialized;
    bool mHaveSentFinish;
    bool mHaveSentFinishAudio;
    bool mHaveSentFinishVideo;

    
    
    const nsRefPtr<SourceMediaStream> mStream;
    
    
    const nsRefPtr<DecodedStreamMainThreadListener> mMainThreadListener;
    
    
    bool mHaveBlockedForPlayState;
  };
  struct OutputStreamData {
    void Init(ProcessedMediaStream* aStream, bool aFinishWhenEnded)
    {
      mStream = aStream;
      mFinishWhenEnded = aFinishWhenEnded;
    }
    nsRefPtr<ProcessedMediaStream> mStream;
    
    nsRefPtr<MediaInputPort> mPort;
    bool mFinishWhenEnded;
  };
  


  void ConnectDecodedStreamToOutputStream(OutputStreamData* aStream);
  



  void DestroyDecodedStream();
  




  void RecreateDecodedStream(int64_t aStartTimeUSecs);
  




  void NotifyDecodedStreamMainThreadStateChanged();
  nsTArray<OutputStreamData>& OutputStreams()
  {
    GetReentrantMonitor().AssertCurrentThreadIn();
    return mOutputStreams;
  }
  DecodedStreamData* GetDecodedStream()
  {
    GetReentrantMonitor().AssertCurrentThreadIn();
    return mDecodedStream;
  }
  class DecodedStreamMainThreadListener : public MainThreadMediaStreamListener {
  public:
    DecodedStreamMainThreadListener(nsBuiltinDecoder* aDecoder)
      : mDecoder(aDecoder) {}
    virtual void NotifyMainThreadStateChanged()
    {
      mDecoder->NotifyDecodedStreamMainThreadStateChanged();
    }
    nsBuiltinDecoder* mDecoder;
  };

  
  
  
  virtual void AddOutputStream(ProcessedMediaStream* aStream, bool aFinishWhenEnded);

  
  virtual double GetDuration();

  
  
  
  
  
  
  
  
  virtual void SetInfinite(bool aInfinite);

  
  virtual bool IsInfinite();

  
  
  
  
  virtual void NotifySuspendedStatusChanged();

  
  
  virtual void NotifyBytesDownloaded();

  
  
  
  virtual void NotifyDownloadEnded(nsresult aStatus);

  
  
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);

  
  
  virtual void NotifyPrincipalChanged();

  
  
  void NotifyBytesConsumed(int64_t aBytes);

  
  
  virtual bool IsSeeking() const;

  
  
  virtual bool IsEnded() const;

  
  
  
  virtual void SetDuration(double aDuration);

  
  virtual void SetSeekable(bool aSeekable);

  
  virtual bool IsSeekable();

  
  virtual nsresult GetSeekable(nsTimeRanges* aSeekable);

  
  
  virtual void SetEndTime(double aTime);

  
  void Invalidate();

  
  
  
  
  virtual void Suspend();

  
  
  
  
  
  
  
  virtual void Resume(bool aForceBuffering);

  
  
  
  
  
  virtual void MoveLoadsToBackground();

  
  mozilla::MediaDecoderOwner* GetMediaOwner() const;

  
  
  uint32_t GetFrameBufferLength() { return mFrameBufferLength; }

  void AudioAvailable(float* aFrameBuffer, uint32_t aFrameBufferLength, float aTime);

  
  
  void DurationChanged();

  virtual bool OnStateMachineThread() const;

  virtual bool OnDecodeThread() const;

  
  
  virtual ReentrantMonitor& GetReentrantMonitor();

  
  
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered);

  
  
  virtual int64_t VideoQueueMemoryInUse();
  virtual int64_t AudioQueueMemoryInUse();

  VideoFrameContainer* GetVideoFrameContainer() { return mVideoFrameContainer; }
  virtual mozilla::layers::ImageContainer* GetImageContainer();

  
  
  virtual nsresult RequestFrameBufferLength(uint32_t aLength);

  
  
  PlayState GetState() {
    return mPlayState;
  }

  
  
  
  
  void Progress(bool aTimer);

  
  
  void FireTimeUpdate();

  
  
  
  void StopProgressUpdates();

  
  
  void StartProgressUpdates();

  
  
  void UpdatePlaybackRate();

  
  double ComputePlaybackRate(bool* aReliable);

  
  
  bool CanPlayThrough();

  
  
  
  
  void UpdatePlaybackPosition(int64_t aTime);
  




  
  
  
  void ChangeState(PlayState aState);

  
  
  virtual void OnReadMetadataCompleted() { }

  
  
  void MetadataLoaded(uint32_t aChannels,
                      uint32_t aRate,
                      bool aHasAudio,
                      const MetadataTags* aTags);

  
  
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

  
  nsBuiltinDecoderStateMachine* GetStateMachine();

  
  virtual void ReleaseStateMachine();

  
  
  
  virtual void NotifyAudioAvailableListener();

  
  virtual void DecodeError();

#ifdef MOZ_RAW
  static bool IsRawEnabled();
#endif

#ifdef MOZ_OGG
  static bool IsOggEnabled();
  static bool IsOpusEnabled();
#endif

#ifdef MOZ_WAVE
  static bool IsWaveEnabled();
#endif

#ifdef MOZ_WEBM
  static bool IsWebMEnabled();
#endif

#ifdef MOZ_GSTREAMER
  static bool IsGStreamerEnabled();
#endif

#ifdef MOZ_WIDGET_GONK
  static bool IsOmxEnabled();
#endif

#ifdef MOZ_MEDIA_PLUGINS
  static bool IsMediaPluginsEnabled();
#endif

#ifdef MOZ_DASH
  static bool IsDASHEnabled();
#endif

  
  
  nsresult ScheduleStateMachineThread();

  struct Statistics {
    
    double mPlaybackRate;
    
    
    double mDownloadRate;
    
    int64_t mTotalBytes;
    
    
    int64_t mDownloadPosition;
    
    
    int64_t mDecoderPosition;
    
    int64_t mPlaybackPosition;
    
    
    
    bool mDownloadRateReliable;
    
    
    
    bool mPlaybackRateReliable;
  };

  
  
  
  
  Statistics GetStatistics();

  
  
  class FrameStatistics {
  public:

    FrameStatistics() :
        mReentrantMonitor("nsBuiltinDecoder::FrameStats"),
        mParsedFrames(0),
        mDecodedFrames(0),
        mPresentedFrames(0) {}

    
    
    uint32_t GetParsedFrames() {
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mParsedFrames;
    }

    
    
    uint32_t GetDecodedFrames() {
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mDecodedFrames;
    }

    
    
    
    uint32_t GetPresentedFrames() {
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mPresentedFrames;
    }

    
    
    void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded) {
      if (aParsed == 0 && aDecoded == 0)
        return;
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      mParsedFrames += aParsed;
      mDecodedFrames += aDecoded;
    }

    
    
    void NotifyPresentedFrame() {
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      ++mPresentedFrames;
    }

  private:

    
    ReentrantMonitor mReentrantMonitor;

    
    
    uint32_t mParsedFrames;

    
    
    uint32_t mDecodedFrames;

    
    
    uint32_t mPresentedFrames;
  };

  
  
  
  class AutoNotifyDecoded {
  public:
    AutoNotifyDecoded(nsBuiltinDecoder* aDecoder, uint32_t& aParsed, uint32_t& aDecoded)
      : mDecoder(aDecoder), mParsed(aParsed), mDecoded(aDecoded) {}
    ~AutoNotifyDecoded() {
      mDecoder->GetFrameStatistics().NotifyDecodedFrames(mParsed, mDecoded);
    }
  private:
    nsBuiltinDecoder* mDecoder;
    uint32_t& mParsed;
    uint32_t& mDecoded;
  };

  
  FrameStatistics& GetFrameStatistics() { return mFrameStats; }

  



  
  
  
  
  int64_t mDecoderPosition;
  
  
  
  
  int64_t mPlaybackPosition;
  
  
  
  MediaChannelStatistics mPlaybackStatistics;

  
  
  
  
  double mCurrentTime;

  
  
  double mInitialVolume;

  
  
  
  
  
  double mRequestedSeekTime;

  
  
  
  int64_t mDuration;

  
  bool mInitialAudioCaptured;

  
  
  bool mSeekable;

  



  
  
  
  
  
  nsCOMPtr<nsBuiltinDecoderStateMachine> mDecoderStateMachine;

  
  nsAutoPtr<MediaResource> mResource;

  
  
  
  
  
  
  
  
  
  
private:
  class RestrictedAccessMonitor
  {
  public:
    RestrictedAccessMonitor(const char* aName) :
      mReentrantMonitor(aName)
    {
      MOZ_COUNT_CTOR(RestrictedAccessMonitor);
    }
    ~RestrictedAccessMonitor()
    {
      MOZ_COUNT_DTOR(RestrictedAccessMonitor);
    }

    
    ReentrantMonitor& GetReentrantMonitor() {
      return mReentrantMonitor;
    }
  private:
    ReentrantMonitor mReentrantMonitor;
  };

  
  RestrictedAccessMonitor mReentrantMonitor;

public:
  
  nsTArray<OutputStreamData> mOutputStreams;
  
  
  
  
  
  nsAutoPtr<DecodedStreamData> mDecodedStream;

  
  
  
  
  
  
  PlayState mPlayState;

  
  
  
  
  
  
  PlayState mNextState;

  
  
  
  bool mResourceLoaded;

  
  
  
  
  
  bool mIgnoreProgressData;

  
  bool mInfiniteStream;

  
  
  bool mTriggerPlaybackEndedWhenSourceStreamFinishes;

protected:

  
  nsresult StartProgress();

  
  nsresult StopProgress();

  
  void PinForSeek();

  
  void UnpinForSeek();

  
  nsCOMPtr<nsITimer> mProgressTimer;

  
  
  
  mozilla::MediaDecoderOwner* mOwner;

  
  FrameStatistics mFrameStats;

  nsRefPtr<VideoFrameContainer> mVideoFrameContainer;

  
  
  TimeStamp mProgressTime;

  
  
  
  
  
  TimeStamp mDataTime;

  
  uint32_t mFrameBufferLength;

  
  
  bool mPinnedForSeek;

  
  
  
  
  bool mShuttingDown;
};

} 

#endif
