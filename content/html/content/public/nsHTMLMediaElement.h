




































#include "nsIDOMHTMLMediaElement.h"
#include "nsGenericHTMLElement.h"
#include "nsMediaDecoder.h"




typedef PRUint16 nsMediaNetworkState;
typedef PRUint16 nsMediaReadyState;

class nsHTMLMediaElement : public nsGenericHTMLElement
{
public:
  nsHTMLMediaElement(nsINodeInfo *aNodeInfo, PRBool aFromParser = PR_FALSE);
  virtual ~nsHTMLMediaElement();

  
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

  
  
  
  void CanPlayThrough();

  
  
  void SeekStarted();

  
  
  void SeekCompleted();

  
  
  void Paint(gfxContext* aContext, const gfxRect& aRect);

  
  nsresult DispatchSimpleEvent(const nsAString& aName);
  nsresult DispatchProgressEvent(const nsAString& aName);
  nsresult DispatchAsyncSimpleEvent(const nsAString& aName);
  nsresult DispatchAsyncProgressEvent(const nsAString& aName);

  
  
  void ChangeReadyState(nsMediaReadyState aState);

  
  
  PRBool IsActivelyPlaying() const;

  
  
  PRBool IsPlaybackEnded() const;

  
  nsIPrincipal* GetCurrentPrincipal();

  
  
  void UpdateMediaSize(nsIntSize size);

protected:
  nsresult PickMediaElement(nsAString& aChosenMediaResource);
  virtual nsresult InitializeDecoder(nsAString& aChosenMediaResource);

  nsRefPtr<nsMediaDecoder> mDecoder;

  
  nsCOMPtr<nsIDOMHTMLMediaError> mError;

  
  
  nsMediaNetworkState mNetworkState;
  nsMediaReadyState mReadyState;

  
  float mMutedVolume;

  
  
  nsIntSize mMediaSize;

  
  
  
  float mDefaultPlaybackRate;

  
  
  
  
  
  float mPlaybackRate;

  
  
  PRPackedBool mBegun;

  
  PRPackedBool mEnded;

  
  
  PRPackedBool mLoadedFirstFrame;

  
  
  
  
  
  
  
  
  
  PRPackedBool mAutoplaying;

  
  
  PRPackedBool mPaused;

  
  PRPackedBool mMuted;

  
  
  PRPackedBool mIsDoneAddingChildren;

  
  
  
  
  PRPackedBool mPlayingBeforeSeek;
};
