






















































































































































































#if !defined(MediaDecoder_h_)
#define MediaDecoder_h_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsAutoPtr.h"
#include "nsITimer.h"
#include "MediaPromise.h"
#include "MediaResource.h"
#include "mozilla/dom/AudioChannelBinding.h"
#include "mozilla/gfx/Rect.h"
#include "mozilla/ReentrantMonitor.h"
#include "MediaDecoderOwner.h"
#include "MediaStreamGraph.h"
#include "AbstractMediaDecoder.h"
#include "StateMirroring.h"
#include "StateWatching.h"
#include "necko-config.h"
#ifdef MOZ_EME
#include "mozilla/CDMProxy.h"
#endif

class nsIStreamListener;
class nsIPrincipal;

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
    , mEventVisibility(MediaDecoderEventVisibility::Observable)
  {
  }
  SeekTarget(int64_t aTimeUsecs,
             Type aType,
             MediaDecoderEventVisibility aEventVisibility =
               MediaDecoderEventVisibility::Observable)
    : mTime(aTimeUsecs)
    , mType(aType)
    , mEventVisibility(aEventVisibility)
  {
  }
  SeekTarget(const SeekTarget& aOther)
    : mTime(aOther.mTime)
    , mType(aOther.mType)
    , mEventVisibility(aOther.mEventVisibility)
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
  MediaDecoderEventVisibility mEventVisibility;
};

class MediaDecoder : public nsIObserver,
                     public AbstractMediaDecoder
{
public:
  struct SeekResolveValue {
    SeekResolveValue(bool aAtEnd, MediaDecoderEventVisibility aEventVisibility)
      : mAtEnd(aAtEnd), mEventVisibility(aEventVisibility) {}
    bool mAtEnd;
    MediaDecoderEventVisibility mEventVisibility;
  };

  typedef MediaPromise<SeekResolveValue, bool ,  true> SeekPromise;
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

  
  static void InitStatics();

  MediaDecoder();

  
  
  virtual void ResetConnectionState();
  
  
  virtual MediaDecoder* Clone() = 0;
  
  
  virtual MediaDecoderStateMachine* CreateStateMachine() = 0;

  
  
  
  
  virtual bool Init(MediaDecoderOwner* aOwner);

  
  
  virtual void Shutdown();

  
  
  
  virtual nsresult Load(nsIStreamListener** aListener,
                        MediaDecoder* aCloneDonor);

  
  nsresult OpenResource(nsIStreamListener** aStreamListener);

  
  virtual void NetworkError();

  
  
  
  
  
  
  
  
  MediaResource* GetResource() const final override
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

  
  nsresult InitializeStateMachine(MediaDecoder* aCloneDonor);

  
  
  virtual nsresult Play();

  
  
  
  
  
  virtual void NotifyOwnerActivityChanged();

  void UpdateDormantState(bool aDormantTimeout, bool aActivity);

  
  virtual void Pause();
  
  virtual void SetVolume(double aVolume);

  virtual void NotifyWaitingForResourcesStatusChanged() override;

  virtual void SetPlaybackRate(double aPlaybackRate);
  void SetPreservesPitch(bool aPreservesPitch);

  
  
  
  void SetMinimizePrerollUntilPlaybackStarts();

  
  
  
  
  
  

  struct DecodedStreamData {
    typedef gfx::IntSize IntSize;

    DecodedStreamData(MediaDecoder* aDecoder,
                      int64_t aInitialTime, SourceMediaStream* aStream);
    ~DecodedStreamData();

    
    bool IsFinished() const {
      return mListener->IsFinishedOnMainThread();
    }

    int64_t GetClock() const {
      return mInitialTime + mListener->GetLastOutputTime();
    }

    
    
    
    int64_t mAudioFramesWritten;
    
    
    const int64_t mInitialTime; 
    
    
    
    int64_t mNextVideoTime; 
    int64_t mNextAudioTime; 
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
    
    
    bool mEOSVideoCompensation;
  };

  class DecodedStreamGraphListener : public MediaStreamListener {
  public:
    DecodedStreamGraphListener(MediaStream* aStream, DecodedStreamData* aData);
    virtual void NotifyOutput(MediaStreamGraph* aGraph, GraphTime aCurrentTime) override;
    virtual void NotifyEvent(MediaStreamGraph* aGraph,
                             MediaStreamListener::MediaStreamGraphEvent event) override;

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

  class OutputStreamListener;

  struct OutputStreamData {
    void Init(MediaDecoder* aDecoder, ProcessedMediaStream* aStream);
    ~OutputStreamData();
    nsRefPtr<ProcessedMediaStream> mStream;
    
    nsRefPtr<MediaInputPort> mPort;
    nsRefPtr<OutputStreamListener> mListener;
  };

  


  void ConnectDecodedStreamToOutputStream(OutputStreamData* aStream);
  



  void DestroyDecodedStream();
  





  void RecreateDecodedStream(int64_t aStartTimeUSecs,
                             MediaStreamGraph* aGraph = nullptr);
  



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

  
  int64_t GetMediaDuration() final override;

  
  
  
  
  
  
  
  
  virtual void SetInfinite(bool aInfinite);

  
  virtual bool IsInfinite();

  
  
  
  
  virtual void NotifySuspendedStatusChanged();

  
  
  virtual void NotifyBytesDownloaded();

  
  
  
  virtual void NotifyDownloadEnded(nsresult aStatus);

  
  
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) override;

  
  
  virtual void NotifyPrincipalChanged();

  
  
  
  void NotifyBytesConsumed(int64_t aBytes, int64_t aOffset) final override;

  
  
  virtual bool IsSeeking() const;

  
  
  
  virtual bool IsEndedOrShutdown() const;

  
  
  
  virtual void SetDuration(double aDuration);

  
  
  void SetMediaDuration(int64_t aDuration) override;
  
  
  
  
  
  
  
  
  void UpdateEstimatedMediaDuration(int64_t aDuration) override;

  
  virtual void SetMediaSeekable(bool aMediaSeekable) override;

  
  
  virtual bool IsMediaSeekable() final override;
  
  
  virtual bool IsTransportSeekable() override;

  
  virtual nsresult GetSeekable(dom::TimeRanges* aSeekable);

  
  
  virtual void SetFragmentEndTime(double aTime);

  
  void SetMediaEndTime(int64_t aTime) final override;

  
  void Invalidate();
  void InvalidateWithFlags(uint32_t aFlags);

  
  
  
  
  virtual void Suspend();

  
  
  
  
  
  
  
  virtual void Resume(bool aForceBuffering);

  
  
  
  
  
  void SetLoadInBackground(bool aLoadInBackground);

  
  MediaDecoderOwner* GetMediaOwner() const;

  
  
  void DurationChanged();

  bool OnStateMachineTaskQueue() const override;

  bool OnDecodeTaskQueue() const override;

  MediaDecoderStateMachine* GetStateMachine() { return mDecoderStateMachine; }
  void SetStateMachine(MediaDecoderStateMachine* aStateMachine);

  
  
  ReentrantMonitor& GetReentrantMonitor() override;

  
  bool IsShutdown() const final override;

  
  
  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered);

  
  
  size_t SizeOfVideoQueue();
  size_t SizeOfAudioQueue();

  VideoFrameContainer* GetVideoFrameContainer() final override
  {
    return mVideoFrameContainer;
  }
  layers::ImageContainer* GetImageContainer() override;

  
  
  PlayState GetState() {
    return mPlayState;
  }

  
  
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

  void SetAudioChannel(dom::AudioChannel aChannel) { mAudioChannel = aChannel; }
  dom::AudioChannel GetAudioChannel() { return mAudioChannel; }

  
  
  
  void QueueMetadata(int64_t aPublishTime,
                     nsAutoPtr<MediaInfo> aInfo,
                     nsAutoPtr<MetadataTags> aTags) override;

  int64_t GetSeekTime() { return mRequestedSeekTarget.mTime; }
  void ResetSeekTime() { mRequestedSeekTarget.Reset(); }

  




  
  
  
  virtual void ChangeState(PlayState aState);

  
  
  virtual void ApplyStateToStateMachine(PlayState aState);

  
  
  void OnReadMetadataCompleted() override { }

  
  
  virtual void MetadataLoaded(nsAutoPtr<MediaInfo> aInfo,
                              nsAutoPtr<MetadataTags> aTags,
                              MediaDecoderEventVisibility aEventVisibility) override;

  
  
  virtual void FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo,
                                MediaDecoderEventVisibility aEventVisibility) override;

  
  
  
  void ConstructMediaTracks();

  
  
  virtual void RemoveMediaTracks() override;

  
  
  
  
  
  virtual bool IsExpectingMoreData();

  
  
  void PlaybackEnded();

  void OnSeekRejected() { mSeekRequest.Complete(); }
  void OnSeekResolved(SeekResolveValue aVal);

  
  
  void SeekingStarted(MediaDecoderEventVisibility aEventVisibility = MediaDecoderEventVisibility::Observable);

  
  
  
  virtual void PlaybackPositionChanged(MediaDecoderEventVisibility aEventVisibility = MediaDecoderEventVisibility::Observable);

  
  
  int64_t GetDownloadPosition();

  
  
  void UpdatePlaybackOffset(int64_t aOffset);

  
  MediaDecoderStateMachine* GetStateMachine() const;

  
  virtual void BreakCycles();

  
  virtual void DecodeError();

  
  void UpdateSameOriginStatus(bool aSameOrigin);

  MediaDecoderOwner* GetOwner() override;

  
  
  
  
  
  bool IsLogicallyPlaying();

#ifdef MOZ_EME
  
  virtual nsresult SetCDMProxy(CDMProxy* aProxy) override;

  
  virtual CDMProxy* GetCDMProxy() override;
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
        mPresentedFrames(0),
        mDroppedFrames(0),
        mCorruptFrames(0) {}

    
    
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

    
    
    uint32_t GetDroppedFrames() {
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mDroppedFrames + mCorruptFrames;
    }

    uint32_t GetCorruptedFrames() {
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mCorruptFrames;
    }

    
    
    void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded,
                             uint32_t aDropped) {
      if (aParsed == 0 && aDecoded == 0 && aDropped == 0)
        return;
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      mParsedFrames += aParsed;
      mDecodedFrames += aDecoded;
      mDroppedFrames += aDropped;
    }

    
    
    void NotifyPresentedFrame() {
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      ++mPresentedFrames;
    }

    void NotifyCorruptFrame() {
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      ++mCorruptFrames;
    }

  private:

    
    ReentrantMonitor mReentrantMonitor;

    
    
    uint32_t mParsedFrames;

    
    
    uint32_t mDecodedFrames;

    
    
    uint32_t mPresentedFrames;

    uint32_t mDroppedFrames;

    uint32_t mCorruptFrames;
  };

  
  FrameStatistics& GetFrameStatistics() { return mFrameStats; }

  
  
  virtual void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded,
                                   uint32_t aDropped) override
  {
    GetFrameStatistics().NotifyDecodedFrames(aParsed, aDecoded, aDropped);
  }

  WatchTarget& ReadyStateWatchTarget() { return *mReadyStateWatchTarget; }

  virtual MediaDecoderOwner::NextFrameStatus NextFrameStatus() { return mNextFrameStatus; }

protected:
  virtual ~MediaDecoder();
  void SetStateMachineParameters();

  static void DormantTimerExpired(nsITimer *aTimer, void *aClosure);

  
  void StartDormantTimer();

  
  void CancelDormantTimer();

  
  bool IsEnded() const;

  WatcherHolder mReadyStateWatchTarget;

  
  Mirror<MediaDecoderOwner::NextFrameStatus>::Holder mNextFrameStatus;

  



  
  
  
  
  int64_t mDecoderPosition;
  
  
  
  
  int64_t mPlaybackPosition;

  
  
  
  
  double mCurrentTime;

  
  
  double mInitialVolume;

  
  
  double mInitialPlaybackRate;
  bool mInitialPreservesPitch;

  
  
  
  int64_t mDuration;

  
  bool mMediaSeekable;

  
  
  bool mSameOriginMedia;

  



  
  nsRefPtr<MediaResource> mResource;

private:
  
  
  
  
  
  
  
  nsRefPtr<MediaDecoderStateMachine> mDecoderStateMachine;

  
  
  
  ReentrantMonitor mReentrantMonitor;

#ifdef MOZ_EME
  nsRefPtr<CDMProxy> mProxy;
#endif

protected:
  
  nsTArray<OutputStreamData> mOutputStreams;
  
  
  
  
  
  nsAutoPtr<DecodedStreamData> mDecodedStream;

  
  
  
  
  
  
  Watchable<PlayState> mPlayState;

  
  
  
  
  
  
  PlayState mNextState;

  
  
  
  
  
  
  
  SeekTarget mRequestedSeekTarget;

  MediaPromiseConsumerHolder<SeekPromise> mSeekRequest;

  
  
  
  
  
  bool mIgnoreProgressData;

  
  bool mInfiniteStream;

  
  void PinForSeek();

  
  void UnpinForSeek();

  const char* PlayStateStr();

  
  
  
  MediaDecoderOwner* mOwner;

  
  FrameStatistics mFrameStats;

  nsRefPtr<VideoFrameContainer> mVideoFrameContainer;

  
  
  
  nsRefPtr<MediaChannelStatistics> mPlaybackStatistics;

  
  
  bool mPinnedForSeek;

  
  
  
  
  bool mShuttingDown;

  
  bool mPausedForPlaybackRateNull;

  
  
  dom::AudioChannel mAudioChannel;

  
  
  
  
  bool mMinimizePreroll;

  
  
  bool mMediaTracksConstructed;

  
  
  nsAutoPtr<MediaInfo> mInfo;

  
  bool mIsDormant;

  
  
  
  
  bool mWasEndedWhenEnteredDormant;

  
  const bool mIsHeuristicDormantSupported;

  
  const int mHeuristicDormantTimeout;

  
  bool mIsHeuristicDormant;

  
  nsCOMPtr<nsITimer> mDormantTimer;
};

} 

#endif
