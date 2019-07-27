
















































































































































































#if !defined(MediaDecoder_h_)
#define MediaDecoder_h_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsAutoPtr.h"
#include "MediaResource.h"
#include "mozilla/dom/AudioChannelBinding.h"
#include "mozilla/gfx/Rect.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/TimeStamp.h"
#include "MediaStreamGraph.h"
#include "AbstractMediaDecoder.h"
#include "necko-config.h"
#ifdef MOZ_EME
#include "mozilla/CDMProxy.h"
#endif

class nsIStreamListener;
class nsIPrincipal;
class nsITimer;

namespace mozilla {
namespace dom {
class TimeRanges;
}
}

namespace mozilla {
namespace layers {
class Image;
} 

class VideoFrameContainer;
class MediaDecoderStateMachine;
class MediaDecoderOwner;



#ifdef GetCurrentTime
#undef GetCurrentTime
#endif



struct SeekTarget {
  enum Type {
    Invalid,
    PrevSyncPoint,
    Accurate
  };
  SeekTarget()
    : mTime(-1.0)
    , mType(SeekTarget::Invalid)
  {
  }
  SeekTarget(int64_t aTimeUsecs, Type aType)
    : mTime(aTimeUsecs)
    , mType(aType)
  {
  }
  SeekTarget(const SeekTarget& aOther)
    : mTime(aOther.mTime)
    , mType(aOther.mType)
  {
  }
  bool IsValid() const {
    return mType != SeekTarget::Invalid;
  }
  void Reset() {
    mTime = -1;
    mType = SeekTarget::Invalid;
  }
  
  int64_t mTime;
  
  
  
  Type mType;
};

class MediaDecoder : public nsIObserver,
                     public AbstractMediaDecoder
{
public:
  class DecodedStreamGraphListener;

  NS_DECL_THREADSAFE_ISUPPORTS
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

  
  
  virtual void ResetConnectionState();
  
  
  virtual MediaDecoder* Clone() = 0;
  
  
  virtual MediaDecoderStateMachine* CreateStateMachine() = 0;

  
  
  
  
  virtual bool Init(MediaDecoderOwner* aOwner);

  
  
  virtual void Shutdown();

  
  
  
  virtual nsresult Load(nsIStreamListener** aListener,
                        MediaDecoder* aCloneDonor);

  
  nsresult OpenResource(nsIStreamListener** aStreamListener);

  
  virtual void ResourceLoaded();

  
  virtual void NetworkError();

  
  
  
  
  
  
  
  
  MediaResource* GetResource() const MOZ_FINAL MOZ_OVERRIDE
  {
    return mResource;
  }
  void SetResource(MediaResource* aResource)
  {
    NS_ASSERTION(NS_IsMainThread(), "Should only be called on main thread");
    mResource = aResource;
  }

  
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  
  
  virtual double GetCurrentTime();

  
  
  
  virtual nsresult Seek(double aTime, SeekTarget::Type aSeekType);

  
  
  
  virtual nsresult GetByteRangeForSeek(int64_t const aOffset,
                                       MediaByteRange &aByteRange) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsresult InitializeStateMachine(MediaDecoder* aCloneDonor);

  
  
  virtual nsresult Play();

  
  
  
  
  virtual void SetDormantIfNecessary(bool aDormant);

  
  virtual void Pause();
  
  virtual void SetVolume(double aVolume);
  
  
  virtual void SetAudioCaptured(bool aCaptured);

  virtual void NotifyWaitingForResourcesStatusChanged() MOZ_OVERRIDE;

  virtual void SetPlaybackRate(double aPlaybackRate);
  void SetPreservesPitch(bool aPreservesPitch);

  
  
  
  void SetMinimizePrerollUntilPlaybackStarts();

  
  
  
  
  
  

  struct DecodedStreamData {
    typedef gfx::IntSize IntSize;

    DecodedStreamData(MediaDecoder* aDecoder,
                      int64_t aInitialTime, SourceMediaStream* aStream);
    ~DecodedStreamData();

    
    int64_t GetLastOutputTime() { return mListener->GetLastOutputTime(); }
    bool IsFinished() { return mListener->IsFinishedOnMainThread(); }

    
    
    int64_t mLastAudioPacketTime; 
    int64_t mLastAudioPacketEndTime; 
    
    int64_t mAudioFramesWritten;
    
    
    int64_t mInitialTime; 
    
    
    
    int64_t mNextVideoTime; 
    MediaDecoder* mDecoder;
    
    
    nsRefPtr<layers::Image> mLastVideoImage;
    IntSize mLastVideoImageDisplaySize;
    
    
    bool mStreamInitialized;
    bool mHaveSentFinish;
    bool mHaveSentFinishAudio;
    bool mHaveSentFinishVideo;

    
    
    const nsRefPtr<SourceMediaStream> mStream;
    
    nsRefPtr<DecodedStreamGraphListener> mListener;
    
    
    bool mHaveBlockedForPlayState;
    
    
    bool mHaveBlockedForStateMachineNotPlaying;
  };

  class DecodedStreamGraphListener : public MediaStreamListener {
  public:
    DecodedStreamGraphListener(MediaStream* aStream, DecodedStreamData* aData);
    virtual void NotifyOutput(MediaStreamGraph* aGraph, GraphTime aCurrentTime) MOZ_OVERRIDE;
    virtual void NotifyEvent(MediaStreamGraph* aGraph,
                             MediaStreamListener::MediaStreamGraphEvent event) MOZ_OVERRIDE;

    void DoNotifyFinished();

    int64_t GetLastOutputTime() 
    {
      MutexAutoLock lock(mMutex);
      return mLastOutputTime;
    }
    void Forget()
    {
      NS_ASSERTION(NS_IsMainThread(), "Main thread only");
      mData = nullptr;

      MutexAutoLock lock(mMutex);
      mStream = nullptr;
    }
    bool SetFinishedOnMainThread(bool aFinished)
    {
      MutexAutoLock lock(mMutex);
      bool result = !mStreamFinishedOnMainThread;
      mStreamFinishedOnMainThread = aFinished;
      return result;
    }
    bool IsFinishedOnMainThread()
    {
      MutexAutoLock lock(mMutex);
      return mStreamFinishedOnMainThread;
    }
  private:
    
    DecodedStreamData* mData;

    Mutex mMutex;
    
    nsRefPtr<MediaStream> mStream;
    
    int64_t mLastOutputTime; 
    
    bool mStreamFinishedOnMainThread;
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
  



  void UpdateStreamBlockingForStateMachinePlaying();
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

  
  
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) MOZ_OVERRIDE;

  
  
  virtual void NotifyPrincipalChanged();

  
  
  
  void NotifyBytesConsumed(int64_t aBytes, int64_t aOffset) MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual bool IsSeeking() const;

  
  
  virtual bool IsEnded() const;

  
  
  
  virtual void SetDuration(double aDuration);

  
  
  void SetMediaDuration(int64_t aDuration) MOZ_OVERRIDE;
  
  
  
  
  
  
  
  
  void UpdateEstimatedMediaDuration(int64_t aDuration) MOZ_OVERRIDE;

  
  virtual void SetMediaSeekable(bool aMediaSeekable) MOZ_OVERRIDE;

  
  
  virtual bool IsMediaSeekable() MOZ_FINAL MOZ_OVERRIDE;
  
  
  virtual bool IsTransportSeekable();

  
  virtual nsresult GetSeekable(dom::TimeRanges* aSeekable);

  
  
  virtual void SetFragmentEndTime(double aTime);

  
  void SetMediaEndTime(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;

  
  void Invalidate();
  void InvalidateWithFlags(uint32_t aFlags);

  
  
  
  
  virtual void Suspend();

  
  
  
  
  
  
  
  virtual void Resume(bool aForceBuffering);

  
  
  
  
  
  virtual void MoveLoadsToBackground();

  
  MediaDecoderOwner* GetMediaOwner() const;

  
  
  void DurationChanged();

  bool OnStateMachineThread() const MOZ_OVERRIDE;

  bool OnDecodeThread() const MOZ_OVERRIDE;

  
  
  ReentrantMonitor& GetReentrantMonitor() MOZ_OVERRIDE;

  
  bool IsShutdown() const MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered);

  
  
  size_t SizeOfVideoQueue();
  size_t SizeOfAudioQueue();

  VideoFrameContainer* GetVideoFrameContainer() MOZ_FINAL MOZ_OVERRIDE
  {
    return mVideoFrameContainer;
  }
  layers::ImageContainer* GetImageContainer() MOZ_OVERRIDE;

  
  
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
    mPlaybackStatistics->Start();
  }

  
  
  virtual void NotifyPlaybackStopped() {
    GetReentrantMonitor().AssertCurrentThreadIn();
    mPlaybackStatistics->Stop();
  }

  
  virtual double ComputePlaybackRate(bool* aReliable);

  
  
  bool IsSameOriginMedia();

  
  
  bool CanPlayThrough();

  
  
  
  
  void UpdatePlaybackPosition(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;

  void SetAudioChannel(dom::AudioChannel aChannel) { mAudioChannel = aChannel; }
  dom::AudioChannel GetAudioChannel() { return mAudioChannel; }

  
  
  
  void QueueMetadata(int64_t aPublishTime,
                     MediaInfo* aInfo,
                     MetadataTags* aTags);

  int64_t GetSeekTime() { return mRequestedSeekTarget.mTime; }
  void ResetSeekTime() { mRequestedSeekTarget.Reset(); }

  




  
  
  
  virtual void ChangeState(PlayState aState);

  
  
  virtual void ApplyStateToStateMachine(PlayState aState);

  
  
  void OnReadMetadataCompleted() MOZ_OVERRIDE { }

  
  
  virtual void MetadataLoaded(MediaInfo* aInfo,
                              MetadataTags* aTags);

  
  
  
  void ConstructMediaTracks();

  
  
  virtual void RemoveMediaTracks() MOZ_OVERRIDE;

  
  
  void FirstFrameLoaded();

  
  
  virtual bool IsDataCachedToEndOfResource();

  
  
  void PlaybackEnded();

  
  
  void SeekingStopped();

  
  
  void SeekingStoppedAtEnd();

  
  
  void SeekingStarted();

  
  
  
  virtual void PlaybackPositionChanged();

  
  
  virtual void UpdateReadyStateForData();

  
  
  int64_t GetDownloadPosition();

  
  
  void UpdatePlaybackOffset(int64_t aOffset);

  
  MediaDecoderStateMachine* GetStateMachine() const;

  
  virtual void BreakCycles();

  
  virtual void DecodeError();

  
  void UpdateSameOriginStatus(bool aSameOrigin);

  MediaDecoderOwner* GetOwner() MOZ_OVERRIDE;

  
  
  
  
  
  bool IsLogicallyPlaying();

#ifdef MOZ_EME
  
  virtual nsresult SetCDMProxy(CDMProxy* aProxy) MOZ_OVERRIDE;

  
  virtual CDMProxy* GetCDMProxy() MOZ_OVERRIDE;
#endif

#ifdef MOZ_RAW
  static bool IsRawEnabled();
#endif

  static bool IsOggEnabled();
  static bool IsOpusEnabled();

#ifdef MOZ_WAVE
  static bool IsWaveEnabled();
#endif

#ifdef MOZ_WEBM
  static bool IsWebMEnabled();
#endif
#ifdef NECKO_PROTOCOL_rtsp
  static bool IsRtspEnabled();
#endif

#ifdef MOZ_GSTREAMER
  static bool IsGStreamerEnabled();
#endif

#ifdef MOZ_OMX_DECODER
  static bool IsOmxEnabled();
  static bool IsOmxAsyncEnabled();
#endif

#ifdef MOZ_ANDROID_OMX
  static bool IsAndroidMediaEnabled();
#endif

#ifdef MOZ_WMF
  static bool IsWMFEnabled();
#endif

#ifdef MOZ_APPLEMEDIA
  static bool IsAppleMP3Enabled();
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

protected:
  virtual ~MediaDecoder();
  void SetStateMachineParameters();

  



  
  
  
  
  int64_t mDecoderPosition;
  
  
  
  
  int64_t mPlaybackPosition;

  
  
  
  
  double mCurrentTime;

  
  
  double mInitialVolume;

  
  
  double mInitialPlaybackRate;
  bool mInitialPreservesPitch;

  
  
  
  int64_t mDuration;

  
  bool mInitialAudioCaptured;

  
  bool mMediaSeekable;

  
  
  bool mSameOriginMedia;

  



  
  
  
  
  
  nsRefPtr<MediaDecoderStateMachine> mDecoderStateMachine;

  
  nsRefPtr<MediaResource> mResource;

  
  
  
  
  
  
  
  
  
  
private:
  class RestrictedAccessMonitor
  {
  public:
    explicit RestrictedAccessMonitor(const char* aName) :
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

#ifdef MOZ_EME
  nsRefPtr<CDMProxy> mProxy;
#endif

protected:
  
  nsTArray<OutputStreamData> mOutputStreams;
  
  
  
  
  
  nsAutoPtr<DecodedStreamData> mDecodedStream;

  
  
  bool mIsDormant;

  
  
  bool mIsExitingDormant;

  
  
  
  
  
  
  PlayState mPlayState;

  
  
  
  
  
  
  PlayState mNextState;

  
  
  
  
  
  
  
  SeekTarget mRequestedSeekTarget;

  
  
  
  bool mCalledResourceLoaded;

  
  
  
  
  
  bool mIgnoreProgressData;

  
  bool mInfiniteStream;

  
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

  
  
  
  nsRefPtr<MediaChannelStatistics> mPlaybackStatistics;

  
  
  bool mPinnedForSeek;

  
  
  
  
  bool mShuttingDown;

  
  bool mPausedForPlaybackRateNull;

  
  
  dom::AudioChannel mAudioChannel;

  
  
  
  
  bool mMinimizePreroll;

  
  
  bool mMediaTracksConstructed;

  
  
  nsAutoPtr<MediaInfo> mInfo;
};

} 

#endif
