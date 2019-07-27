




#ifndef mozilla_dom_HTMLMediaElement_h
#define mozilla_dom_HTMLMediaElement_h

#include "nsIDOMHTMLMediaElement.h"
#include "nsGenericHTMLElement.h"
#include "MediaDecoderOwner.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIObserver.h"
#include "mozilla/CORSMode.h"
#include "DOMMediaStream.h"
#include "AudioChannelCommon.h"
#include "DecoderTraits.h"
#include "nsIAudioChannelAgent.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/TextTrackManager.h"
#include "MediaDecoder.h"
#ifdef MOZ_EME
#include "mozilla/dom/MediaKeys.h"
#endif
#include "StateWatching.h"
#include "nsGkAtoms.h"


#ifdef CurrentTime
#undef CurrentTime
#endif

#include "mozilla/dom/HTMLMediaElementBinding.h"




class nsIChannel;
class nsIHttpChannel;
class nsILoadGroup;

typedef uint16_t nsMediaNetworkState;
typedef uint16_t nsMediaReadyState;

namespace mozilla {
class ErrorResult;
class MediaResource;
class MediaDecoder;
class VideoFrameContainer;
namespace dom {
class MediaKeys;
class TextTrack;
class TimeRanges;
class WakeLock;
class MediaTrack;
}
}

class nsITimer;
class nsRange;
class nsIRunnable;

namespace mozilla {
namespace dom {


#define TIMEUPDATE_MS 250

class MediaError;
class MediaSource;
class TextTrackList;
class AudioTrackList;
class VideoTrackList;

class HTMLMediaElement : public nsGenericHTMLElement,
                         public nsIDOMHTMLMediaElement,
                         public nsIObserver,
                         public MediaDecoderOwner,
                         public nsIAudioChannelAgentCallback
{
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::VideoFrameContainer VideoFrameContainer;
  typedef mozilla::MediaStream MediaStream;
  typedef mozilla::MediaResource MediaResource;
  typedef mozilla::MediaDecoderOwner MediaDecoderOwner;
  typedef mozilla::MetadataTags MetadataTags;

  CORSMode GetCORSMode() {
    return mCORSMode;
  }

  explicit HTMLMediaElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  







  nsresult LoadWithChannel(nsIChannel *aChannel, nsIStreamListener **aListener);

  
  NS_DECL_NSIDOMHTMLMEDIAELEMENT

  NS_DECL_NSIOBSERVER

  NS_DECL_NSIAUDIOCHANNELAGENTCALLBACK

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLMediaElement,
                                           nsGenericHTMLElement)

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) override;
  
  
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) override;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttr,
                             bool aNotify) override;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;
  virtual void DoneCreatingElement() override;

  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable,
                               int32_t *aTabIndex) override;
  virtual int32_t TabIndexDefault() override;

  



  virtual void NotifyOwnerDocumentActivityChanged();

  
  
  
  virtual void MetadataLoaded(const MediaInfo* aInfo,
                              nsAutoPtr<const MetadataTags> aTags) final override;

  
  
  virtual void FirstFrameLoaded() final override;

  
  
  virtual void NetworkError() final override;

  
  
  virtual void DecodeError() final override;

  
  
  virtual void LoadAborted() final override;

  
  
  virtual void PlaybackEnded() final override;

  
  
  virtual void SeekStarted() final override;

  
  
  virtual void SeekCompleted() final override;

  
  
  
  virtual void DownloadSuspended() final override;

  
  
  
  
  
  
  
  virtual void DownloadResumed(bool aForceNetworkLoading = false) final override;

  
  virtual void DownloadProgressed() final override;

  
  
  virtual void NotifySuspendedByCache(bool aIsSuspended) final override;

  virtual bool IsActive() final override;

  virtual bool IsHidden() final override;

  
  
  virtual VideoFrameContainer* GetVideoFrameContainer() final override;
  layers::ImageContainer* GetImageContainer();

  
  virtual nsresult DispatchAsyncEvent(const nsAString& aName) final override;

  
  nsresult DispatchPendingMediaEvents();

  
  bool CanActivateAutoplay();

  
  
  
  
  void CheckAutoplayDataReady();

  
  bool ShouldCheckAllowOrigin();

  
  
  bool IsCORSSameOrigin();

  
  
  bool IsPotentiallyPlaying() const;

  
  
  bool IsPlaybackEnded() const;

  
  
  
  already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  
  virtual void NotifyDecoderPrincipalChanged() final override;

  
  
  void UpdateMediaSize(const nsIntSize& aSize);
  
  
  void UpdateInitialMediaSize(const nsIntSize& aSize);

  
  
  static CanPlayStatus GetCanPlay(const nsAString& aType);

  



  void NotifyAddedSource();

  



  void NotifyLoadError();

  void NotifyMediaTrackEnabled(MediaTrack* aTrack);

  


  void NotifyMediaStreamTracksAvailable(DOMMediaStream* aStream);

  virtual bool IsNodeOfType(uint32_t aFlags) const override;

  




  uint32_t GetCurrentLoadID() { return mCurrentLoadID; }

  



  already_AddRefed<nsILoadGroup> GetDocumentLoadGroup();

  



  bool GetPlayedOrSeeked() const { return mHasPlayedOrSeeked; }

  nsresult CopyInnerTo(Element* aDest);

  



  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel) = 0;

  



  void SetRequestHeaders(nsIHttpChannel* aChannel);

  




  void RunInStableState(nsIRunnable* aRunnable);

  





  virtual void FireTimeUpdate(bool aPeriodic) final override;

  MediaStream* GetSrcMediaStream() const
  {
    NS_ASSERTION(mSrcStream, "Don't call this when not playing a stream");
    if (!mPlaybackStream) {
      
      return mSrcStream->GetStream();
    }
    return mPlaybackStream->GetStream();
  }

  

  MediaError* GetError() const
  {
    return mError;
  }

  
  void SetSrc(const nsAString& aSrc, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::src, aSrc, aRv);
  }

  

  void GetCrossOrigin(nsAString& aResult)
  {
    
    
    
    GetEnumAttr(nsGkAtoms::crossorigin, nullptr, aResult);
  }
  void SetCrossOrigin(const nsAString& aCrossOrigin, ErrorResult& aError)
  {
    SetOrRemoveNullableStringAttr(nsGkAtoms::crossorigin, aCrossOrigin, aError);
  }

  uint16_t NetworkState() const
  {
    return mNetworkState;
  }

  
  
  virtual void ResetConnectionState() final override;

  
  void SetPreload(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::preload, aValue, aRv);
  }

  already_AddRefed<TimeRanges> Buffered() const;

  

  

  uint16_t ReadyState() const
  {
    return mReadyState;
  }

  bool Seeking() const;

  double CurrentTime() const;

  void SetCurrentTime(double aCurrentTime, ErrorResult& aRv);

  void FastSeek(double aTime, ErrorResult& aRv);

  double Duration() const;

  bool HasAudio() const
  {
    return mMediaInfo.HasAudio();
  }

  bool HasVideo() const
  {
    return mMediaInfo.HasVideo();
  }

  bool IsEncrypted() const
  {
    return mIsEncrypted;
  }

  bool Paused() const
  {
    return mPaused;
  }

  double DefaultPlaybackRate() const
  {
    return mDefaultPlaybackRate;
  }

  void SetDefaultPlaybackRate(double aDefaultPlaybackRate, ErrorResult& aRv);

  double PlaybackRate() const
  {
    return mPlaybackRate;
  }

  void SetPlaybackRate(double aPlaybackRate, ErrorResult& aRv);

  already_AddRefed<TimeRanges> Played();

  already_AddRefed<TimeRanges> Seekable() const;

  bool Ended();

  bool Autoplay() const
  {
    return GetBoolAttr(nsGkAtoms::autoplay);
  }

  void SetAutoplay(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::autoplay, aValue, aRv);
  }

  bool Loop() const
  {
    return GetBoolAttr(nsGkAtoms::loop);
  }

  void SetLoop(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::loop, aValue, aRv);
  }

  void Play(ErrorResult& aRv);

  void Pause(ErrorResult& aRv);

  bool Controls() const
  {
    return GetBoolAttr(nsGkAtoms::controls);
  }

  void SetControls(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::controls, aValue, aRv);
  }

  double Volume() const
  {
    return mVolume;
  }

  void SetVolume(double aVolume, ErrorResult& aRv);

  bool Muted() const
  {
    return mMuted & MUTED_BY_CONTENT;
  }

  

  bool DefaultMuted() const
  {
    return GetBoolAttr(nsGkAtoms::muted);
  }

  void SetDefaultMuted(bool aMuted, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::muted, aMuted, aRv);
  }

  bool MozMediaStatisticsShowing() const
  {
    return mStatsShowing;
  }

  void SetMozMediaStatisticsShowing(bool aShow)
  {
    mStatsShowing = aShow;
  }

  bool MozAllowCasting() const
  {
    return mAllowCasting;
  }

  void SetMozAllowCasting(bool aShow)
  {
    mAllowCasting = aShow;
  }

  bool MozIsCasting() const
  {
    return mIsCasting;
  }

  void SetMozIsCasting(bool aShow)
  {
    mIsCasting = aShow;
  }

  already_AddRefed<MediaSource> GetMozMediaSourceObject() const;
  already_AddRefed<DOMMediaStream> GetMozSrcObject() const;

  void SetMozSrcObject(DOMMediaStream& aValue);
  void SetMozSrcObject(DOMMediaStream* aValue);

  bool MozPreservesPitch() const
  {
    return mPreservesPitch;
  }

  

#ifdef MOZ_EME
  MediaKeys* GetMediaKeys() const;

  already_AddRefed<Promise> SetMediaKeys(MediaKeys* mediaKeys,
                                         ErrorResult& aRv);

  mozilla::dom::EventHandlerNonNull* GetOnencrypted();
  void SetOnencrypted(mozilla::dom::EventHandlerNonNull* listener);

  void DispatchEncrypted(const nsTArray<uint8_t>& aInitData,
                         const nsAString& aInitDataType) override;

  bool IsEventAttributeName(nsIAtom* aName) override;

  
  
  already_AddRefed<nsIPrincipal> GetTopLevelPrincipal();

  bool ContainsRestrictedContent();
#endif 

  bool MozAutoplayEnabled() const
  {
    return mAutoplayEnabled;
  }

  already_AddRefed<DOMMediaStream> MozCaptureStream(ErrorResult& aRv,
                                                    MediaStreamGraph* aGraph = nullptr);

  already_AddRefed<DOMMediaStream> MozCaptureStreamUntilEnded(ErrorResult& aRv,
                                                              MediaStreamGraph* aGraph = nullptr);

  bool MozAudioCaptured() const
  {
    return mAudioCaptured;
  }

  void MozGetMetadata(JSContext* aCx, JS::MutableHandle<JSObject*> aResult,
                      ErrorResult& aRv);

  double MozFragmentEnd();

  AudioChannel MozAudioChannelType() const
  {
    return mAudioChannel;
  }

  void SetMozAudioChannelType(AudioChannel aValue, ErrorResult& aRv);

  AudioTrackList* AudioTracks();

  VideoTrackList* VideoTracks();

  TextTrackList* GetTextTracks();

  already_AddRefed<TextTrack> AddTextTrack(TextTrackKind aKind,
                                           const nsAString& aLabel,
                                           const nsAString& aLanguage);

  void AddTextTrack(TextTrack* aTextTrack) {
    GetOrCreateTextTrackManager()->AddTextTrack(aTextTrack);
  }

  void RemoveTextTrack(TextTrack* aTextTrack, bool aPendingListOnly = false) {
    if (mTextTrackManager) {
      mTextTrackManager->RemoveTextTrack(aTextTrack, aPendingListOnly);
    }
  }

  void AddCue(TextTrackCue& aCue) {
    if (mTextTrackManager) {
      mTextTrackManager->AddCue(aCue);
    }
  }

  


  nsresult FinishDecoderSetup(MediaDecoder* aDecoder, MediaResource* aStream) {
    return FinishDecoderSetup(aDecoder, aStream, nullptr, nullptr);
  }

  
  
  
  bool IsBeingDestroyed();

protected:
  virtual ~HTMLMediaElement();

  class MediaLoadListener;
  class MediaStreamTracksAvailableCallback;
  class StreamListener;
  class StreamSizeListener;

  MediaDecoderOwner::NextFrameStatus NextFrameStatus();

  void SetDecoder(MediaDecoder* aDecoder)
  {
    if (mDecoder) {
      mReadyStateUpdater->Unwatch(mDecoder->ReadyStateWatchTarget());
    }
    mDecoder = aDecoder;
    if (mDecoder) {
      mReadyStateUpdater->Watch(mDecoder->ReadyStateWatchTarget());
    }
  }

  virtual void GetItemValueText(DOMString& text) override;
  virtual void SetItemValueText(const nsAString& text) override;

  class WakeLockBoolWrapper {
  public:
    explicit WakeLockBoolWrapper(bool val = false)
      : mValue(val), mCanPlay(true), mOuter(nullptr) {}

    ~WakeLockBoolWrapper();

    void SetOuter(HTMLMediaElement* outer) { mOuter = outer; }
    void SetCanPlay(bool aCanPlay);

    MOZ_IMPLICIT operator bool() const { return mValue; }

    WakeLockBoolWrapper& operator=(bool val);

    bool operator !() const { return !mValue; }

    static void TimerCallback(nsITimer* aTimer, void* aClosure);

  private:
    void UpdateWakeLock();

    bool mValue;
    bool mCanPlay;
    HTMLMediaElement* mOuter;
    nsCOMPtr<nsITimer> mTimer;
  };

  


  void ChangeReadyState(nsMediaReadyState aState);

  



  void ChangeNetworkState(nsMediaNetworkState aState);

  



  virtual void WakeLockCreate();
  virtual void WakeLockRelease();
  nsRefPtr<WakeLock> mWakeLock;

  





  void ReportLoadError(const char* aMsg,
                       const char16_t** aParams = nullptr,
                       uint32_t aParamCount = 0);

  




  void SetPlayedOrSeeked(bool aValue);

  


  void SetupSrcMediaStreamPlayback(DOMMediaStream* aStream);
  


  void EndSrcMediaStreamPlayback();

  






  already_AddRefed<DOMMediaStream> CaptureStreamInternal(bool aFinishWhenEnded,
                                                         MediaStreamGraph* aGraph = nullptr);

  




  nsresult InitializeDecoderAsClone(MediaDecoder* aOriginal);

  




  nsresult InitializeDecoderForChannel(nsIChannel *aChannel,
                                       nsIStreamListener **aListener);

  



  nsresult FinishDecoderSetup(MediaDecoder* aDecoder,
                              MediaResource* aStream,
                              nsIStreamListener **aListener,
                              MediaDecoder* aCloneDonor);

  


  void AddMediaElementToURITable();
  


  void RemoveMediaElementFromURITable();
  



  HTMLMediaElement* LookupMediaElementURITable(nsIURI* aURI);

  


  void ShutdownDecoder();
  




  void AbortExistingLoads();

  




  void NoSupportedMediaSourceError();

  




  void LoadFromSourceChildren();

  



  void QueueLoadFromSourceTask();

  


  void SelectResource();

  



  void SelectResourceWrapper();

  



  void QueueSelectResourceTask();

  



  void ResetState();

  


  nsresult LoadResource();

  




  nsIContent* GetNextSource();

  



  void ChangeDelayLoadStatus(bool aDelay);

  


  void StopSuspendingAfterFirstFrame();

  



  nsresult OnChannelRedirect(nsIChannel *aChannel,
                             nsIChannel *aNewChannel,
                             uint32_t aFlags);

  


  void AddRemoveSelfReference();

  


  void DoRemoveSelfReference();

  


  enum PreloadAttrValue {
    PRELOAD_ATTR_EMPTY,    
    PRELOAD_ATTR_NONE,     
    PRELOAD_ATTR_METADATA, 
    PRELOAD_ATTR_AUTO      
  };

  



  enum PreloadAction {
    PRELOAD_UNDEFINED = 0, 
    PRELOAD_NONE = 1,      
    PRELOAD_METADATA = 2,  
    PRELOAD_ENOUGH = 3     
                           
  };

  




  void SuspendLoad();

  




  void ResumeLoad(PreloadAction aAction);

  






  void UpdatePreloadAction();

  





  void CheckProgress(bool aHaveNewProgress);
  static void ProgressTimerCallback(nsITimer* aTimer, void* aClosure);
  


  void StartProgressTimer();
  


  void StartProgress();
  


  void StopProgress();

  


  void DispatchAsyncSourceError(nsIContent* aSourceElement);

  



  void Error(uint16_t aErrorCode);

  


  void GetCurrentSpec(nsCString& aString);

  


  void ProcessMediaFragmentURI();

  


  void SetMutedInternal(uint32_t aMuted);
  



  void SetVolumeInternal();

  




  void SuspendOrResumeElement(bool aPauseElement, bool aSuspendEvents);

  
  
  virtual HTMLMediaElement* GetMediaElement() final override
  {
    return this;
  }

  
  virtual bool GetPaused() final override
  {
    bool isPaused = false;
    GetPaused(&isPaused);
    return isPaused;
  }

#ifdef MOZ_EME
  void ReportEMETelemetry();
#endif
  void ReportMSETelemetry();

  
  bool CheckAudioChannelPermissions(const nsAString& aType);

  
  nsresult UpdateChannelMuteState(mozilla::dom::AudioChannelState aCanPlay);

  
  
  
  
  void Seek(double aTime, SeekTarget::Type aSeekType, ErrorResult& aRv);

  
  void UpdateAudioChannelPlayingState();

  
  
  
  void PopulatePendingTextTrackList();

  
  
  TextTrackManager* GetOrCreateTextTrackManager();

  
  void UpdateReadyStateInternal();

  class nsAsyncEventRunner;
  using nsGenericHTMLElement::DispatchEvent;
  
  nsresult DispatchEvent(const nsAString& aName);

  
  
  nsRefPtr<MediaDecoder> mDecoder;

  
  
  nsRefPtr<VideoFrameContainer> mVideoFrameContainer;

  
  
  nsRefPtr<DOMMediaStream> mSrcAttrStream;

  
  
  
  nsRefPtr<DOMMediaStream> mSrcStream;

  
  nsRefPtr<MediaInputPort> mPlaybackStreamInputPort;

  
  
  
  nsRefPtr<DOMMediaStream> mPlaybackStream;

  
  
  struct OutputMediaStream {
    nsRefPtr<DOMMediaStream> mStream;
    bool mFinishWhenEnded;
  };
  nsTArray<OutputMediaStream> mOutputStreams;

  
  
  nsRefPtr<StreamListener> mMediaStreamListener;
  
  
  nsRefPtr<StreamSizeListener> mMediaStreamSizeListener;

  
  nsRefPtr<MediaSource> mMediaSource;

  
  
  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  nsRefPtr<MediaError> mError;

  
  
  
  uint32_t mCurrentLoadID;

  
  
  nsRefPtr<nsRange> mSourcePointer;

  
  
  nsCOMPtr<nsIDocument> mLoadBlockedDoc;

  
  
  nsTArray<nsString> mPendingEvents;

  
  
  nsMediaNetworkState mNetworkState;
  Watchable<nsMediaReadyState> mReadyState;

  WatcherHolder mReadyStateUpdater;

  enum LoadAlgorithmState {
    
    
    NOT_WAITING,
    
    
    
    
    WAITING_FOR_SOURCE
  };

  
  
  
  LoadAlgorithmState mLoadWaitStatus;

  
  double mVolume;

  
  
  static PLDHashOperator BuildObjectFromTags(nsCStringHashKey::KeyType aKey,
                                             nsCString aValue,
                                             void* aUserArg);
  nsAutoPtr<const MetadataTags> mTags;

  
  
  
  
  
  nsCOMPtr<nsIURI> mLoadingSrc;

  
  
  
  PreloadAction mPreloadAction;

  
  
  TimeStamp mTimeUpdateTime;

  
  
  TimeStamp mProgressTime;

  
  
  
  
  TimeStamp mDataTime;

  
  
  double mLastCurrentTime;

  
  
  
  double mFragmentStart;

  
  
  
  double mFragmentEnd;

  
  
  double mDefaultPlaybackRate;

  
  
  
  
  double mPlaybackRate;

  
  
  bool mPreservesPitch;

  
  
  nsCOMPtr<nsIContent> mSourceLoadCandidate;

  
  nsRefPtr<TimeRanges> mPlayed;

  
  nsCOMPtr<nsITimer> mProgressTimer;

#ifdef MOZ_EME
  
  nsRefPtr<MediaKeys> mMediaKeys;
#endif

  
  double mCurrentPlayRangeStart;

  
  
  bool mBegun;

  
  bool mLoadedDataFired;

  
  
  
  
  
  
  
  
  
  bool mAutoplaying;

  
  
  bool mAutoplayEnabled;

  
  
  WakeLockBoolWrapper mPaused;

  enum MutedReasons {
    MUTED_BY_CONTENT               = 0x01,
    MUTED_BY_INVALID_PLAYBACK_RATE = 0x02,
    MUTED_BY_AUDIO_CHANNEL         = 0x04,
    MUTED_BY_AUDIO_TRACK           = 0x08
  };

  uint32_t mMuted;

  
  
  bool mStatsShowing;

  
  
  
  
  bool mAllowCasting;
  
  bool mIsCasting;

  
  bool mAudioCaptured;

  
  
  
  
  bool mPlayingBeforeSeek;

  
  
  bool mPlayingThroughTheAudioChannelBeforeSeek;

  
  
  bool mPausedForInactiveDocumentOrChannel;

  
  bool mEventDeliveryPaused;

  
  
  bool mWaitingFired;

  
  bool mIsRunningLoadMethod;

  
  bool mIsLoadingFromSourceChildren;

  
  
  bool mDelayingLoadEvent;

  
  
  bool mIsRunningSelectResource;

  
  bool mHaveQueuedSelectResource;

  
  
  
  bool mSuspendedAfterFirstFrame;

  
  
  
  bool mAllowSuspendAfterFirstFrame;

  
  
  bool mHasPlayedOrSeeked;

  
  
  
  bool mHasSelfReference;

  
  
  bool mShuttingDown;

  
  
  
  
  bool mSuspendedForPreloadNone;

  
  bool mMediaSecurityVerified;

  
  CORSMode mCORSMode;

  
  MediaInfo mMediaInfo;

  
  bool mIsEncrypted;

#ifdef MOZ_EME
  
  EncryptionInfo mPendingEncryptedInitData;
#endif 

  
  Watchable<bool> mDownloadSuspendedByCache;

  
  AudioChannel mAudioChannel;

  
  bool mAudioChannelFaded;

  
  bool mPlayingThroughTheAudioChannel;

  
  
  
  bool mDisableVideo;

  
  nsCOMPtr<nsIAudioChannelAgent> mAudioChannelAgent;

  nsRefPtr<TextTrackManager> mTextTrackManager;

  nsRefPtr<AudioTrackList> mAudioTrackList;

  nsRefPtr<VideoTrackList> mVideoTrackList;

  enum ElementInTreeState {
    
    ELEMENT_NOT_INTREE,
    
    ELEMENT_INTREE,
    
    
    ELEMENT_NOT_INTREE_HAD_INTREE
  };

  ElementInTreeState mElementInTreeState;

public:
  
  class TimeDurationAccumulator {
  public:
    TimeDurationAccumulator()
      : mCount(0)
    {
    }
    void Start() {
      if (IsStarted()) {
        return;
      }
      mStartTime = TimeStamp::Now();
    }
    void Pause() {
      if (!IsStarted()) {
        return;
      }
      mSum += (TimeStamp::Now() - mStartTime);
      mCount++;
      mStartTime = TimeStamp();
    }
    bool IsStarted() const {
      return !mStartTime.IsNull();
    }
    double Total() const {
      return mSum.ToSeconds();
    }
    uint32_t Count() const {
      return mCount;
    }
  private:
    TimeStamp mStartTime;
    TimeDuration mSum;
    uint32_t mCount;
  };
private:
  
  TimeDurationAccumulator mPlayTime;

  
  TimeDurationAccumulator mJoinLatency;
};

} 
} 

#endif 
