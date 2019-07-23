




































#include "nsIDOMHTMLMediaElement.h"
#include "nsGenericHTMLElement.h"
#include "nsMediaDecoder.h"
#include "nsIChannel.h"
#include "nsThreadUtils.h"
#include "nsIDOMRange.h"
#include "nsCycleCollectionParticipant.h"
#include "nsILoadGroup.h"




typedef PRUint16 nsMediaNetworkState;
typedef PRUint16 nsMediaReadyState;

class nsHTMLMediaElement : public nsGenericHTMLElement
{
public:
  nsHTMLMediaElement(nsINodeInfo *aNodeInfo, PRBool aFromParser = PR_FALSE);
  virtual ~nsHTMLMediaElement();

  







  nsresult LoadWithChannel(nsIChannel *aChannel, nsIStreamListener **aListener);

  
  NS_DECL_NSIDOMHTMLMEDIAELEMENT

  
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

  virtual PRBool IsDoneAddingChildren();
  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);

  



  void NotifyOwnerDocumentActivityChanged();

  
  
  
  void MetadataLoaded();

  
  
  
  
  void FirstFrameLoaded(PRBool aResourceFullyLoaded);

  
  
  void ResourceLoaded();

  
  
  void NetworkError();

  
  
  void DecodeError();

  
  
  void PlaybackEnded();

  
  
  void SeekStarted();

  
  
  void SeekCompleted();

  
  
  
  void DownloadSuspended();

  
  
  
  void DownloadResumed();

  
  
  void DownloadStalled();

  
  
  void Paint(gfxContext* aContext,
             gfxPattern::GraphicsFilter aFilter,
             const gfxRect& aRect);

  
  nsresult DispatchSimpleEvent(const nsAString& aName);
  nsresult DispatchProgressEvent(const nsAString& aName);
  nsresult DispatchAsyncSimpleEvent(const nsAString& aName);
  nsresult DispatchAsyncProgressEvent(const nsAString& aName);

  
  
  
  
  
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

  
  
  
  
  static PRBool CanHandleMediaType(const char* aMIMEType,
                                   const char*** aSupportedCodecs);

  
  
  
  
  static PRBool ShouldHandleMediaType(const char* aMIMEType);

  


  static void InitMediaTypes();
  


  static void ShutdownMediaTypes();

  



  void NotifyAddedSource();

  



  void NotifyLoadError();

  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  




  PRUint32 GetCurrentLoadID() { return mCurrentLoadID; }

  



  already_AddRefed<nsILoadGroup> GetDocumentLoadGroup();

  



  PRBool GetPlayedOrSeeked() { return mHasPlayedOrSeeked; }

protected:
  class MediaLoadListener;
  class LoadNextSourceEvent;
  class SelectResourceEvent;

  




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

  



  already_AddRefed<nsIURI> GetNextSource();

  



  void ChangeDelayLoadStatus(PRBool aDelay);

  


  void StopSuspendingAfterFirstFrame();

  



  nsresult OnChannelRedirect(nsIChannel *aChannel,
                             nsIChannel *aNewChannel,
                             PRUint32 aFlags);

  


  void AddRemoveSelfReference();

  




  void DoRelease() { Release(); }

  nsRefPtr<nsMediaDecoder> mDecoder;

  
  
  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  nsCOMPtr<nsIDOMHTMLMediaError> mError;

  
  
  
  PRUint32 mCurrentLoadID;

  
  
  nsCOMPtr<nsIDOMRange> mSourcePointer;

  
  
  nsCOMPtr<nsIDocument> mLoadBlockedDoc;

  
  
  nsMediaNetworkState mNetworkState;
  nsMediaReadyState mReadyState;

  enum LoadAlgorithmState {
    
    NOT_WAITING,
    
    WAITING_FOR_SRC_OR_SOURCE,
    
    
    
    WAITING_FOR_SOURCE
  };

  
  
  LoadAlgorithmState mLoadWaitStatus;

  
  float mVolume;

  
  
  nsIntSize mMediaSize;

  
  
  PRPackedBool mBegun;

  
  
  PRPackedBool mLoadedFirstFrame;

  
  
  
  
  
  
  
  
  
  PRPackedBool mAutoplaying;

  
  
  PRPackedBool mAutoplayEnabled;

  
  
  PRPackedBool mPaused;

  
  PRPackedBool mMuted;

  
  
  PRPackedBool mIsDoneAddingChildren;

  
  
  
  
  PRPackedBool mPlayingBeforeSeek;

  
  PRPackedBool mPausedForInactiveDocument;

  
  
  PRPackedBool mWaitingFired;

  
  PRPackedBool mIsBindingToTree;

  
  PRPackedBool mIsRunningLoadMethod;

  
  PRPackedBool mIsLoadingFromSrcAttribute;

  
  
  PRPackedBool mDelayingLoadEvent;

  
  
  PRPackedBool mIsRunningSelectResource;

  
  
  PRPackedBool mSuspendedAfterFirstFrame;

  
  
  PRPackedBool mAllowSuspendAfterFirstFrame;

  
  
  PRPackedBool mHasPlayedOrSeeked;

  
  
  
  PRPackedBool mHasSelfReference;
};
