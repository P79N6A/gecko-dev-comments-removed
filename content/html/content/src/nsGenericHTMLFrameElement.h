






#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMMozBrowserFrame.h"
#include "nsIDOMEventListener.h"
#include "nsIWebProgressListener.h"




class nsGenericHTMLFrameElement : public nsGenericHTMLElement,
                                  public nsIFrameLoaderOwner,
                                  public nsIDOMMozBrowserFrame,
                                  public nsIWebProgressListener
{
public:
  nsGenericHTMLFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                            mozilla::dom::FromParser aFromParser)
    : nsGenericHTMLElement(aNodeInfo)
    , mNetworkCreated(aFromParser == mozilla::dom::FROM_PARSER_NETWORK)
    , mBrowserFrameListenersRegistered(false)
  {
  }

  virtual ~nsGenericHTMLFrameElement();

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
  NS_DECL_NSIFRAMELOADEROWNER
  NS_DECL_NSIDOMMOZBROWSERFRAME
  NS_DECL_NSIWEBPROGRESSLISTENER

  
  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, PRInt32 *aTabIndex);
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual void DestroyContent();

  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  
  NS_IMETHOD GetTabIndex(PRInt32 *aTabIndex);
  NS_IMETHOD SetTabIndex(PRInt32 aTabIndex);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsGenericHTMLFrameElement,
                                                     nsGenericHTMLElement)

protected:
  




  class TitleChangedListener MOZ_FINAL : public nsIDOMEventListener
  {
  public:
    TitleChangedListener(nsGenericHTMLFrameElement *aElement,
                         nsIDOMEventTarget *aChromeHandler);

    
    void Unregister();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENTLISTENER

  private:
    nsWeakPtr mElement; 
    nsWeakPtr mChromeHandler; 
  };

  
  
  nsresult EnsureFrameLoader();
  nsresult LoadSrc();
  nsresult GetContentDocument(nsIDOMDocument** aContentDocument);
  nsresult GetContentWindow(nsIDOMWindow** aContentWindow);

  void MaybeEnsureBrowserFrameListenersRegistered();
  bool BrowserFrameSecurityCheck();
  nsresult MaybeFireBrowserEvent(const nsAString &aEventName,
                                 const nsAString &aEventType,
                                 const nsAString &aValue = EmptyString());

  nsRefPtr<nsFrameLoader> mFrameLoader;
  nsRefPtr<TitleChangedListener> mTitleChangedListener;

  
  
  
  bool                    mNetworkCreated;

  bool                    mBrowserFrameListenersRegistered;
};
