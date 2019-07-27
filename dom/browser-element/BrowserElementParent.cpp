



#include "TabParent.h"





#ifdef CreateEvent
#undef CreateEvent
#endif

#include "BrowserElementParent.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/dom/HTMLIFrameElement.h"
#include "mozilla/dom/ToJSValue.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsVariant.h"
#include "mozilla/dom/BrowserElementDictionariesBinding.h"
#include "mozilla/dom/CustomEvent.h"

using namespace mozilla;
using namespace mozilla::dom;

namespace {

using mozilla::BrowserElementParent;




already_AddRefed<HTMLIFrameElement>
CreateIframe(Element* aOpenerFrameElement, const nsAString& aName, bool aRemote)
{
  nsNodeInfoManager *nodeInfoManager =
    aOpenerFrameElement->OwnerDoc()->NodeInfoManager();

  nsRefPtr<NodeInfo> nodeInfo =
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

  
  if (aOpenerFrameElement->HasAttr(kNameSpaceID_None, nsGkAtoms::parentapp)) {
    nsAutoString parentApp;
    aOpenerFrameElement->GetAttr(kNameSpaceID_None, nsGkAtoms::parentapp,
                                 parentApp);
    popupFrameElement->SetAttr(kNameSpaceID_None, nsGkAtoms::parentapp,
                               parentApp,  false);
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
                       JSContext* cx, JS::Handle<JS::Value> aDetailValue,
                       nsEventStatus *aStatus)
{
  NS_ENSURE_TRUE(aFrameElement, false);
  nsIPresShell *shell = aFrameElement->OwnerDoc()->GetShell();
  nsRefPtr<nsPresContext> presContext;
  if (shell) {
    presContext = shell->GetPresContext();
  }

  nsCOMPtr<nsIDOMEvent> domEvent;
  EventDispatcher::CreateEvent(aFrameElement, presContext, nullptr,
                               NS_LITERAL_STRING("customevent"),
                               getter_AddRefs(domEvent));
  NS_ENSURE_TRUE(domEvent, false);

  nsCOMPtr<nsIDOMCustomEvent> customEvent = do_QueryInterface(domEvent);
  NS_ENSURE_TRUE(customEvent, false);
  ErrorResult res;
  CustomEvent* event = static_cast<CustomEvent*>(customEvent.get());
  event->InitCustomEvent(cx,
                         aEventName,
                          true,
                          true,
                         aDetailValue,
                         res);
  if (res.Failed()) {
    return false;
  }
  customEvent->SetTrusted(true);
  
  *aStatus = nsEventStatus_eConsumeNoDefault;
  nsresult rv =
    EventDispatcher::DispatchDOMEvent(aFrameElement, nullptr,
                                      domEvent, presContext, aStatus);
  return NS_SUCCEEDED(rv);
}

} 

namespace mozilla {









BrowserElementParent::OpenWindowResult
BrowserElementParent::DispatchOpenWindowEvent(Element* aOpenerFrameElement,
                        Element* aPopupFrameElement,
                        const nsAString& aURL,
                        const nsAString& aName,
                        const nsAString& aFeatures)
{
  
  
  

  
  OpenWindowEventDetail detail;
  detail.mUrl = aURL;
  detail.mName = aName;
  detail.mFeatures = aFeatures;
  detail.mFrameElement = aPopupFrameElement;

  AutoJSContext cx;
  JS::Rooted<JS::Value> val(cx);

  nsIGlobalObject* sgo = aPopupFrameElement->OwnerDoc()->GetScopeObject();
  if (!sgo) {
    return BrowserElementParent::OPEN_WINDOW_IGNORED;
  }

  JS::Rooted<JSObject*> global(cx, sgo->GetGlobalJSObject());
  JSAutoCompartment ac(cx, global);
  if (!ToJSValue(cx, detail, &val)) {
    MOZ_CRASH("Failed to convert dictionary to JS::Value due to OOM.");
    return BrowserElementParent::OPEN_WINDOW_IGNORED;
  }

  nsEventStatus status;
  bool dispatchSucceeded =
    DispatchCustomDOMEvent(aOpenerFrameElement,
                           NS_LITERAL_STRING("mozbrowseropenwindow"),
                           cx,
                           val, &status);

  if (dispatchSucceeded) {
    if (aPopupFrameElement->IsInDoc()) {
      return BrowserElementParent::OPEN_WINDOW_ADDED;
    } else if (status == nsEventStatus_eConsumeNoDefault) {
      
      
      return BrowserElementParent::OPEN_WINDOW_CANCELLED;
    }
  }

  return BrowserElementParent::OPEN_WINDOW_IGNORED;
}


BrowserElementParent::OpenWindowResult
BrowserElementParent::OpenWindowOOP(TabParent* aOpenerTabParent,
                                    TabParent* aPopupTabParent,
                                    const nsAString& aURL,
                                    const nsAString& aName,
                                    const nsAString& aFeatures)
{
  
  nsCOMPtr<Element> openerFrameElement = aOpenerTabParent->GetOwnerElement();
  NS_ENSURE_TRUE(openerFrameElement,
                 BrowserElementParent::OPEN_WINDOW_IGNORED);
  nsRefPtr<HTMLIFrameElement> popupFrameElement =
    CreateIframe(openerFrameElement, aName,  true);

  
  
  
  
  
  
  
  
  
  
  popupFrameElement->DisallowCreateFrameLoader();

  OpenWindowResult opened =
    DispatchOpenWindowEvent(openerFrameElement, popupFrameElement,
                            aURL, aName, aFeatures);

  if (opened != BrowserElementParent::OPEN_WINDOW_ADDED) {
    return opened;
  }

  
  
  aPopupTabParent->SetOwnerElement(popupFrameElement);
  popupFrameElement->AllowCreateFrameLoader();
  popupFrameElement->CreateRemoteFrameLoader(aPopupTabParent);
  return opened;
}


BrowserElementParent::OpenWindowResult
BrowserElementParent::OpenWindowInProcess(nsIDOMWindow* aOpenerWindow,
                                          nsIURI* aURI,
                                          const nsAString& aName,
                                          const nsACString& aFeatures,
                                          nsIDOMWindow** aReturnWindow)
{
  *aReturnWindow = nullptr;

  
  
  
  
  
  
  
  
  nsCOMPtr<nsIDOMWindow> topWindow;
  aOpenerWindow->GetScriptableTop(getter_AddRefs(topWindow));

  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(topWindow);

  nsCOMPtr<Element> openerFrameElement = win->GetFrameElementInternal();
  NS_ENSURE_TRUE(openerFrameElement, BrowserElementParent::OPEN_WINDOW_IGNORED);


  nsRefPtr<HTMLIFrameElement> popupFrameElement =
    CreateIframe(openerFrameElement, aName,  false);
  NS_ENSURE_TRUE(popupFrameElement, BrowserElementParent::OPEN_WINDOW_IGNORED);

  nsAutoCString spec;
  if (aURI) {
    aURI->GetSpec(spec);
  }

  OpenWindowResult opened =
    DispatchOpenWindowEvent(openerFrameElement, popupFrameElement,
                            NS_ConvertUTF8toUTF16(spec),
                            aName,
                            NS_ConvertUTF8toUTF16(aFeatures));

  if (opened != BrowserElementParent::OPEN_WINDOW_ADDED) {
    return opened;
  }

  
  nsCOMPtr<nsIFrameLoader> frameLoader;
  popupFrameElement->GetFrameLoader(getter_AddRefs(frameLoader));
  NS_ENSURE_TRUE(frameLoader, BrowserElementParent::OPEN_WINDOW_IGNORED);

  nsCOMPtr<nsIDocShell> docshell;
  frameLoader->GetDocShell(getter_AddRefs(docshell));
  NS_ENSURE_TRUE(docshell, BrowserElementParent::OPEN_WINDOW_IGNORED);

  nsCOMPtr<nsIDOMWindow> window = docshell->GetWindow();
  window.forget(aReturnWindow);

  return !!*aReturnWindow ? opened : BrowserElementParent::OPEN_WINDOW_CANCELLED;
}

class DispatchAsyncScrollEventRunnable : public nsRunnable
{
public:
  DispatchAsyncScrollEventRunnable(TabParent* aTabParent,
                                   const CSSRect& aContentRect,
                                   const CSSSize& aContentSize)
    : mTabParent(aTabParent)
    , mContentRect(aContentRect)
    , mContentSize(aContentSize)
  {}

  NS_IMETHOD Run();

private:
  nsRefPtr<TabParent> mTabParent;
  const CSSRect mContentRect;
  const CSSSize mContentSize;
};

NS_IMETHODIMP DispatchAsyncScrollEventRunnable::Run()
{
  nsCOMPtr<Element> frameElement = mTabParent->GetOwnerElement();
  NS_ENSURE_STATE(frameElement);
  nsIDocument *doc = frameElement->OwnerDoc();
  nsCOMPtr<nsIGlobalObject> globalObject = doc->GetScopeObject();
  NS_ENSURE_TRUE(globalObject, NS_ERROR_UNEXPECTED);

  
  AsyncScrollEventDetail detail;
  detail.mLeft = mContentRect.x;
  detail.mTop = mContentRect.y;
  detail.mWidth = mContentRect.width;
  detail.mHeight = mContentRect.height;
  detail.mScrollWidth = mContentRect.width;
  detail.mScrollHeight = mContentRect.height;

  AutoSafeJSContext cx;
  JS::Rooted<JSObject*> globalJSObject(cx, globalObject->GetGlobalJSObject());
  NS_ENSURE_TRUE(globalJSObject, NS_ERROR_UNEXPECTED);

  JSAutoCompartment ac(cx, globalJSObject);
  JS::Rooted<JS::Value> val(cx);

  if (!ToJSValue(cx, detail, &val)) {
    MOZ_CRASH("Failed to convert dictionary to JS::Value due to OOM.");
    return NS_ERROR_FAILURE;
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  DispatchCustomDOMEvent(frameElement,
                         NS_LITERAL_STRING("mozbrowserasyncscroll"),
                         cx,
                         val, &status);
  return NS_OK;
}

bool
BrowserElementParent::DispatchAsyncScrollEvent(TabParent* aTabParent,
                                               const CSSRect& aContentRect,
                                               const CSSSize& aContentSize)
{
  nsRefPtr<DispatchAsyncScrollEventRunnable> runnable =
    new DispatchAsyncScrollEventRunnable(aTabParent, aContentRect,
                                         aContentSize);
  return NS_SUCCEEDED(NS_DispatchToMainThread(runnable));
}

} 
