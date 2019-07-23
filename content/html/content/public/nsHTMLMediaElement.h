




































#include "nsIDOMHTMLMediaElement.h"
#include "nsGenericHTMLElement.h"
#include "nsMediaDecoder.h"
#include "nsIChannel.h"




typedef PRUint16 nsMediaNetworkState;
typedef PRUint16 nsMediaReadyState;

class nsHTMLMediaElement : public nsGenericHTMLElement
{
public:
  nsHTMLMediaElement(nsINodeInfo *aNodeInfo, PRBool aFromParser = PR_FALSE);
  virtual ~nsHTMLMediaElement();

  






 
  nsresult LoadWithChannel(nsIChannel *aChannel, nsIStreamListener **aListener);

  
  NS_DECL_NSIDOMHTMLMEDIAELEMENT

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

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  virtual PRBool IsDoneAddingChildren();
  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);
  virtual void DestroyContent();

  
  
  
  void MetadataLoaded();

  
  
  void FirstFrameLoaded();

  
  
  void ResourceLoaded();

  
  
  void NetworkError();

  
  
  void PlaybackEnded();

  
  
  void SeekStarted();

  
  
  void SeekCompleted();

  
  
  void Paint(gfxContext* aContext, const gfxRect& aRect);

  
  nsresult DispatchSimpleEvent(const nsAString& aName);
  nsresult DispatchProgressEvent(const nsAString& aName);
  nsresult DispatchAsyncSimpleEvent(const nsAString& aName);
  nsresult DispatchAsyncProgressEvent(const nsAString& aName);

  
  
  
  
  
  void UpdateReadyStateForData(PRBool aNextFrameAvailable);

  
  
  void ChangeReadyState(nsMediaReadyState aState);

  
  
  PRBool ShouldCheckAllowOrigin();

  
  
  PRBool IsPotentiallyPlaying() const;

  
  
  PRBool IsPlaybackEnded() const;

  
  nsIPrincipal* GetCurrentPrincipal();

  
  
  void UpdateMediaSize(nsIntSize size);

  
  
  void Freeze();
  void Thaw();

  
  
  
  
  
  
  static PRBool CanHandleMediaType(const char* aMIMEType,
                                   const char*** aSupportedCodecs,
                                   const char*** aMaybeSupportedCodecs);

  


  static void InitMediaTypes();
  


  static void ShutdownMediaTypes();

protected:
  class nsMediaLoadListener;

  



  nsresult PickMediaElement(nsIURI** aURI);
  



  PRBool CreateDecoder(const nsACString& aMIMEType);
  



  nsresult InitializeDecoderForChannel(nsIChannel *aChannel,
                                       nsIStreamListener **aListener);
  




  PRBool AbortExistingLoads();
  


  nsresult NewURIFromString(const nsAutoString& aURISpec, nsIURI** aURI);

  nsRefPtr<nsMediaDecoder> mDecoder;

  nsCOMPtr<nsIChannel> mChannel;

  
  nsCOMPtr<nsIDOMHTMLMediaError> mError;

  
  
  nsMediaNetworkState mNetworkState;
  nsMediaReadyState mReadyState;

  
  float mMutedVolume;

  
  
  nsIntSize mMediaSize;

  
  
  PRPackedBool mBegun;

  
  
  PRPackedBool mLoadedFirstFrame;

  
  
  
  
  
  
  
  
  
  PRPackedBool mAutoplaying;

  
  
  PRPackedBool mPaused;

  
  PRPackedBool mMuted;

  
  
  PRPackedBool mIsDoneAddingChildren;

  
  
  
  
  PRPackedBool mPlayingBeforeSeek;

  
  
  
  PRPackedBool mPausedBeforeFreeze;
};
