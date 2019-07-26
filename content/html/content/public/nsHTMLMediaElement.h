




































#if !defined(nsHTMLMediaElement_h__)
#define nsHTMLMediaElement_h__

#include "nsIDOMHTMLMediaElement.h"
#include "nsGenericHTMLElement.h"
#include "nsMediaDecoder.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsThreadUtils.h"
#include "nsIDOMRange.h"
#include "nsCycleCollectionParticipant.h"
#include "nsILoadGroup.h"
#include "nsIObserver.h"
#include "nsAudioStream.h"
#include "VideoFrameContainer.h"
#include "mozilla/CORSMode.h"
#include "nsDOMMediaStream.h"
#include "mozilla/Mutex.h"
#include "nsTimeRanges.h"




typedef PRUint16 nsMediaNetworkState;
typedef PRUint16 nsMediaReadyState;

namespace mozilla {
class MediaResource;
}

class nsHTMLMediaElement : public nsGenericHTMLElement,
                           public nsIObserver
{
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::VideoFrameContainer VideoFrameContainer;
  typedef mozilla::MediaStream MediaStream;
  typedef mozilla::MediaResource MediaResource;

  enum CanPlayStatus {
    CANPLAY_NO,
    CANPLAY_MAYBE,
    CANPLAY_YES
  };

  mozilla::CORSMode GetCORSMode() {
    return mCORSMode;
  }

  nsHTMLMediaElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLMediaElement();

  







  nsresult LoadWithChannel(nsIChannel *aChannel, nsIStreamListener **aListener);

  
  NS_DECL_NSIDOMHTMLMEDIAELEMENT

  NS_DECL_NSIOBSERVER

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLMediaElement,
                                           nsGenericHTMLElement)

  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  
  
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr,
                             bool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual void DoneCreatingElement();

  



  void NotifyOwnerDocumentActivityChanged();

  
  
  
  void MetadataLoaded(PRUint32 aChannels, PRUint32 aRate, bool aHasAudio);

  
  
  
  
  void FirstFrameLoaded(bool aResourceFullyLoaded);

  
  
  void ResourceLoaded();

  
  
  void NetworkError();

  
  
  void DecodeError();

  
  
  void LoadAborted();

  
  
  void PlaybackEnded();

  
  
  void SeekStarted();

  
  
  void SeekCompleted();

  
  
  
  void DownloadSuspended();

  
  
  
  void DownloadResumed();

  
  
  void DownloadStalled();

  
  
  
  
  
  void NotifyAudioAvailableListener();

  
  
  VideoFrameContainer* GetVideoFrameContainer();
  ImageContainer* GetImageContainer()
  {
    VideoFrameContainer* container = GetVideoFrameContainer();
    return container ? container->GetImageContainer() : nsnull;
  }

  
  
  gfxASurface* GetPrintSurface() { return mPrintSurface; }

  
  using nsGenericHTMLElement::DispatchEvent;
  nsresult DispatchEvent(const nsAString& aName);
  nsresult DispatchAsyncEvent(const nsAString& aName);
  nsresult DispatchAudioAvailableEvent(float* aFrameBuffer,
                                       PRUint32 aFrameBufferLength,
                                       float aTime);

  
  nsresult DispatchPendingMediaEvents();

  
  
  
  
  
  enum NextFrameStatus {
    
    NEXT_FRAME_AVAILABLE,
    
    
    NEXT_FRAME_UNAVAILABLE_BUFFERING,
    
    NEXT_FRAME_UNAVAILABLE
  };
  void UpdateReadyStateForData(NextFrameStatus aNextFrame);

  
  
  void ChangeReadyState(nsMediaReadyState aState);

  
  bool CanActivateAutoplay();

  
  
  
  void NotifyAutoplayDataReady();

  
  bool ShouldCheckAllowOrigin();

  
  
  bool IsPotentiallyPlaying() const;

  
  
  bool IsPlaybackEnded() const;

  
  
  
  already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  
  void NotifyDecoderPrincipalChanged();

  
  
  void UpdateMediaSize(nsIntSize size);

  
  
  
  
  
  static CanPlayStatus CanHandleMediaType(const char* aMIMEType,
                                          char const *const ** aSupportedCodecs);

  
  
  static CanPlayStatus GetCanPlay(const nsAString& aType);

  
  
  
  
  static bool ShouldHandleMediaType(const char* aMIMEType);

#ifdef MOZ_RAW
  static bool IsRawEnabled();
#endif

#ifdef MOZ_OGG
  static bool IsOggEnabled();
  static bool IsOggType(const nsACString& aType);
  static const char gOggTypes[3][16];
  static char const *const gOggCodecs[3];
  static bool IsOpusEnabled();
  static char const *const gOggCodecsWithOpus[4];
#endif

#ifdef MOZ_WAVE
  static bool IsWaveEnabled();
  static bool IsWaveType(const nsACString& aType);
  static const char gWaveTypes[4][16];
  static char const *const gWaveCodecs[2];
#endif

#ifdef MOZ_WEBM
  static bool IsWebMEnabled();
  static bool IsWebMType(const nsACString& aType);
  static const char gWebMTypes[2][17];
  static char const *const gWebMCodecs[4];
#endif

#ifdef MOZ_GSTREAMER
  static bool IsH264Enabled();
  static bool IsH264Type(const nsACString& aType);
  static const char gH264Types[3][17];
  static char const *const gH264Codecs[6];
#endif

  



  void NotifyAddedSource();

  



  void NotifyLoadError();

  


  void NotifyAudioAvailable(float* aFrameBuffer, PRUint32 aFrameBufferLength,
                            float aTime);

  virtual bool IsNodeOfType(PRUint32 aFlags) const;

  




  PRUint32 GetCurrentLoadID() { return mCurrentLoadID; }

  



  already_AddRefed<nsILoadGroup> GetDocumentLoadGroup();

  



  bool GetPlayedOrSeeked() const { return mHasPlayedOrSeeked; }

  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  



  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel) = 0;

  



  void SetRequestHeaders(nsIHttpChannel* aChannel);

  





  void FireTimeUpdate(bool aPeriodic);

  MediaStream* GetMediaStream()
  {
    NS_ASSERTION(mStream, "Don't call this when not playing a stream");
    return mStream->GetStream();
  }

protected:
  class MediaLoadListener;
  class StreamListener;

  





  void ReportLoadError(const char* aMsg,
                       const PRUnichar** aParams = nsnull,
                       PRUint32 aParamCount = 0);

  




  void SetPlayedOrSeeked(bool aValue);

  


  void SetupMediaStreamPlayback();
  


  void EndMediaStreamPlayback();

  






  already_AddRefed<nsDOMMediaStream> CaptureStreamInternal(bool aFinishWhenEnded);

  



  already_AddRefed<nsMediaDecoder> CreateDecoder(const nsACString& aMIMEType);

  




  nsresult InitializeDecoderAsClone(nsMediaDecoder* aOriginal);

  




  nsresult InitializeDecoderForChannel(nsIChannel *aChannel,
                                       nsIStreamListener **aListener);

  



  nsresult FinishDecoderSetup(nsMediaDecoder* aDecoder,
                              MediaResource* aStream,
                              nsIStreamListener **aListener,
                              nsMediaDecoder* aCloneDonor);

  


  void AddMediaElementToURITable();
  


  void RemoveMediaElementFromURITable();
  



  nsHTMLMediaElement* LookupMediaElementURITable(nsIURI* aURI);

  


  void ShutdownDecoder();
  




  void AbortExistingLoads();

  


  nsresult NewURIFromString(const nsAutoString& aURISpec, nsIURI** aURI);

  




  void NoSupportedMediaSourceError();

  




  void LoadFromSourceChildren();

  



  void QueueLoadFromSourceTask();

  


  void SelectResource();

  



  void SelectResourceWrapper();

  



  void QueueSelectResourceTask();

  


  nsresult LoadResource();

  




  nsIContent* GetNextSource();

  



  void ChangeDelayLoadStatus(bool aDelay);

  


  void StopSuspendingAfterFirstFrame();

  



  nsresult OnChannelRedirect(nsIChannel *aChannel,
                             nsIChannel *aNewChannel,
                             PRUint32 aFlags);

  


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

  



  void Error(PRUint16 aErrorCode);

  


  void GetCurrentSpec(nsCString& aString);

  


  void ProcessMediaFragmentURI();

  
  
  nsRefPtr<nsMediaDecoder> mDecoder;

  
  
  nsRefPtr<VideoFrameContainer> mVideoFrameContainer;

  
  
  nsRefPtr<nsDOMMediaStream> mSrcAttrStream;

  
  
  
  nsRefPtr<nsDOMMediaStream> mStream;

  
  
  struct OutputMediaStream {
    nsRefPtr<nsDOMMediaStream> mStream;
    bool mFinishWhenEnded;
  };
  nsTArray<OutputMediaStream> mOutputStreams;

  
  StreamListener* mStreamListener;

  
  
  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  nsCOMPtr<nsIDOMMediaError> mError;

  
  
  
  PRUint32 mCurrentLoadID;

  
  
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

  
  PRUint32 mChannels;

  
  PRUint32 mRate;

  
  
  
  
  
  nsCOMPtr<nsIURI> mLoadingSrc;
  
  
  
  
  PreloadAction mPreloadAction;

  
  
  
  
  
  nsIntSize mMediaSize;

  
  
  TimeStamp mTimeUpdateTime;

  
  
  double mLastCurrentTime;

  
  
  
  double mFragmentStart;

  
  
  
  double mFragmentEnd;

  nsRefPtr<gfxASurface> mPrintSurface;

  
  
  nsCOMPtr<nsIContent> mSourceLoadCandidate;

  
  nsRefPtr<nsAudioStream> mAudioStream;

  
  nsTimeRanges mPlayed;

  
  double mCurrentPlayRangeStart;

  
  
  bool mAllowAudioData;

  
  
  bool mBegun;

  
  
  bool mLoadedFirstFrame;

  
  
  
  
  
  
  
  
  
  bool mAutoplaying;

  
  
  bool mAutoplayEnabled;

  
  
  bool mPaused;

  
  bool mMuted;

  
  bool mAudioCaptured;

  
  
  
  
  bool mPlayingBeforeSeek;

  
  bool mPausedForInactiveDocument;

  
  
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

  
  
  
  
  bool mLoadIsSuspended;

  
  bool mMediaSecurityVerified;

  
  mozilla::CORSMode mCORSMode;

  
  bool mHasAudio;
};

#endif
