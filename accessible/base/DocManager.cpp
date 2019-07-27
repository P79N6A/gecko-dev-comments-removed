




#include "DocManager.h"

#include "ApplicationAccessible.h"
#include "ARIAMap.h"
#include "DocAccessible-inl.h"
#include "nsAccessibilityService.h"
#include "RootAccessibleWrap.h"

#ifdef A11Y_LOG
#include "Logging.h"
#endif

#include "mozilla/EventListenerManager.h"
#include "mozilla/dom/Event.h" 
#include "nsCURILoader.h"
#include "nsDocShellLoadTypes.h"
#include "nsIChannel.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebNavigation.h"
#include "nsServiceManagerUtils.h"
#include "nsIWebProgress.h"
#include "nsCoreUtils.h"

using namespace mozilla;
using namespace mozilla::a11y;
using namespace mozilla::dom;





DocManager::DocManager()
  : mDocAccessibleCache(2)
{
}




DocAccessible*
DocManager::GetDocAccessible(nsIDocument* aDocument)
{
  if (!aDocument)
    return nullptr;

  
  ApplicationAcc()->EnsureChildren();

  DocAccessible* docAcc = GetExistingDocAccessible(aDocument);
  if (docAcc)
    return docAcc;

  return CreateDocOrRootAccessible(aDocument);
}

Accessible*
DocManager::FindAccessibleInCache(nsINode* aNode) const
{
  nsSearchAccessibleInCacheArg arg;
  arg.mNode = aNode;

  mDocAccessibleCache.EnumerateRead(SearchAccessibleInDocCache,
                                    static_cast<void*>(&arg));

  return arg.mAccessible;
}

#ifdef DEBUG
bool
DocManager::IsProcessingRefreshDriverNotification() const
{
  bool isDocRefreshing = false;
  mDocAccessibleCache.EnumerateRead(SearchIfDocIsRefreshing,
                                    static_cast<void*>(&isDocRefreshing));

  return isDocRefreshing;
}
#endif





bool
DocManager::Init()
{
  nsCOMPtr<nsIWebProgress> progress =
    do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID);

  if (!progress)
    return false;

  progress->AddProgressListener(static_cast<nsIWebProgressListener*>(this),
                                nsIWebProgress::NOTIFY_STATE_DOCUMENT);

  return true;
}

void
DocManager::Shutdown()
{
  nsCOMPtr<nsIWebProgress> progress =
    do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID);

  if (progress)
    progress->RemoveProgressListener(static_cast<nsIWebProgressListener*>(this));

  ClearDocCache();
}




NS_IMPL_ISUPPORTS(DocManager,
                  nsIWebProgressListener,
                  nsIDOMEventListener,
                  nsISupportsWeakReference)




NS_IMETHODIMP
DocManager::OnStateChange(nsIWebProgress* aWebProgress,
                          nsIRequest* aRequest, uint32_t aStateFlags,
                          nsresult aStatus)
{
  NS_ASSERTION(aStateFlags & STATE_IS_DOCUMENT, "Other notifications excluded");

  if (nsAccessibilityService::IsShutdown() || !aWebProgress ||
      (aStateFlags & (STATE_START | STATE_STOP)) == 0)
    return NS_OK;

  nsCOMPtr<nsIDOMWindow> DOMWindow;
  aWebProgress->GetDOMWindow(getter_AddRefs(DOMWindow));
  NS_ENSURE_STATE(DOMWindow);

  nsCOMPtr<nsIDOMDocument> DOMDocument;
  DOMWindow->GetDocument(getter_AddRefs(DOMDocument));
  NS_ENSURE_STATE(DOMDocument);

  nsCOMPtr<nsIDocument> document(do_QueryInterface(DOMDocument));

  
  if (aStateFlags & STATE_STOP) {
#ifdef A11Y_LOG
    if (logging::IsEnabled(logging::eDocLoad))
      logging::DocLoad("document loaded", aWebProgress, aRequest, aStateFlags);
#endif

    
    uint32_t eventType = nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_STOPPED;

    
    
    
    if (NS_SUCCEEDED(aStatus) || !nsCoreUtils::IsContentDocument(document))
      eventType = nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE;

    
    
    
    if (aRequest) {
      uint32_t loadFlags = 0;
      aRequest->GetLoadFlags(&loadFlags);
      if (loadFlags & nsIChannel::LOAD_RETARGETED_DOCUMENT_URI)
        eventType = 0;
    }

    HandleDOMDocumentLoad(document, eventType);
    return NS_OK;
  }

  
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eDocLoad))
    logging::DocLoad("start document loading", aWebProgress, aRequest, aStateFlags);
#endif

  DocAccessible* docAcc = GetExistingDocAccessible(document);
  if (!docAcc)
    return NS_OK;

  nsCOMPtr<nsIWebNavigation> webNav(do_GetInterface(DOMWindow));
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(webNav));
  NS_ENSURE_STATE(docShell);

  bool isReloading = false;
  uint32_t loadType;
  docShell->GetLoadType(&loadType);
  if (loadType == LOAD_RELOAD_NORMAL ||
      loadType == LOAD_RELOAD_BYPASS_CACHE ||
      loadType == LOAD_RELOAD_BYPASS_PROXY ||
      loadType == LOAD_RELOAD_BYPASS_PROXY_AND_CACHE ||
      loadType == LOAD_RELOAD_ALLOW_MIXED_CONTENT) {
    isReloading = true;
  }

  docAcc->NotifyOfLoading(isReloading);
  return NS_OK;
}

NS_IMETHODIMP
DocManager::OnProgressChange(nsIWebProgress* aWebProgress,
                             nsIRequest* aRequest,
                             int32_t aCurSelfProgress,
                             int32_t aMaxSelfProgress,
                             int32_t aCurTotalProgress,
                             int32_t aMaxTotalProgress)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
DocManager::OnLocationChange(nsIWebProgress* aWebProgress,
                             nsIRequest* aRequest, nsIURI* aLocation,
                             uint32_t aFlags)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
DocManager::OnStatusChange(nsIWebProgress* aWebProgress,
                           nsIRequest* aRequest, nsresult aStatus,
                           const char16_t* aMessage)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
DocManager::OnSecurityChange(nsIWebProgress* aWebProgress,
                             nsIRequest* aRequest,
                             uint32_t aState)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}




NS_IMETHODIMP
DocManager::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);

  nsCOMPtr<nsIDocument> document =
    do_QueryInterface(aEvent->InternalDOMEvent()->GetTarget());
  NS_ASSERTION(document, "pagehide or DOMContentLoaded for non document!");
  if (!document)
    return NS_OK;

  if (type.EqualsLiteral("pagehide")) {
    
    
    
    

#ifdef A11Y_LOG
    if (logging::IsEnabled(logging::eDocDestroy))
      logging::DocDestroy("received 'pagehide' event", document);
#endif

    

    
    
    
    DocAccessible* docAccessible = GetExistingDocAccessible(document);
    if (docAccessible)
      docAccessible->Shutdown();

    return NS_OK;
  }

  
  
  if (type.EqualsLiteral("DOMContentLoaded") &&
      nsCoreUtils::IsErrorPage(document)) {
#ifdef A11Y_LOG
    if (logging::IsEnabled(logging::eDocLoad))
      logging::DocLoad("handled 'DOMContentLoaded' event", document);
#endif

    HandleDOMDocumentLoad(document,
                          nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE);
  }

  return NS_OK;
}




void
DocManager::HandleDOMDocumentLoad(nsIDocument* aDocument,
                                  uint32_t aLoadEventType)
{
  
  
  DocAccessible* docAcc = GetExistingDocAccessible(aDocument);
  if (!docAcc) {
    docAcc = CreateDocOrRootAccessible(aDocument);
    if (!docAcc)
      return;
  }

  docAcc->NotifyOfLoad(aLoadEventType);
}

void
DocManager::AddListeners(nsIDocument* aDocument,
                         bool aAddDOMContentLoadedListener)
{
  nsPIDOMWindow* window = aDocument->GetWindow();
  EventTarget* target = window->GetChromeEventHandler();
  EventListenerManager* elm = target->GetOrCreateListenerManager();
  elm->AddEventListenerByType(this, NS_LITERAL_STRING("pagehide"),
                              TrustedEventsAtCapture());

#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eDocCreate))
    logging::Text("added 'pagehide' listener");
#endif

  if (aAddDOMContentLoadedListener) {
    elm->AddEventListenerByType(this, NS_LITERAL_STRING("DOMContentLoaded"),
                                TrustedEventsAtCapture());
#ifdef A11Y_LOG
    if (logging::IsEnabled(logging::eDocCreate))
      logging::Text("added 'DOMContentLoaded' listener");
#endif
  }
}

void
DocManager::RemoveListeners(nsIDocument* aDocument)
{
  nsPIDOMWindow* window = aDocument->GetWindow();
  if (!window)
    return;

  EventTarget* target = window->GetChromeEventHandler();
  if (!target)
    return;

  EventListenerManager* elm = target->GetOrCreateListenerManager();
  elm->RemoveEventListenerByType(this, NS_LITERAL_STRING("pagehide"),
                                 TrustedEventsAtCapture());

  elm->RemoveEventListenerByType(this, NS_LITERAL_STRING("DOMContentLoaded"),
                                 TrustedEventsAtCapture());
}

DocAccessible*
DocManager::CreateDocOrRootAccessible(nsIDocument* aDocument)
{
  
  if (!aDocument->IsVisibleConsideringAncestors() ||
      aDocument->IsResourceDoc() || !aDocument->IsActive())
    return nullptr;

  
  nsIPresShell* presShell = aDocument->GetShell();
  if (!presShell || presShell->IsDestroying())
    return nullptr;

  bool isRootDoc = nsCoreUtils::IsRootDocument(aDocument);

  DocAccessible* parentDocAcc = nullptr;
  if (!isRootDoc) {
    
    
    parentDocAcc = GetDocAccessible(aDocument->GetParentDocument());
    NS_ASSERTION(parentDocAcc,
                 "Can't create an accessible for the document!");
    if (!parentDocAcc)
      return nullptr;
  }

  
  
  nsIContent *rootElm = nsCoreUtils::GetRoleContent(aDocument);
  nsRefPtr<DocAccessible> docAcc = isRootDoc ?
    new RootAccessibleWrap(aDocument, rootElm, presShell) :
    new DocAccessibleWrap(aDocument, rootElm, presShell);

  
  mDocAccessibleCache.Put(aDocument, docAcc);

  
  docAcc->Init();
  docAcc->SetRoleMapEntry(aria::GetRoleMap(aDocument));

  
  if (isRootDoc) {
    if (!ApplicationAcc()->AppendChild(docAcc)) {
      docAcc->Shutdown();
      return nullptr;
    }

    
    
    
    
    
    
    docAcc->FireDelayedEvent(nsIAccessibleEvent::EVENT_REORDER,
                             ApplicationAcc());

  } else {
    parentDocAcc->BindChildDocument(docAcc);
  }

#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eDocCreate)) {
    logging::DocCreate("document creation finished", aDocument);
    logging::Stack();
  }
#endif

  AddListeners(aDocument, isRootDoc);
  return docAcc;
}




PLDHashOperator
DocManager::GetFirstEntryInDocCache(const nsIDocument* aKey,
                                    DocAccessible* aDocAccessible,
                                    void* aUserArg)
{
  NS_ASSERTION(aDocAccessible,
               "No doc accessible for the object in doc accessible cache!");
  *reinterpret_cast<DocAccessible**>(aUserArg) = aDocAccessible;

  return PL_DHASH_STOP;
}

void
DocManager::ClearDocCache()
{
  DocAccessible* docAcc = nullptr;
  while (mDocAccessibleCache.EnumerateRead(GetFirstEntryInDocCache, static_cast<void*>(&docAcc))) {
    if (docAcc)
      docAcc->Shutdown();
  }
}

PLDHashOperator
DocManager::SearchAccessibleInDocCache(const nsIDocument* aKey,
                                       DocAccessible* aDocAccessible,
                                       void* aUserArg)
{
  NS_ASSERTION(aDocAccessible,
               "No doc accessible for the object in doc accessible cache!");

  if (aDocAccessible) {
    nsSearchAccessibleInCacheArg* arg =
      static_cast<nsSearchAccessibleInCacheArg*>(aUserArg);
    arg->mAccessible = aDocAccessible->GetAccessible(arg->mNode);
    if (arg->mAccessible)
      return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

#ifdef DEBUG
PLDHashOperator
DocManager::SearchIfDocIsRefreshing(const nsIDocument* aKey,
                                    DocAccessible* aDocAccessible,
                                    void* aUserArg)
{
  NS_ASSERTION(aDocAccessible,
               "No doc accessible for the object in doc accessible cache!");

  if (aDocAccessible && aDocAccessible->mNotificationController &&
      aDocAccessible->mNotificationController->IsUpdating()) {
    *(static_cast<bool*>(aUserArg)) = true;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}
#endif
