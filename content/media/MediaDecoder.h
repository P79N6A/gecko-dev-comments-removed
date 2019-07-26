
















































































































































































#if !defined(MediaDecoder_h_)
#define MediaDecoder_h_

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
#include "AudioChannelCommon.h"
#include "AbstractMediaDecoder.h"

class nsIStreamListener;
class nsTimeRanges;
class nsIMemoryReporter;
class nsIPrincipal;
class nsITimer;

using namespace mozilla::dom;

namespace mozilla {
namespace layers {
class Image;
} 

class MediaByteRange;
class VideoFrameContainer;
class AudioStream;
class MediaDecoderStateMachine;
class MediaDecoderOwner;




static const uint32_t FRAMEBUFFER_LENGTH_PER_CHANNEL = 1024;



static const uint32_t FRAMEBUFFER_LENGTH_MIN = 512;
static const uint32_t FRAMEBUFFER_LENGTH_MAX = 16384;



#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

class MediaDecoder : public nsIObserver,
                     public AbstractMediaDecoder
{
public:
  typedef mozilla::layers::Image Image;

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

  MediaDecoder();
  virtual ~MediaDecoder();

  
  
  virtual MediaDecoder* Clone() = 0;
  
  
  virtual MediaDecoderStateMachine* CreateStateMachine() = 0;

  
  
  
  
  virtual bool Init(MediaDecoderOwner* aOwner);

  
  
  virtual void Shutdown();

  
  
  
  
  
  virtual nsresult Load(MediaResource* aResource,
                        nsIStreamListener** aListener,
                        MediaDecoder* aCloneDonor);

  
  nsresult OpenResource(MediaResource* aResource,
                        nsIStreamListener** aStreamListener);

  
  virtual void ResourceLoaded();

  
  virtual void NetworkError();

  
  
  MediaResource* GetResource() const MOZ_FINAL MOZ_OVERRIDE
  {
    return mResource;
  }

  
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  
  
  virtual double GetCurrentTime();

  
  virtual nsresult Seek(double aTime);

  
  
  
  virtual nsresult GetByteRangeForSeek(int64_t const aOffset,
                                       MediaByteRange &aByteRange) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsresult InitializeStateMachine(MediaDecoder* aCloneDonor);

  
  
  virtual nsresult Play();

  
  virtual void Pause();
  
  virtual void SetVolume(double aVolume);
  
  
  virtual void SetAudioCaptured(bool aCaptured);

  void SetPlaybackRate(double aPlaybackRate);
  void SetPreservesPitch(bool aPreservesPitch);

  
  
  
  
  
  

  struct DecodedStreamData MOZ_FINAL : public MainThreadMediaStreamListener {
    DecodedStreamData(MediaDecoder* aDecoder,
                      int64_t aInitialTime, SourceMediaStream* aStream);
    ~DecodedStreamData();

    
    
    int64_t mLastAudioPacketTime; 
    int64_t mLastAudioPacketEndTime; 
    
    int64_t mAudioFramesWritten;
    
    
    int64_t mInitialTime; 
    
    
    
    int64_t mNextVideoTime; 
    MediaDecoder* mDecoder;
    
    
    nsRefPtr<Image> mLastVideoImage;
    gfxIntSize mLastVideoImageDisplaySize;
    
    
    bool mStreamInitialized;
    bool mHaveSentFinish;
    bool mHaveSentFinishAudio;
    bool mHaveSentFinishVideo;

    
    
    const nsRefPtr<SourceMediaStream> mStream;
    
    
    bool mHaveBlockedForPlayState;

    virtual void NotifyMainThreadStateChanged() MOZ_OVERRIDE;
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

  
  
  
  virtual void AddOutputStream(ProcessedMediaStream* aStream, bool aFinishWhenEnded);

  
  virtual double GetDuration();

  
  int64_t GetMediaDuration() MOZ_FINAL MOZ_OVERRIDE;

  
  
  
  
  
  
  
  
  virtual void SetInfinite(bool aInfinite);

  
  virtual bool IsInfinite();

  
  
  
  
  virtual void NotifySuspendedStatusChanged();

  
  
  virtual void NotifyBytesDownloaded();

  
  
  
  virtual void NotifyDownloadEnded(nsresult aStatus);

  
  
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);

  
  
  virtual void NotifyPrincipalChanged();

  
  
  void NotifyBytesConsumed(int64_t aBytes) MOZ_FINAL MOZ_OVERRIDE;

  int64_t GetEndMediaTime() const MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual bool IsSeeking() const;

  
  
  virtual bool IsEnded() const;

  
  
  
  virtual void SetDuration(double aDuration);

  void SetMediaDuration(int64_t aDuration) MOZ_OVERRIDE;

  
  virtual void SetMediaSeekable(bool aMediaSeekable) MOZ_OVERRIDE;
  virtual void SetTransportSeekable(bool aTransportSeekable) MOZ_FINAL MOZ_OVERRIDE;
  
  
  virtual bool IsMediaSeekable() MOZ_FINAL MOZ_OVERRIDE;
  
  
  virtual bool IsTransportSeekable();

  
  virtual nsresult GetSeekable(nsTimeRanges* aSeekable);

  
  
  virtual void SetFragmentEndTime(double aTime);

  
  void SetMediaEndTime(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;

  
  void Invalidate();

  
  
  
  
  virtual void Suspend();

  
  
  
  
  
  
  
  virtual void Resume(bool aForceBuffering);

  
  
  
  
  
  virtual void MoveLoadsToBackground();

  
  mozilla::MediaDecoderOwner* GetMediaOwner() const;

  
  
  uint32_t GetFrameBufferLength() { return mFrameBufferLength; }

  void AudioAvailable(float* aFrameBuffer, uint32_t aFrameBufferLength, float aTime);

  
  
  void DurationChanged();

  bool OnStateMachineThread() const MOZ_OVERRIDE;

  bool OnDecodeThread() const MOZ_OVERRIDE;

  
  
  ReentrantMonitor& GetReentrantMonitor() MOZ_OVERRIDE;

  
  bool IsShutdown() const MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered);

  
  
  virtual int64_t VideoQueueMemoryInUse();
  virtual int64_t AudioQueueMemoryInUse();

  VideoFrameContainer* GetVideoFrameContainer() MOZ_FINAL MOZ_OVERRIDE
  {
    return mVideoFrameContainer;
  }
  mozilla::layers::ImageContainer* GetImageContainer() MOZ_OVERRIDE;

  
  
  virtual nsresult RequestFrameBufferLength(uint32_t aLength);

  
  
  PlayState GetState() {
    return mPlayState;
  }

  
  
  
  
  void Progress(bool aTimer);

  
  
  void FireTimeUpdate();

  
  
  
  virtual void StopProgressUpdates();

  
  
  virtual void StartProgressUpdates();

  
  
  virtual void UpdatePlaybackRate();

  
  
  virtual void NotifyPlaybackStarted() {
    GetReentrantMonitor().AssertCurrentThreadIn();
    mPlaybackStatistics.Start();
  }

  
  
  virtual void NotifyPlaybackStopped() {
    GetReentrantMonitor().AssertCurrentThreadIn();
    mPlaybackStatistics.Stop();
  }

  
  virtual double ComputePlaybackRate(bool* aReliable);

  
  
  bool CanPlayThrough();

  
  
  
  
  void UpdatePlaybackPosition(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;

  void SetAudioChannelType(AudioChannelType aType) { mAudioChannelType = aType; }
  AudioChannelType GetAudioChannelType() { return mAudioChannelType; }

  
  
  
  void QueueMetadata(int64_t aPublishTime,
                     int aChannels,
                     int aRate,
                     bool aHasAudio,
                     bool aHasVideo,
                     MetadataTags* aTags);

  




  
  
  
  void ChangeState(PlayState aState);

  
  
  void OnReadMetadataCompleted() MOZ_OVERRIDE { }

  
  
  void MetadataLoaded(int aChannels, int aRate, bool aHasAudio, bool aHasVideo, MetadataTags* aTags);

  
  
  void FirstFrameLoaded();

  
  
  virtual bool IsDataCachedToEndOfResource();

  
  
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

  
  MediaDecoderStateMachine* GetStateMachine() const;

  
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

#ifdef MOZ_WMF
  static bool IsWMFEnabled();
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

  
  
  
  
  virtual Statistics GetStatistics();

  
  
  class FrameStatistics {
  public:

    FrameStatistics() :
        mReentrantMonitor("MediaDecoder::FrameStats"),
        mParsedFrames(0),
        mDecodedFrames(0),
        mPresentedFrames(0) {}

    
    
    uint32_t GetParsedFrames() {
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mParsedFrames;
    }

    
    
    uint32_t GetDecodedFrames() {
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mDecodedFrames;
    }

    
    
    
    uint32_t GetPresentedFrames() {
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mPresentedFrames;
    }

    
    
    void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded) {
      if (aParsed == 0 && aDecoded == 0)
        return;
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      mParsedFrames += aParsed;
      mDecodedFrames += aDecoded;
    }

    
    
    void NotifyPresentedFrame() {
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      ++mPresentedFrames;
    }

  private:

    
    ReentrantMonitor mReentrantMonitor;

    
    
    uint32_t mParsedFrames;

    
    
    uint32_t mDecodedFrames;

    
    
    uint32_t mPresentedFrames;
  };

  
  FrameStatistics& GetFrameStatistics() { return mFrameStats; }

  
  
  virtual void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded) MOZ_OVERRIDE
  {
    GetFrameStatistics().NotifyDecodedFrames(aParsed, aDecoded);
  }

  



  
  
  
  
  int64_t mDecoderPosition;
  
  
  
  
  int64_t mPlaybackPosition;

  
  
  
  
  double mCurrentTime;

  
  
  double mInitialVolume;

  
  
  double mInitialPlaybackRate;
  bool mInitialPreservesPitch;

  
  
  
  
  
  double mRequestedSeekTime;

  
  
  
  int64_t mDuration;

  
  bool mInitialAudioCaptured;

  
  
  bool mTransportSeekable;

  
  bool mMediaSeekable;

  



  
  
  
  
  
  nsCOMPtr<MediaDecoderStateMachine> mDecoderStateMachine;

  
  nsRefPtr<MediaResource> mResource;

  
  
  
  
  
  
  
  
  
  
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

  
  
  
  bool mCalledResourceLoaded;

  
  
  
  
  
  bool mIgnoreProgressData;

  
  bool mInfiniteStream;

  
  
  bool mTriggerPlaybackEndedWhenSourceStreamFinishes;

protected:

  
  nsresult StartProgress();

  
  nsresult StopProgress();

  
  void PinForSeek();

  
  void UnpinForSeek();

  
  nsCOMPtr<nsITimer> mProgressTimer;

  
  
  
  MediaDecoderOwner* mOwner;

  
  FrameStatistics mFrameStats;

  nsRefPtr<VideoFrameContainer> mVideoFrameContainer;

  
  
  TimeStamp mProgressTime;

  
  
  
  
  
  TimeStamp mDataTime;

  
  
  
  MediaChannelStatistics mPlaybackStatistics;

  
  uint32_t mFrameBufferLength;

  
  
  bool mPinnedForSeek;

  
  
  
  
  bool mShuttingDown;

  
  bool mPausedForPlaybackRateNull;

  
  
  AudioChannelType mAudioChannelType;
};

} 

#endif
