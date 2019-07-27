






















































































































































































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
#include "mozilla/ReentrantMonitor.h"
#include "MediaDecoderOwner.h"
#include "MediaStreamGraph.h"
#include "AbstractMediaDecoder.h"
#include "DecodedStream.h"
#include "StateMirroring.h"
#include "StateWatching.h"
#include "necko-config.h"
#ifdef MOZ_EME
#include "mozilla/CDMProxy.h"
#endif
#include "TimeUnits.h"

class nsIStreamListener;
class nsIPrincipal;

namespace mozilla {

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

class MediaDecoder : public AbstractMediaDecoder
{
public:
  struct SeekResolveValue {
    SeekResolveValue(bool aAtEnd, MediaDecoderEventVisibility aEventVisibility)
      : mAtEnd(aAtEnd), mEventVisibility(aEventVisibility) {}
    bool mAtEnd;
    MediaDecoderEventVisibility mEventVisibility;
  };

  typedef MediaPromise<SeekResolveValue, bool ,  true> SeekPromise;

  NS_DECL_THREADSAFE_ISUPPORTS

  
  enum PlayState {
    PLAY_STATE_START,
    PLAY_STATE_LOADING,
    PLAY_STATE_PAUSED,
    PLAY_STATE_PLAYING,
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
    MOZ_ASSERT(NS_IsMainThread());
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

  
  
  
  
  
  

  
  
  
  virtual void AddOutputStream(ProcessedMediaStream* aStream, bool aFinishWhenEnded);

  
  virtual double GetDuration();

  AbstractCanonical<media::NullableTimeUnit>* CanonicalDurationOrNull() override;

  
  
  
  
  
  
  
  
  virtual void SetInfinite(bool aInfinite);

  
  virtual bool IsInfinite();

  
  
  
  
  virtual void NotifySuspendedStatusChanged();

  
  
  virtual void NotifyBytesDownloaded();

  
  
  
  virtual void NotifyDownloadEnded(nsresult aStatus);

  
  
  virtual void NotifyDataArrived(uint32_t aLength, int64_t aOffset,
                                 bool aThrottleUpdates) override;

  
  
  virtual void NotifyPrincipalChanged();

  
  
  
  void NotifyBytesConsumed(int64_t aBytes, int64_t aOffset) final override;

  
  
  virtual bool IsSeeking() const;

  
  
  
  virtual bool IsEndedOrShutdown() const;

protected:
  
  
  
  
  
  
  
  
  void UpdateEstimatedMediaDuration(int64_t aDuration) override;
public:

  
  virtual void SetMediaSeekable(bool aMediaSeekable) override;

  
  
  virtual bool IsMediaSeekable() final override;
  
  
  virtual bool IsTransportSeekable() override;

  
  virtual media::TimeIntervals GetSeekable();

  
  
  virtual void SetFragmentEndTime(double aTime);

  
  void Invalidate();
  void InvalidateWithFlags(uint32_t aFlags);

  
  
  
  
  virtual void Suspend();

  
  
  
  
  
  
  
  virtual void Resume(bool aForceBuffering);

  
  
  
  
  
  void SetLoadInBackground(bool aLoadInBackground);

  
  MediaDecoderOwner* GetMediaOwner() const;

  bool OnStateMachineTaskQueue() const override;

  bool OnDecodeTaskQueue() const override;

  MediaDecoderStateMachine* GetStateMachine() { return mDecoderStateMachine; }
  void SetStateMachine(MediaDecoderStateMachine* aStateMachine);

  
  
  ReentrantMonitor& GetReentrantMonitor() override;

  
  bool IsShutdown() const final override;

  
  
  virtual media::TimeIntervals GetBuffered();

  
  
  size_t SizeOfVideoQueue();
  size_t SizeOfAudioQueue();

  VideoFrameContainer* GetVideoFrameContainer() final override
  {
    return mVideoFrameContainer;
  }
  layers::ImageContainer* GetImageContainer() override;

  
  
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

  




  
  
  
  virtual void ChangeState(PlayState aState);

  
  
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

  void OnSeekRejected()
  {
    MOZ_ASSERT(NS_IsMainThread());
    mSeekRequest.Complete();
    mLogicallySeeking = false;
  }
  void OnSeekResolved(SeekResolveValue aVal);

  
  
  void SeekingStarted(MediaDecoderEventVisibility aEventVisibility = MediaDecoderEventVisibility::Observable);

  void UpdateLogicalPosition(MediaDecoderEventVisibility aEventVisibility);
  void UpdateLogicalPosition()
  {
    MOZ_ASSERT(NS_IsMainThread());
    UpdateLogicalPosition(MediaDecoderEventVisibility::Observable);
  }

  
  
  int64_t GetDownloadPosition();

  
  
  void UpdatePlaybackOffset(int64_t aOffset);

  
  MediaDecoderStateMachine* GetStateMachine() const;

  
  virtual void BreakCycles();

  
  virtual void DecodeError();

  
  void UpdateSameOriginStatus(bool aSameOrigin);

  MediaDecoderOwner* GetOwner() override;

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

  
  
  nsresult ScheduleStateMachine();

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

  void UpdateReadyState()
  {
    if (mOwner) {
      mOwner->UpdateReadyState();
    }
  }

  virtual MediaDecoderOwner::NextFrameStatus NextFrameStatus() { return mNextFrameStatus; }

protected:
  virtual ~MediaDecoder();
  void SetStateMachineParameters();

  static void DormantTimerExpired(nsITimer *aTimer, void *aClosure);

  
  void StartDormantTimer();

  
  void CancelDormantTimer();

  
  bool IsEnded() const;

  
  
  void DurationChanged();

  
  WatchManager<MediaDecoder> mWatchManager;

  
  Mirror<media::TimeIntervals> mBuffered;

  
  Mirror<bool> mStateMachineIsShutdown;

  
  virtual void ShutdownBitChanged() {}

  
  Mirror<MediaDecoderOwner::NextFrameStatus> mNextFrameStatus;

  



  
  bool mDormantSupported;

  
  
  
  
  int64_t mDecoderPosition;
  
  
  
  
  int64_t mPlaybackPosition;

  
  
  
  
  
  double mLogicalPosition;

  
  
  
  
  Mirror<int64_t> mCurrentPosition;

  
  
  virtual int64_t CurrentPosition() { return mCurrentPosition; }

  
  Canonical<double> mVolume;
public:
  AbstractCanonical<double>* CanonicalVolume() { return &mVolume; }
protected:

  
  Canonical<double> mPlaybackRate;
public:
  AbstractCanonical<double>* CanonicalPlaybackRate() { return &mPlaybackRate; }
protected:

  Canonical<bool> mPreservesPitch;
public:
  AbstractCanonical<bool>* CanonicalPreservesPitch() { return &mPreservesPitch; }
protected:

  
  double mDuration;

  
  Mirror<media::NullableTimeUnit> mStateMachineDuration;

  
  bool mMediaSeekable;

  
  
  bool mSameOriginMedia;

  



  
  nsRefPtr<MediaResource> mResource;

private:
  
  
  
  
  
  
  
  nsRefPtr<MediaDecoderStateMachine> mDecoderStateMachine;

  
  
  
  ReentrantMonitor mReentrantMonitor;

#ifdef MOZ_EME
  nsRefPtr<CDMProxy> mProxy;
#endif

  
  
  
  
  
  
  Canonical<media::NullableTimeUnit> mEstimatedDuration;
public:
  AbstractCanonical<media::NullableTimeUnit>* CanonicalEstimatedDuration() { return &mEstimatedDuration; }
protected:

  
  
  Canonical<Maybe<double>> mExplicitDuration;
  double ExplicitDuration() { return mExplicitDuration.Ref().ref(); }
  void SetExplicitDuration(double aValue)
  {
    mExplicitDuration.Set(Some(aValue));

    
    
    DurationChanged();
  }

public:
  AbstractCanonical<Maybe<double>>* CanonicalExplicitDuration() { return &mExplicitDuration; }
protected:

  
  
  
  
  
  
  Canonical<PlayState> mPlayState;

  
  
  
  
  
  Canonical<PlayState> mNextState;

  
  Canonical<bool> mLogicallySeeking;
public:
  AbstractCanonical<PlayState>* CanonicalPlayState() { return &mPlayState; }
  AbstractCanonical<PlayState>* CanonicalNextPlayState() { return &mNextState; }
  AbstractCanonical<bool>* CanonicalLogicallySeeking() { return &mLogicallySeeking; }
protected:

  virtual void CallSeek(const SeekTarget& aTarget);

  MediaPromiseRequestHolder<SeekPromise> mSeekRequest;

  
  
  
  
  
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

  
  bool mFiredMetadataLoaded;

  
  
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
