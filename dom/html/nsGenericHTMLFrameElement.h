





#ifndef nsGenericHTMLFrameElement_h
#define nsGenericHTMLFrameElement_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/nsBrowserElement.h"

#include "nsFrameLoader.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMEventListener.h"
#include "nsIFrameLoader.h"
#include "nsIMozBrowserFrame.h"

class nsXULElement;




class nsGenericHTMLFrameElement : public nsGenericHTMLElement,
                                  public nsIFrameLoaderOwner,
                                  public mozilla::nsBrowserElement,
                                  public nsIMozBrowserFrame
{
public:
  nsGenericHTMLFrameElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                            mozilla::dom::FromParser aFromParser)
    : nsGenericHTMLElement(aNodeInfo)
    , nsBrowserElement()
    , mNetworkCreated(aFromParser == mozilla::dom::FROM_PARSER_NETWORK)
    , mIsPrerendered(false)
    , mBrowserFrameListenersRegistered(false)
    , mFrameLoaderCreationDisallowed(false)
  {
  }

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIFRAMELOADEROWNER
  NS_DECL_NSIDOMMOZBROWSERFRAME
  NS_DECL_NSIMOZBROWSERFRAME

  
  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex) override;
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) override;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify) override;
  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue,
                                bool aNotify) override;
  virtual void DestroyContent() override;

  nsresult CopyInnerTo(mozilla::dom::Element* aDest);

  virtual int32_t TabIndexDefault() override;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsGenericHTMLFrameElement,
                                                     nsGenericHTMLElement)

  void SwapFrameLoaders(nsXULElement& aOtherOwner, mozilla::ErrorResult& aError);

  static bool BrowserFramesEnabled();

  







  static int32_t MapScrollingAttribute(const nsAttrValue* aValue);

protected:
  virtual ~nsGenericHTMLFrameElement();

  
  
  void EnsureFrameLoader();
  nsresult LoadSrc();
  nsIDocument* GetContentDocument();
  nsresult GetContentDocument(nsIDOMDocument** aContentDocument);
  already_AddRefed<nsPIDOMWindow> GetContentWindow();
  nsresult GetContentWindow(nsIDOMWindow** aContentWindow);

  nsRefPtr<nsFrameLoader> mFrameLoader;

  




  bool mNetworkCreated;

  bool mIsPrerendered;
  bool mBrowserFrameListenersRegistered;
  bool mFrameLoaderCreationDisallowed;

private:
  void GetManifestURLByType(nsIAtom *aAppType, nsAString& aOut);
};

#endif 
