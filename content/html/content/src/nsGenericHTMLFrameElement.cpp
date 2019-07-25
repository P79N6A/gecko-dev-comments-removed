






#include "nsGenericHTMLFrameElement.h"
#include "nsIWebProgress.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMCustomEvent.h"
#include "nsIVariant.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsVariant.h"
#include "nsContentUtils.h"
#include "nsDOMMemoryReporter.h"
#include "nsEventDispatcher.h"
#include "nsContentUtils.h"
#include "nsAsyncDOMEvent.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_CLASS(nsGenericHTMLFrameElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsGenericHTMLFrameElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mFrameLoader, nsIFrameLoader)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_TABLE_HEAD(nsGenericHTMLFrameElement)
  NS_INTERFACE_TABLE_INHERITED3(nsGenericHTMLFrameElement,
                                nsIFrameLoaderOwner,
                                nsIDOMMozBrowserFrame,
                                nsIWebProgressListener)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsGenericHTMLFrameElement)
NS_INTERFACE_MAP_END_INHERITING(nsGenericHTMLElement)

NS_IMPL_INT_ATTR(nsGenericHTMLFrameElement, TabIndex, tabindex)
NS_IMPL_BOOL_ATTR(nsGenericHTMLFrameElement, Mozbrowser, mozbrowser)

nsGenericHTMLFrameElement::~nsGenericHTMLFrameElement()
{
  if (mFrameLoader) {
    mFrameLoader->Destroy();
  }
}

nsresult
nsGenericHTMLFrameElement::GetContentDocument(nsIDOMDocument** aContentDocument)
{
  NS_PRECONDITION(aContentDocument, "Null out param");
  *aContentDocument = nsnull;

  nsCOMPtr<nsIDOMWindow> win;
  GetContentWindow(getter_AddRefs(win));

  if (!win) {
    return NS_OK;
  }

  return win->GetDocument(aContentDocument);
}

nsresult
nsGenericHTMLFrameElement::GetContentWindow(nsIDOMWindow** aContentWindow)
{
  NS_PRECONDITION(aContentWindow, "Null out param");
  *aContentWindow = nsnull;

  nsresult rv = EnsureFrameLoader();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mFrameLoader) {
    return NS_OK;
  }

  bool depthTooGreat = false;
  mFrameLoader->GetDepthTooGreat(&depthTooGreat);
  if (depthTooGreat) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIDocShell> doc_shell;
  mFrameLoader->GetDocShell(getter_AddRefs(doc_shell));

  nsCOMPtr<nsPIDOMWindow> win(do_GetInterface(doc_shell));

  if (!win) {
    return NS_OK;
  }

  NS_ASSERTION(win->IsOuterWindow(),
               "Uh, this window should always be an outer window!");

  return CallQueryInterface(win, aContentWindow);
}

nsresult
nsGenericHTMLFrameElement::EnsureFrameLoader()
{
  if (!GetParent() || !IsInDoc() || mFrameLoader) {
    
    return NS_OK;
  }

  mFrameLoader = nsFrameLoader::Create(this, mNetworkCreated);
  if (!mFrameLoader) {
    
    
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIDocShell> docShell;
  mFrameLoader->GetDocShell(getter_AddRefs(docShell));
  nsCOMPtr<nsIWebProgress> webProgress = do_QueryInterface(docShell);
  NS_ENSURE_TRUE(webProgress, NS_OK);

  
  webProgress->AddProgressListener(this,
    nsIWebProgress::NOTIFY_LOCATION |
    nsIWebProgress::NOTIFY_STATE_WINDOW);

  return NS_OK;
}

NS_IMETHODIMP
nsGenericHTMLFrameElement::GetFrameLoader(nsIFrameLoader **aFrameLoader)
{
  NS_IF_ADDREF(*aFrameLoader = mFrameLoader);
  return NS_OK;
}

NS_IMETHODIMP_(already_AddRefed<nsFrameLoader>)
nsGenericHTMLFrameElement::GetFrameLoader()
{
  nsRefPtr<nsFrameLoader> loader = mFrameLoader;
  return loader.forget();
}

NS_IMETHODIMP
nsGenericHTMLFrameElement::SwapFrameLoaders(nsIFrameLoaderOwner* aOtherOwner)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsGenericHTMLFrameElement::LoadSrc()
{
  nsresult rv = EnsureFrameLoader();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mFrameLoader) {
    return NS_OK;
  }

  rv = mFrameLoader->LoadFrame();
#ifdef DEBUG
  if (NS_FAILED(rv)) {
    NS_WARNING("failed to load URL");
  }
#endif

  return rv;
}

nsresult
nsGenericHTMLFrameElement::BindToTree(nsIDocument* aDocument,
                                      nsIContent* aParent,
                                      nsIContent* aBindingParent,
                                      bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
                 "Missing a script blocker!");
    
    LoadSrc();
  }

  
  
  mNetworkCreated = false;
  return rv;
}

void
nsGenericHTMLFrameElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  if (mFrameLoader) {
    
    
    
    
    
    
    mFrameLoader->Destroy();
    mFrameLoader = nsnull;
  }

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

nsresult
nsGenericHTMLFrameElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                   nsIAtom* aPrefix, const nsAString& aValue,
                                   bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix,
                                              aValue, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::src) {
    
    
    LoadSrc();
  }

  return NS_OK;
}

void
nsGenericHTMLFrameElement::DestroyContent()
{
  if (mFrameLoader) {
    mFrameLoader->Destroy();
    mFrameLoader = nsnull;
  }

  nsGenericHTMLElement::DestroyContent();
}

nsresult
nsGenericHTMLFrameElement::CopyInnerTo(nsGenericElement* aDest) const
{
  nsresult rv = nsGenericHTMLElement::CopyInnerTo(aDest);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIDocument* doc = aDest->OwnerDoc();
  if (doc->IsStaticDocument() && mFrameLoader) {
    nsGenericHTMLFrameElement* dest =
      static_cast<nsGenericHTMLFrameElement*>(aDest);
    nsFrameLoader* fl = nsFrameLoader::Create(dest, false);
    NS_ENSURE_STATE(fl);
    dest->mFrameLoader = fl;
    static_cast<nsFrameLoader*>(mFrameLoader.get())->CreateStaticClone(fl);
  }

  return rv;
}

bool
nsGenericHTMLFrameElement::IsHTMLFocusable(bool aWithMouse,
                                           bool *aIsFocusable,
                                           PRInt32 *aTabIndex)
{
  if (nsGenericHTMLElement::IsHTMLFocusable(aWithMouse, aIsFocusable, aTabIndex)) {
    return true;
  }

  *aIsFocusable = nsContentUtils::IsSubDocumentTabbable(this);

  if (!*aIsFocusable && aTabIndex) {
    *aTabIndex = -1;
  }

  return false;
}

PRInt64
nsGenericHTMLFrameElement::SizeOf() const
{
  PRInt64 size = MemoryReporter::GetBasicSize<nsGenericHTMLFrameElement,
                                              nsGenericHTMLElement>(this);
  
  size += mFrameLoader ? sizeof(*mFrameLoader.get()) : 0;
  return size;
}





bool
nsGenericHTMLFrameElement::BrowserFrameSecurityCheck()
{
  
  if (!Preferences::GetBool("dom.mozBrowserFramesEnabled")) {
    return false;
  }

  
  bool isBrowser = false;
  GetMozbrowser(&isBrowser);
  if (!isBrowser) {
    return false;
  }

  
  nsIPrincipal *principal = NodePrincipal();
  nsCOMPtr<nsIURI> principalURI;
  principal->GetURI(getter_AddRefs(principalURI));
  if (!nsContentUtils::URIIsChromeOrInPref(principalURI,
                                           "dom.mozBrowserFramesWhitelist")) {
    return false;
  }

  
  return true;
}











nsresult
nsGenericHTMLFrameElement::MaybeFireBrowserEvent(
  const nsAString &aEventName,
  const nsAString &aEventType,
  const nsAString &aValue )
{
  MOZ_ASSERT(aEventType.EqualsLiteral("event") ||
             aEventType.EqualsLiteral("customevent"));
  MOZ_ASSERT_IF(aEventType.EqualsLiteral("event"),
                aValue.IsEmpty());

  if (!BrowserFrameSecurityCheck()) {
    return NS_OK;
  }

  nsAutoString eventName;
  eventName.AppendLiteral("mozbrowser");
  eventName.Append(aEventName);

  nsCOMPtr<nsIDOMEvent> domEvent;
  nsEventDispatcher::CreateEvent(GetPresContext(), nsnull,
                                 aEventType, getter_AddRefs(domEvent));

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(domEvent);
  NS_ENSURE_STATE(privateEvent);

  nsresult rv = privateEvent->SetTrusted(true);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aEventType.EqualsLiteral("customevent")) {
    nsCOMPtr<nsIDOMCustomEvent> customEvent = do_QueryInterface(domEvent);
    NS_ENSURE_STATE(customEvent);

    nsCOMPtr<nsIWritableVariant> value = new nsVariant();
    value->SetAsAString(aValue);

    rv = customEvent->InitCustomEvent(eventName,
                                       false,
                                       false,
                                      value);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = domEvent->InitEvent(eventName,
                              false,
                              false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return (new nsAsyncDOMEvent(this, domEvent))->PostDOMEvent();
}

NS_IMETHODIMP
nsGenericHTMLFrameElement::OnLocationChange(nsIWebProgress* aWebProgress,
                                            nsIRequest* aRequest,
                                            nsIURI* aURI,
                                            PRUint32 aFlags)
{
  nsCAutoString spec;
  aURI->GetSpec(spec);

  MaybeFireBrowserEvent(NS_LITERAL_STRING("locationchange"),
                        NS_LITERAL_STRING("customevent"),
                        NS_ConvertUTF8toUTF16(spec));
  return NS_OK;
}

NS_IMETHODIMP
nsGenericHTMLFrameElement::OnStateChange(nsIWebProgress* aProgress,
                                         nsIRequest* aRequest,
                                         PRUint32 aProgressStateFlags,
                                         nsresult aStatus)
{
  if (!(aProgressStateFlags & STATE_IS_WINDOW)) {
    return NS_OK;
  }

  nsAutoString status;
  if (aProgressStateFlags & STATE_START) {
    MaybeFireBrowserEvent(NS_LITERAL_STRING("loadstart"),
                          NS_LITERAL_STRING("event"));
  }
  else if (aProgressStateFlags & STATE_STOP) {
    MaybeFireBrowserEvent(NS_LITERAL_STRING("loadend"),
                          NS_LITERAL_STRING("event"));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGenericHTMLFrameElement::OnProgressChange(nsIWebProgress* aProgress,
                                            nsIRequest* aRequest,
                                            PRInt32 aCurSelfProgress,
                                            PRInt32 aMaxSelfProgress,
                                            PRInt32 aCurTotalProgress,
                                            PRInt32 aMaxTotalProgress)
{
  return NS_OK;
}

NS_IMETHODIMP
nsGenericHTMLFrameElement::OnStatusChange(nsIWebProgress* aWebProgress,
                                          nsIRequest* aRequest,
                                          nsresult aStatus,
                                          const PRUnichar* aMessage)
{
  return NS_OK;
}

NS_IMETHODIMP
nsGenericHTMLFrameElement::OnSecurityChange(nsIWebProgress *aWebProgress,
                                            nsIRequest *aRequest,
                                            PRUint32 state)
{
  return NS_OK;
}
