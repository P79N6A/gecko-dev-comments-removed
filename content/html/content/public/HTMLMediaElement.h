




#ifndef mozilla_dom_HTMLMediaElement_h
#define mozilla_dom_HTMLMediaElement_h

#include "nsIDOMHTMLMediaElement.h"
#include "nsGenericHTMLElement.h"
#include "MediaDecoderOwner.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsThreadUtils.h"
#include "nsIDOMRange.h"
#include "nsCycleCollectionParticipant.h"
#include "nsILoadGroup.h"
#include "nsIObserver.h"
#include "AudioStream.h"
#include "VideoFrameContainer.h"
#include "mozilla/CORSMode.h"
#include "DOMMediaStream.h"
#include "mozilla/Mutex.h"
#include "mozilla/dom/TimeRanges.h"
#include "nsIDOMWakeLock.h"
#include "AudioChannelCommon.h"
#include "DecoderTraits.h"
#include "MediaMetadataManager.h"
#include "AudioChannelAgent.h"




typedef uint16_t nsMediaNetworkState;
typedef uint16_t nsMediaReadyState;

namespace mozilla {
class MediaResource;
class MediaDecoder;
}

namespace mozilla {
namespace dom {

class HTMLMediaElement : public nsGenericHTMLElement,
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

  HTMLMediaElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLMediaElement();

  







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
                                nsAttrValue& aResult);
  
  
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttr,
                             bool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual void DoneCreatingElement();

  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable,
                               int32_t *aTabIndex);
  virtual int32_t TabIndexDefault();

  



  void NotifyOwnerDocumentActivityChanged();

  
  
  
  virtual void MetadataLoaded(int aChannels,
                              int aRate,
                              bool aHasAudio,
                              bool aHasVideo,
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

  
  
  
  
  
  void NotifyAudioAvailableListener();

  
  
  virtual VideoFrameContainer* GetVideoFrameContainer() MOZ_FINAL MOZ_OVERRIDE;
  layers::ImageContainer* GetImageContainer();

  
  
  gfxASurface* GetPrintSurface() { return mPrintSurface; }

  
  using nsGenericHTMLElement::DispatchEvent;
  virtual nsresult DispatchEvent(const nsAString& aName) MOZ_FINAL MOZ_OVERRIDE;
  virtual nsresult DispatchAsyncEvent(const nsAString& aName) MOZ_FINAL MOZ_OVERRIDE;
  nsresult DispatchAudioAvailableEvent(float* aFrameBuffer,
                                       uint32_t aFrameBufferLength,
                                       float aTime);

  
  nsresult DispatchPendingMediaEvents();

  
  
  
  
  
  virtual void UpdateReadyStateForData(MediaDecoderOwner::NextFrameStatus aNextFrame) MOZ_FINAL MOZ_OVERRIDE;

  
  
  void ChangeReadyState(nsMediaReadyState aState);

  
  bool CanActivateAutoplay();

  
  
  
  virtual void NotifyAutoplayDataReady() MOZ_FINAL MOZ_OVERRIDE;

  
  bool ShouldCheckAllowOrigin();

  
  
  bool IsPotentiallyPlaying() const;

  
  
  bool IsPlaybackEnded() const;

  
  
  
  already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  
  virtual void NotifyDecoderPrincipalChanged() MOZ_FINAL MOZ_OVERRIDE;

  
  
  void UpdateMediaSize(nsIntSize size);

  
  
  static CanPlayStatus GetCanPlay(const nsAString& aType);

  



  void NotifyAddedSource();

  



  void NotifyLoadError();

  


  virtual void NotifyAudioAvailable(float* aFrameBuffer, uint32_t aFrameBufferLength,
                                    float aTime) MOZ_FINAL MOZ_OVERRIDE;

  virtual bool IsNodeOfType(uint32_t aFlags) const;

  




  uint32_t GetCurrentLoadID() { return mCurrentLoadID; }

  



  already_AddRefed<nsILoadGroup> GetDocumentLoadGroup();

  



  bool GetPlayedOrSeeked() const { return mHasPlayedOrSeeked; }

  nsresult CopyInnerTo(Element* aDest);

  



  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel) = 0;

  



  void SetRequestHeaders(nsIHttpChannel* aChannel);

  





  virtual void FireTimeUpdate(bool aPeriodic) MOZ_FINAL MOZ_OVERRIDE;

  MediaStream* GetSrcMediaStream()
  {
    NS_ASSERTION(mSrcStream, "Don't call this when not playing a stream");
    return mSrcStream->GetStream();
  }

protected:
  class MediaLoadListener;
  class StreamListener;

  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);

  class WakeLockBoolWrapper {
  public:
    WakeLockBoolWrapper(bool val = false) : mValue(val), mOuter(NULL), mWakeLock(NULL) {}
    void SetOuter(HTMLMediaElement* outer) { mOuter = outer; }
    operator bool() { return mValue; }
    WakeLockBoolWrapper& operator=(bool val);
    bool operator !() const { return !mValue; }
  private:
    bool mValue;
    HTMLMediaElement* mOuter;
    nsCOMPtr<nsIDOMMozWakeLock> mWakeLock;
  };

  





  void ReportLoadError(const char* aMsg,
                       const PRUnichar** aParams = nullptr,
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

  


  nsresult NewURIFromString(const nsAutoString& aURISpec, nsIURI** aURI);

  




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

  


  void SetMutedInternal(bool aMuted);

  




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

  
  nsresult UpdateChannelMuteState(bool aCanPlay);

  
  void UpdateAudioChannelPlayingState();

  
  
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

  
  
  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  nsCOMPtr<nsIDOMMediaError> mError;

  
  
  
  uint32_t mCurrentLoadID;

  
  
  nsCOMPtr<nsIDOMRange> mSourcePointer;

  
  
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

  
  uint32_t mChannels;

  
  uint32_t mRate;

  
  
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

  nsRefPtr<gfxASurface> mPrintSurface;

  
  
  nsCOMPtr<nsIContent> mSourceLoadCandidate;

  
  nsAutoPtr<AudioStream> mAudioStream;

  
  TimeRanges mPlayed;

  
  double mCurrentPlayRangeStart;

  
  
  bool mAllowAudioData;

  
  
  bool mBegun;

  
  
  bool mLoadedFirstFrame;

  
  
  
  
  
  
  
  
  
  bool mAutoplaying;

  
  
  bool mAutoplayEnabled;

  
  
  WakeLockBoolWrapper mPaused;

  
  bool mMuted;

  
  bool mAudioCaptured;

  
  
  
  
  bool mPlayingBeforeSeek;

  
  
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

  
  AudioChannelType mAudioChannelType;

  
  bool mChannelSuspended;

  
  bool mPlayingThroughTheAudioChannel;

  
  nsCOMPtr<nsIAudioChannelAgent> mAudioChannelAgent;
};

} 
} 

#endif 
