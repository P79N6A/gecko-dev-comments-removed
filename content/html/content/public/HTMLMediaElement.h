




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
#include "mozilla/dom/AudioChannelBinding.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/TextTrackManager.h"
#include "MediaDecoder.h"
#ifdef MOZ_EME
#include "mozilla/dom/MediaKeys.h"
#endif




#ifdef None
#undef None
#endif


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

  HTMLMediaElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  







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
                              nsAttrValue& aResult) MOZ_OVERRIDE;
  
  
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) MOZ_OVERRIDE;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttr,
                             bool aNotify) MOZ_OVERRIDE;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;
  virtual void DoneCreatingElement() MOZ_OVERRIDE;

  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable,
                               int32_t *aTabIndex) MOZ_OVERRIDE;
  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;

  



  virtual void NotifyOwnerDocumentActivityChanged();

  
  
  
  virtual void MetadataLoaded(const MediaInfo* aInfo,
                              const MetadataTags* aTags) MOZ_FINAL MOZ_OVERRIDE;

  
  
  
  
  virtual void FirstFrameLoaded(bool aResourceFullyLoaded) MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual void ResourceLoaded() MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual void NetworkError() MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual void DecodeError() MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual void LoadAborted() MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual void PlaybackEnded() MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual void SeekStarted() MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual void SeekCompleted() MOZ_FINAL MOZ_OVERRIDE;

  
  
  
  virtual void DownloadSuspended() MOZ_FINAL MOZ_OVERRIDE;

  
  
  
  
  
  
  
  virtual void DownloadResumed(bool aForceNetworkLoading = false) MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual void DownloadStalled() MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual void NotifySuspendedByCache(bool aIsSuspended) MOZ_FINAL MOZ_OVERRIDE;

  
  
  virtual VideoFrameContainer* GetVideoFrameContainer() MOZ_FINAL MOZ_OVERRIDE;
  layers::ImageContainer* GetImageContainer();

  
  using nsGenericHTMLElement::DispatchEvent;
  virtual nsresult DispatchEvent(const nsAString& aName) MOZ_FINAL MOZ_OVERRIDE;
  virtual nsresult DispatchAsyncEvent(const nsAString& aName) MOZ_FINAL MOZ_OVERRIDE;

  
  nsresult DispatchPendingMediaEvents();

  
  
  
  
  
  virtual void UpdateReadyStateForData(MediaDecoderOwner::NextFrameStatus aNextFrame) MOZ_FINAL MOZ_OVERRIDE;

  
  
  void ChangeReadyState(nsMediaReadyState aState);

  
  bool CanActivateAutoplay();

  
  
  
  
  void CheckAutoplayDataReady();

  
  bool ShouldCheckAllowOrigin();

  
  
  bool IsPotentiallyPlaying() const;

  
  
  bool IsPlaybackEnded() const;

  
  
  
  already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  
  virtual void NotifyDecoderPrincipalChanged() MOZ_FINAL MOZ_OVERRIDE;

  
  
  void UpdateMediaSize(nsIntSize size);

  
  
  static CanPlayStatus GetCanPlay(const nsAString& aType);

  



  void NotifyAddedSource();

  



  void NotifyLoadError();

  void NotifyMediaTrackEnabled(MediaTrack* aTrack);

  virtual bool IsNodeOfType(uint32_t aFlags) const MOZ_OVERRIDE;

  




  uint32_t GetCurrentLoadID() { return mCurrentLoadID; }

  



  already_AddRefed<nsILoadGroup> GetDocumentLoadGroup();

  



  bool GetPlayedOrSeeked() const { return mHasPlayedOrSeeked; }

  nsresult CopyInnerTo(Element* aDest);

  



  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel) = 0;

  



  void SetRequestHeaders(nsIHttpChannel* aChannel);

  




  void RunInStableState(nsIRunnable* aRunnable);

  





  virtual void FireTimeUpdate(bool aPeriodic) MOZ_FINAL MOZ_OVERRIDE;

  MediaStream* GetSrcMediaStream() const
  {
    NS_ASSERTION(mSrcStream, "Don't call this when not playing a stream");
    return mSrcStream->GetStream();
  }

  

  MediaError* GetError() const
  {
    return mError;
  }

  
  void SetSrc(const nsAString& aSrc, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::src, aSrc, aRv);
  }

  

  
  void SetCrossOrigin(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::crossorigin, aValue, aRv);
  }

  uint16_t NetworkState() const
  {
    return mNetworkState;
  }

  
  
  void ResetConnectionState() MOZ_FINAL MOZ_OVERRIDE;

  
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

  already_AddRefed<DOMMediaStream> GetMozSrcObject() const;

  void SetMozSrcObject(DOMMediaStream& aValue);

  bool MozPreservesPitch() const
  {
    return mPreservesPitch;
  }

  

#ifdef MOZ_EME
  MediaKeys* GetMediaKeys() const;

  already_AddRefed<Promise> SetMediaKeys(MediaKeys* mediaKeys,
                                         ErrorResult& aRv);
  
  MediaWaitingFor WaitingFor() const;

  mozilla::dom::EventHandlerNonNull* GetOnencrypted();
  void SetOnencrypted(mozilla::dom::EventHandlerNonNull* listener);

  void DispatchEncrypted(const nsTArray<uint8_t>& aInitData,
                         const nsAString& aInitDataType);


  bool IsEventAttributeName(nsIAtom* aName) MOZ_OVERRIDE;
#endif 

  bool MozAutoplayEnabled() const
  {
    return mAutoplayEnabled;
  }

  already_AddRefed<DOMMediaStream> MozCaptureStream(ErrorResult& aRv);

  already_AddRefed<DOMMediaStream> MozCaptureStreamUntilEnded(ErrorResult& aRv);

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

  TextTrackList* TextTracks();

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

protected:
  virtual ~HTMLMediaElement();

  class MediaLoadListener;
  class StreamListener;

  virtual void GetItemValueText(nsAString& text) MOZ_OVERRIDE;
  virtual void SetItemValueText(const nsAString& text) MOZ_OVERRIDE;

  class WakeLockBoolWrapper {
  public:
    WakeLockBoolWrapper(bool val = false)
      : mValue(val), mCanPlay(true), mOuter(nullptr) {}

    ~WakeLockBoolWrapper();

    void SetOuter(HTMLMediaElement* outer) { mOuter = outer; }
    void SetCanPlay(bool aCanPlay);

    operator bool() const { return mValue; }

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

  



  virtual void WakeLockCreate();
  virtual void WakeLockRelease();
  nsRefPtr<WakeLock> mWakeLock;

  





  void ReportLoadError(const char* aMsg,
                       const char16_t** aParams = nullptr,
                       uint32_t aParamCount = 0);

  




  void SetPlayedOrSeeked(bool aValue);

  


  void SetupSrcMediaStreamPlayback(DOMMediaStream* aStream);
  


  void EndSrcMediaStreamPlayback();

  






  already_AddRefed<DOMMediaStream> CaptureStreamInternal(bool aFinishWhenEnded);

  




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

  


  void DispatchAsyncSourceError(nsIContent* aSourceElement);

  



  void Error(uint16_t aErrorCode);

  


  void GetCurrentSpec(nsCString& aString);

  


  void ProcessMediaFragmentURI();

  


  void SetMutedInternal(uint32_t aMuted);
  



  void SetVolumeInternal();

  




  void SuspendOrResumeElement(bool aPauseElement, bool aSuspendEvents);

  
  
  virtual HTMLMediaElement* GetMediaElement() MOZ_FINAL MOZ_OVERRIDE
  {
    return this;
  }

  
  virtual bool GetPaused() MOZ_FINAL MOZ_OVERRIDE
  {
    bool isPaused = false;
    GetPaused(&isPaused);
    return isPaused;
  }

  
  bool CheckAudioChannelPermissions(const nsAString& aType);

  
  nsresult UpdateChannelMuteState(mozilla::dom::AudioChannelState aCanPlay);

  
  
  
  
  void Seek(double aTime, SeekTarget::Type aSeekType, ErrorResult& aRv);
  
  
  void UpdateAudioChannelPlayingState();

  
  
  
  void PopulatePendingTextTrackList();

  
  
  TextTrackManager* GetOrCreateTextTrackManager();

  
  
  nsRefPtr<MediaDecoder> mDecoder;

  
  
  nsRefPtr<VideoFrameContainer> mVideoFrameContainer;

  
  
  nsRefPtr<DOMMediaStream> mSrcAttrStream;

  
  
  
  nsRefPtr<DOMMediaStream> mSrcStream;

  
  
  struct OutputMediaStream {
    nsRefPtr<DOMMediaStream> mStream;
    bool mFinishWhenEnded;
  };
  nsTArray<OutputMediaStream> mOutputStreams;

  
  nsRefPtr<StreamListener> mSrcStreamListener;

  
  nsRefPtr<MediaSource> mMediaSource;

  
  
  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  nsRefPtr<MediaError> mError;

  
  
  
  uint32_t mCurrentLoadID;

  
  
  nsRefPtr<nsRange> mSourcePointer;

  
  
  nsCOMPtr<nsIDocument> mLoadBlockedDoc;

  
  
  nsTArray<nsString> mPendingEvents;

  
  
  nsMediaNetworkState mNetworkState;
  nsMediaReadyState mReadyState;

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

  
  
  
  
  
  nsIntSize mMediaSize;

  
  
  TimeStamp mTimeUpdateTime;

  
  
  double mLastCurrentTime;

  
  
  
  double mFragmentStart;

  
  
  
  double mFragmentEnd;

  
  
  double mDefaultPlaybackRate;

  
  
  
  
  double mPlaybackRate;

  
  
  bool mPreservesPitch;

  
  
  nsCOMPtr<nsIContent> mSourceLoadCandidate;

  
  nsRefPtr<TimeRanges> mPlayed;

#ifdef MOZ_EME
  
  nsRefPtr<MediaKeys> mMediaKeys;
#endif

  
  double mCurrentPlayRangeStart;

  
  
  bool mBegun;

  
  
  bool mLoadedFirstFrame;

  
  
  
  
  
  
  
  
  
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

  
  bool mHasAudio;

  
  bool mDownloadSuspendedByCache;

  
  AudioChannel mAudioChannel;

  
  bool mAudioChannelFaded;

  
  bool mPlayingThroughTheAudioChannel;

  
  
  
  bool mDisableVideo;

  
  nsCOMPtr<nsIAudioChannelAgent> mAudioChannelAgent;

  nsRefPtr<TextTrackManager> mTextTrackManager;

  nsRefPtr<AudioTrackList> mAudioTrackList;

  nsRefPtr<VideoTrackList> mVideoTrackList;

  MediaWaitingFor mWaitingFor;
};

} 
} 

#endif 
