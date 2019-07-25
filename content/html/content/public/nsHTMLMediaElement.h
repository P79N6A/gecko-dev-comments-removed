




































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
#include "ImageLayers.h"
#include "nsAudioStream.h"




typedef PRUint16 nsMediaNetworkState;
typedef PRUint16 nsMediaReadyState;

class nsHTMLMediaElement : public nsGenericHTMLElement,
                           public nsIObserver
{
  typedef mozilla::layers::ImageContainer ImageContainer;

public:

  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  enum CanPlayStatus {
    CANPLAY_NO,
    CANPLAY_MAYBE,
    CANPLAY_YES
  };

  nsHTMLMediaElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                     mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);
  virtual ~nsHTMLMediaElement();

  







  nsresult LoadWithChannel(nsIChannel *aChannel, nsIStreamListener **aListener);

  
  NS_DECL_NSIDOMHTMLMEDIAELEMENT

  NS_DECL_NSIOBSERVER

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLMediaElement,
                                           nsGenericHTMLElement)

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  
  
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr,
                             PRBool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  



  void NotifyOwnerDocumentActivityChanged();

  
  
  
  void MetadataLoaded(PRUint32 aChannels, PRUint32 aRate);

  
  
  
  
  void FirstFrameLoaded(PRBool aResourceFullyLoaded);

  
  
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

  
  
  ImageContainer* GetImageContainer();

  
  
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

  
  PRBool CanActivateAutoplay();

  
  
  
  void NotifyAutoplayDataReady();

  
  
  PRBool ShouldCheckAllowOrigin();

  
  
  PRBool IsPotentiallyPlaying() const;

  
  
  PRBool IsPlaybackEnded() const;

  
  already_AddRefed<nsIPrincipal> GetCurrentPrincipal();

  
  
  void UpdateMediaSize(nsIntSize size);

  
  
  
  
  
  static CanPlayStatus CanHandleMediaType(const char* aMIMEType,
                                          char const *const ** aSupportedCodecs);

  
  
  static CanPlayStatus GetCanPlay(const nsAString& aType);

  
  
  
  
  static PRBool ShouldHandleMediaType(const char* aMIMEType);

#ifdef MOZ_OGG
  static bool IsOggEnabled();
  static bool IsOggType(const nsACString& aType);
  static const char gOggTypes[3][16];
  static char const *const gOggCodecs[3];
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

  



  void NotifyAddedSource();

  



  void NotifyLoadError();

  


  void NotifyAudioAvailable(float* aFrameBuffer, PRUint32 aFrameBufferLength,
                            float aTime);

  



  PRBool MayHaveAudioAvailableEventListener();

  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  




  PRUint32 GetCurrentLoadID() { return mCurrentLoadID; }

  



  already_AddRefed<nsILoadGroup> GetDocumentLoadGroup();

  



  PRBool GetPlayedOrSeeked() const { return mHasPlayedOrSeeked; }

  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  



  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel) = 0;

  



  void SetRequestHeaders(nsIHttpChannel* aChannel);

  





  void FireTimeUpdate(PRBool aPeriodic);

protected:
  class MediaLoadListener;

  




  void SetPlayedOrSeeked(PRBool aValue);

  



  already_AddRefed<nsMediaDecoder> CreateDecoder(const nsACString& aMIMEType);

  



  nsresult InitializeDecoderAsClone(nsMediaDecoder* aOriginal);

  



  nsresult InitializeDecoderForChannel(nsIChannel *aChannel,
                                       nsIStreamListener **aListener);

  


  nsresult FinishDecoderSetup(nsMediaDecoder* aDecoder);

  




  void AbortExistingLoads();

  


  nsresult NewURIFromString(const nsAutoString& aURISpec, nsIURI** aURI);

  




  void NoSupportedMediaSourceError();

  




  void LoadFromSourceChildren();

  



  void QueueLoadFromSourceTask();

  


  void SelectResource();

  



  void QueueSelectResourceTask();

  


  nsresult LoadResource(nsIURI* aURI);

  




  nsIContent* GetNextSource();

  



  void ChangeDelayLoadStatus(PRBool aDelay);

  


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

  




  void SuspendLoad(nsIURI* aURI);

  




  void ResumeLoad(PreloadAction aAction);

  






  void UpdatePreloadAction();

  


  void DispatchAsyncSourceError(nsIContent* aSourceElement);

  



  void Error(PRUint16 aErrorCode);

  nsRefPtr<nsMediaDecoder> mDecoder;

  
  
  nsRefPtr<ImageContainer> mImageContainer;

  
  
  
  
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

  nsRefPtr<gfxASurface> mPrintSurface;

  
  
  nsCOMPtr<nsIContent> mSourceLoadCandidate;

  
  nsRefPtr<nsAudioStream> mAudioStream;

  
  
  PRBool mAllowAudioData;

  
  
  PRPackedBool mBegun;

  
  
  PRPackedBool mLoadedFirstFrame;

  
  
  
  
  
  
  
  
  
  PRPackedBool mAutoplaying;

  
  
  PRPackedBool mAutoplayEnabled;

  
  
  PRPackedBool mPaused;

  
  PRPackedBool mMuted;

  
  
  
  
  PRPackedBool mPlayingBeforeSeek;

  
  PRPackedBool mPausedForInactiveDocument;

  
  
  PRPackedBool mWaitingFired;

  
  PRPackedBool mIsRunningLoadMethod;

  
  PRPackedBool mIsLoadingFromSourceChildren;

  
  
  PRPackedBool mDelayingLoadEvent;

  
  
  PRPackedBool mIsRunningSelectResource;

  
  
  
  PRPackedBool mSuspendedAfterFirstFrame;

  
  
  
  PRPackedBool mAllowSuspendAfterFirstFrame;

  
  
  PRPackedBool mHasPlayedOrSeeked;

  
  
  
  PRPackedBool mHasSelfReference;

  
  
  PRPackedBool mShuttingDown;

  
  
  
  
  PRPackedBool mLoadIsSuspended;

  
  PRPackedBool mMediaSecurityVerified;
};

#endif
