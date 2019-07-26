



#include "TabParent.h"





#ifdef CreateEvent
#undef CreateEvent
#endif

#include "BrowserElementParent.h"
#include "mozilla/dom/HTMLIFrameElement.h"
#include "nsOpenWindowEventDetail.h"
#include "nsEventDispatcher.h"
#include "nsIDOMCustomEvent.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsVariant.h"
#include "nsAsyncScrollEventDetail.h"

using mozilla::dom::Element;
using mozilla::dom::HTMLIFrameElement;
using mozilla::dom::TabParent;

namespace {





already_AddRefed<HTMLIFrameElement>
CreateIframe(Element* aOpenerFrameElement, const nsAString& aName, bool aRemote)
{
  nsNodeInfoManager *nodeInfoManager =
    aOpenerFrameElement->OwnerDoc()->NodeInfoManager();

  nsCOMPtr<nsINodeInfo> nodeInfo =
    nodeInfoManager->GetNodeInfo(nsGkAtoms::iframe,
                                  nullptr,
                                 kNameSpaceID_XHTML,
                                 nsIDOMNode::ELEMENT_NODE);

  nsRefPtr<HTMLIFrameElement> popupFrameElement =
    static_cast<HTMLIFrameElement*>(
      NS_NewHTMLIFrameElement(nodeInfo.forget(), mozilla::dom::NOT_FROM_PARSER));

  popupFrameElement->SetMozbrowser(true);

  
  if (aOpenerFrameElement->HasAttr(kNameSpaceID_None, nsGkAtoms::mozapp)) {
    nsAutoString mozapp;
    aOpenerFrameElement->GetAttr(kNameSpaceID_None, nsGkAtoms::mozapp, mozapp);
    popupFrameElement->SetAttr(kNameSpaceID_None, nsGkAtoms::mozapp,
                               mozapp,  false);
  }

  
  popupFrameElement->SetAttr(kNameSpaceID_None, nsGkAtoms::name,
                             aName,  false);

  
  popupFrameElement->SetAttr(kNameSpaceID_None, nsGkAtoms::Remote,
                             aRemote ? NS_LITERAL_STRING("true") :
                                       NS_LITERAL_STRING("false"),
                              false);

  return popupFrameElement.forget();
}

bool
DispatchCustomDOMEvent(Element* aFrameElement, const nsAString& aEventName,
                       nsISupports *aDetailValue)
{
  NS_ENSURE_TRUE(aFrameElement, false);
  nsIPresShell *shell = aFrameElement->OwnerDoc()->GetShell();
  nsRefPtr<nsPresContext> presContext;
  if (shell) {
    presContext = shell->GetPresContext();
  }

  nsCOMPtr<nsIDOMEvent> domEvent;
  nsEventDispatcher::CreateEvent(presContext, nullptr,
                                 NS_LITERAL_STRING("customevent"),
                                 getter_AddRefs(domEvent));
  NS_ENSURE_TRUE(domEvent, false);

  nsCOMPtr<nsIWritableVariant> detailVariant = new nsVariant();
  nsresult rv = detailVariant->SetAsISupports(aDetailValue);
  NS_ENSURE_SUCCESS(rv, false);
  nsCOMPtr<nsIDOMCustomEvent> customEvent = do_QueryInterface(domEvent);
  NS_ENSURE_TRUE(customEvent, false);
  customEvent->InitCustomEvent(aEventName,
                                true,
                                false,
                               detailVariant);
  customEvent->SetTrusted(true);
  
  nsEventStatus status = nsEventStatus_eIgnore;
  rv = nsEventDispatcher::DispatchDOMEvent(aFrameElement, nullptr,
                                           domEvent, presContext, &status);
  return NS_SUCCEEDED(rv);
}








bool
DispatchOpenWindowEvent(Element* aOpenerFrameElement,
                        Element* aPopupFrameElement,
                        const nsAString& aURL,
                        const nsAString& aName,
                        const nsAString& aFeatures)
{
  
  
  

  
  nsRefPtr<nsOpenWindowEventDetail> detail =
    new nsOpenWindowEventDetail(aURL, aName, aFeatures,
                                aPopupFrameElement->AsDOMNode());

  bool dispatchSucceeded =
    DispatchCustomDOMEvent(aOpenerFrameElement,
                           NS_LITERAL_STRING("mozbrowseropenwindow"),
                           detail);

  
  
  return (dispatchSucceeded && aPopupFrameElement->IsInDoc());
}

} 

namespace mozilla {

 bool
BrowserElementParent::OpenWindowOOP(TabParent* aOpenerTabParent,
                                    TabParent* aPopupTabParent,
                                    const nsAString& aURL,
                                    const nsAString& aName,
                                    const nsAString& aFeatures)
{
  
  nsCOMPtr<Element> openerFrameElement =
    do_QueryInterface(aOpenerTabParent->GetOwnerElement());
  NS_ENSURE_TRUE(openerFrameElement, false);
  nsRefPtr<HTMLIFrameElement> popupFrameElement =
    CreateIframe(openerFrameElement, aName,  true);

  
  
  
  
  
  
  
  
  
  
  popupFrameElement->DisallowCreateFrameLoader();

  bool dispatchSucceeded =
    DispatchOpenWindowEvent(openerFrameElement, popupFrameElement,
                            aURL, aName, aFeatures);
  if (!dispatchSucceeded) {
    return false;
  }

  
  
  aPopupTabParent->SetOwnerElement(popupFrameElement);
  popupFrameElement->AllowCreateFrameLoader();
  popupFrameElement->CreateRemoteFrameLoader(aPopupTabParent);

  return true;
}

 bool
BrowserElementParent::OpenWindowInProcess(nsIDOMWindow* aOpenerWindow,
                                          nsIURI* aURI,
                                          const nsAString& aName,
                                          const nsACString& aFeatures,
                                          nsIDOMWindow** aReturnWindow)
{
  *aReturnWindow = NULL;

  
  
  
  
  
  
  
  
  nsCOMPtr<nsIDOMWindow> topWindow;
  aOpenerWindow->GetScriptableTop(getter_AddRefs(topWindow));

  nsCOMPtr<nsIDOMElement> openerFrameDOMElement;
  topWindow->GetFrameElement(getter_AddRefs(openerFrameDOMElement));
  NS_ENSURE_TRUE(openerFrameDOMElement, false);

  nsCOMPtr<Element> openerFrameElement =
    do_QueryInterface(openerFrameDOMElement);

  nsRefPtr<HTMLIFrameElement> popupFrameElement =
    CreateIframe(openerFrameElement, aName,  false);
  NS_ENSURE_TRUE(popupFrameElement, false);

  nsAutoCString spec;
  if (aURI) {
    aURI->GetSpec(spec);
  }
  bool dispatchSucceeded =
    DispatchOpenWindowEvent(openerFrameElement, popupFrameElement,
                            NS_ConvertUTF8toUTF16(spec),
                            aName,
                            NS_ConvertUTF8toUTF16(aFeatures));
  if (!dispatchSucceeded) {
    return false;
  }

  
  nsCOMPtr<nsIFrameLoader> frameLoader;
  popupFrameElement->GetFrameLoader(getter_AddRefs(frameLoader));
  NS_ENSURE_TRUE(frameLoader, false);

  nsCOMPtr<nsIDocShell> docshell;
  frameLoader->GetDocShell(getter_AddRefs(docshell));
  NS_ENSURE_TRUE(docshell, false);

  nsCOMPtr<nsIDOMWindow> window = do_GetInterface(docshell);
  window.forget(aReturnWindow);
  return !!*aReturnWindow;
}

class DispatchAsyncScrollEventRunnable : public nsRunnable
{
public:
  DispatchAsyncScrollEventRunnable(TabParent* aTabParent,
                                   const gfx::Rect& aContentRect,
                                   const gfx::Size& aContentSize)
    : mTabParent(aTabParent)
    , mContentRect(aContentRect)
    , mContentSize(aContentSize)
  {}

  NS_IMETHOD Run();

private:
  nsRefPtr<TabParent> mTabParent;
  const gfx::Rect mContentRect;
  const gfx::Size mContentSize;
};

NS_IMETHODIMP DispatchAsyncScrollEventRunnable::Run()
{
  nsIDOMElement* element = mTabParent->GetOwnerElement();
  nsCOMPtr<Element> frameElement = do_QueryInterface(element);
  
  nsRefPtr<nsAsyncScrollEventDetail> detail =
    new nsAsyncScrollEventDetail(mContentRect.x, mContentRect.y,
                                 mContentRect.width, mContentRect.height,
                                 mContentSize.width, mContentSize.height);
  DispatchCustomDOMEvent(frameElement,
                         NS_LITERAL_STRING("mozbrowserasyncscroll"),
                         detail);
  return NS_OK;
}

bool
BrowserElementParent::DispatchAsyncScrollEvent(TabParent* aTabParent,
                                               const gfx::Rect& aContentRect,
                                               const gfx::Size& aContentSize)
{
  nsRefPtr<DispatchAsyncScrollEventRunnable> runnable =
    new DispatchAsyncScrollEventRunnable(aTabParent, aContentRect,
                                         aContentSize);
  return NS_SUCCEEDED(NS_DispatchToMainThread(runnable));
}

} 
