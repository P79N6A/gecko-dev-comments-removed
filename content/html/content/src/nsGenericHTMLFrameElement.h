






#include "nsGenericHTMLElement.h"
#include "nsIFrameLoader.h"
#include "nsIMozBrowserFrame.h"
#include "nsIDOMEventListener.h"

#include "nsFrameLoader.h"




class nsGenericHTMLFrameElement : public nsGenericHTMLElement,
                                  public nsIFrameLoaderOwner,
                                  public nsIMozBrowserFrame
{
public:
  nsGenericHTMLFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                            mozilla::dom::FromParser aFromParser)
    : nsGenericHTMLElement(aNodeInfo)
    , mNetworkCreated(aFromParser == mozilla::dom::FROM_PARSER_NETWORK)
    , mBrowserFrameListenersRegistered(false)
    , mFrameLoaderCreationDisallowed(false)
  {
  }

  virtual ~nsGenericHTMLFrameElement();

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
  NS_DECL_NSIFRAMELOADEROWNER
  NS_DECL_NSIDOMMOZBROWSERFRAME
  NS_DECL_NSIMOZBROWSERFRAME

  
  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex);
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual void DestroyContent();

  nsresult CopyInnerTo(mozilla::dom::Element* aDest);

  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;

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

  
  
  
  bool MayBeAppFrame();

  nsRefPtr<nsFrameLoader> mFrameLoader;
  
  
  
  
  
  
  
  nsCOMPtr<mozIApplication> mApp;
  nsCOMPtr<mozIApplication> mContainingApp;

  
  
  
  bool                    mNetworkCreated;

  bool                    mBrowserFrameListenersRegistered;
  bool                    mFrameLoaderCreationDisallowed;
};
