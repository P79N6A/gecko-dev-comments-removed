



#include "TabParent.h"





#ifdef CreateEvent
#undef CreateEvent
#endif

#include "BrowserElementParent.h"
#include "nsHTMLIFrameElement.h"
#include "nsOpenWindowEventDetail.h"
#include "nsEventDispatcher.h"
#include "nsIDOMCustomEvent.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsVariant.h"

using mozilla::dom::Element;
using mozilla::dom::TabParent;

namespace {





already_AddRefed<nsHTMLIFrameElement>
CreateIframe(Element* aOpenerFrameElement, const nsAString& aName, bool aRemote)
{
  nsNodeInfoManager *nodeInfoManager =
    aOpenerFrameElement->OwnerDoc()->NodeInfoManager();

  nsCOMPtr<nsINodeInfo> nodeInfo =
    nodeInfoManager->GetNodeInfo(nsGkAtoms::iframe,
                                  nullptr,
                                 kNameSpaceID_XHTML,
                                 nsIDOMNode::ELEMENT_NODE);

  nsRefPtr<nsHTMLIFrameElement> popupFrameElement =
    static_cast<nsHTMLIFrameElement*>(
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
DispatchOpenWindowEvent(Element* aOpenerFrameElement,
                        Element* aPopupFrameElement,
                        const nsAString& aURL,
                        const nsAString& aName,
                        const nsAString& aFeatures)
{
  
  
  

  
  nsRefPtr<nsOpenWindowEventDetail> detail =
    new nsOpenWindowEventDetail(aURL, aName, aFeatures,
                                aPopupFrameElement->AsDOMNode());
  nsCOMPtr<nsIWritableVariant> detailVariant = new nsVariant();
  nsresult rv = detailVariant->SetAsISupports(detail);
  NS_ENSURE_SUCCESS(rv, false);

  
  nsIPresShell *shell = aOpenerFrameElement->OwnerDoc()->GetShell();
  nsRefPtr<nsPresContext> presContext;
  if (shell) {
    presContext = shell->GetPresContext();
  }

  nsCOMPtr<nsIDOMEvent> domEvent;
  nsEventDispatcher::CreateEvent(presContext, nullptr,
                                 NS_LITERAL_STRING("customevent"),
                                 getter_AddRefs(domEvent));
  NS_ENSURE_TRUE(domEvent, false);

  nsCOMPtr<nsIDOMCustomEvent> customEvent = do_QueryInterface(domEvent);
  NS_ENSURE_TRUE(customEvent, false);
  customEvent->InitCustomEvent(NS_LITERAL_STRING("mozbrowseropenwindow"),
                                true,
                                false,
                               detailVariant);
  customEvent->SetTrusted(true);

  
  nsEventStatus status = nsEventStatus_eIgnore;
  rv = nsEventDispatcher::DispatchDOMEvent(aOpenerFrameElement, nullptr,
                                           domEvent, presContext, &status);
  NS_ENSURE_SUCCESS(rv, false);

  
  
  return aPopupFrameElement->IsInDoc();
}

} 

namespace mozilla {

 bool
BrowserElementParent::OpenWindowOOP(mozilla::dom::TabParent* aOpenerTabParent,
                                    mozilla::dom::TabParent* aPopupTabParent,
                                    const nsAString& aURL,
                                    const nsAString& aName,
                                    const nsAString& aFeatures)
{
  
  nsCOMPtr<Element> openerFrameElement =
    do_QueryInterface(aOpenerTabParent->GetOwnerElement());
  NS_ENSURE_TRUE(openerFrameElement, false);
  nsRefPtr<nsHTMLIFrameElement> popupFrameElement =
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

  nsRefPtr<nsHTMLIFrameElement> popupFrameElement =
    CreateIframe(openerFrameElement, aName,  false);
  NS_ENSURE_TRUE(popupFrameElement, false);

  nsCAutoString spec;
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

} 
