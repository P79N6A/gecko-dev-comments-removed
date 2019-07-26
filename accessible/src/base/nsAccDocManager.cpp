




#include "nsAccDocManager.h"

#include "ApplicationAccessible.h"
#include "DocAccessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsARIAMap.h"
#include "RootAccessibleWrap.h"
#include "States.h"

#ifdef DEBUG
#include "Logging.h"
#endif

#include "nsCURILoader.h"
#include "nsDocShellLoadTypes.h"
#include "nsIChannel.h"
#include "nsIContentViewer.h"
#include "nsIDOMDocument.h"
#include "nsEventListenerManager.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebNavigation.h"
#include "nsServiceManagerUtils.h"

using namespace mozilla::a11y;








DocAccessible*
nsAccDocManager::GetDocAccessible(nsIDocument *aDocument)
{
  if (!aDocument)
    return nullptr;

  
  nsAccessNode::GetApplicationAccessible()->EnsureChildren();

  DocAccessible* docAcc = mDocAccessibleCache.GetWeak(aDocument);
  if (docAcc)
    return docAcc;

  return CreateDocOrRootAccessible(aDocument);
}

Accessible*
nsAccDocManager::FindAccessibleInCache(nsINode* aNode) const
{
  nsSearchAccessibleInCacheArg arg;
  arg.mNode = aNode;

  mDocAccessibleCache.EnumerateRead(SearchAccessibleInDocCache,
                                    static_cast<void*>(&arg));

  return arg.mAccessible;
}

#ifdef DEBUG
bool
nsAccDocManager::IsProcessingRefreshDriverNotification() const
{
  bool isDocRefreshing = false;
  mDocAccessibleCache.EnumerateRead(SearchIfDocIsRefreshing,
                                    static_cast<void*>(&isDocRefreshing));

  return isDocRefreshing;
}
#endif





bool
nsAccDocManager::Init()
{
  mDocAccessibleCache.Init(4);

  nsCOMPtr<nsIWebProgress> progress =
    do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID);

  if (!progress)
    return false;

  progress->AddProgressListener(static_cast<nsIWebProgressListener*>(this),
                                nsIWebProgress::NOTIFY_STATE_DOCUMENT);

  return true;
}

void
nsAccDocManager::Shutdown()
{
  nsCOMPtr<nsIWebProgress> progress =
    do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID);

  if (progress)
    progress->RemoveProgressListener(static_cast<nsIWebProgressListener*>(this));

  ClearDocCache();
}




NS_IMPL_THREADSAFE_ISUPPORTS3(nsAccDocManager,
                              nsIWebProgressListener,
                              nsIDOMEventListener,
                              nsISupportsWeakReference)




NS_IMETHODIMP
nsAccDocManager::OnStateChange(nsIWebProgress *aWebProgress,
                               nsIRequest *aRequest, PRUint32 aStateFlags,
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
#ifdef DEBUG
    if (logging::IsEnabled(logging::eDocLoad))
      logging::DocLoad("document loaded", aWebProgress, aRequest, aStateFlags);
#endif

    
    PRUint32 eventType = nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_STOPPED;

    
    
    
    if (NS_SUCCEEDED(aStatus) || !nsCoreUtils::IsContentDocument(document))
      eventType = nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE;

    
    
    
    if (aRequest) {
      PRUint32 loadFlags = 0;
      aRequest->GetLoadFlags(&loadFlags);
      if (loadFlags & nsIChannel::LOAD_RETARGETED_DOCUMENT_URI)
        eventType = 0;
    }

    HandleDOMDocumentLoad(document, eventType);
    return NS_OK;
  }

  
#ifdef DEBUG
  if (logging::IsEnabled(logging::eDocLoad))
    logging::DocLoad("start document loading", aWebProgress, aRequest, aStateFlags);
#endif

  DocAccessible* docAcc = mDocAccessibleCache.GetWeak(document);
  if (!docAcc)
    return NS_OK;

  nsCOMPtr<nsIWebNavigation> webNav(do_GetInterface(DOMWindow));
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(webNav));
  NS_ENSURE_STATE(docShell);

  bool isReloading = false;
  PRUint32 loadType;
  docShell->GetLoadType(&loadType);
  if (loadType == LOAD_RELOAD_NORMAL ||
      loadType == LOAD_RELOAD_BYPASS_CACHE ||
      loadType == LOAD_RELOAD_BYPASS_PROXY ||
      loadType == LOAD_RELOAD_BYPASS_PROXY_AND_CACHE) {
    isReloading = true;
  }

  docAcc->NotifyOfLoading(isReloading);
  return NS_OK;
}

NS_IMETHODIMP
nsAccDocManager::OnProgressChange(nsIWebProgress *aWebProgress,
                                  nsIRequest *aRequest,
                                  PRInt32 aCurSelfProgress,
                                  PRInt32 aMaxSelfProgress,
                                  PRInt32 aCurTotalProgress,
                                  PRInt32 aMaxTotalProgress)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsAccDocManager::OnLocationChange(nsIWebProgress *aWebProgress,
                                  nsIRequest *aRequest, nsIURI *aLocation,
                                  PRUint32 aFlags)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsAccDocManager::OnStatusChange(nsIWebProgress *aWebProgress,
                                nsIRequest *aRequest, nsresult aStatus,
                                const PRUnichar *aMessage)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsAccDocManager::OnSecurityChange(nsIWebProgress *aWebProgress,
                                  nsIRequest *aRequest,
                                  PRUint32 aState)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}




NS_IMETHODIMP
nsAccDocManager::HandleEvent(nsIDOMEvent *aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);

  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetTarget(getter_AddRefs(target));

  nsCOMPtr<nsIDocument> document(do_QueryInterface(target));
  NS_ASSERTION(document, "pagehide or DOMContentLoaded for non document!");
  if (!document)
    return NS_OK;

  if (type.EqualsLiteral("pagehide")) {
    
    
    
    

#ifdef DEBUG
    if (logging::IsEnabled(logging::eDocDestroy))
      logging::DocDestroy("received 'pagehide' event", document);
#endif

    
    
    if (document->IsInitialDocument())
      return NS_OK;

    

    
    
    
    DocAccessible* docAccessible = mDocAccessibleCache.GetWeak(document);
    if (docAccessible)
      docAccessible->Shutdown();

    return NS_OK;
  }

  
  
  if (type.EqualsLiteral("DOMContentLoaded") &&
      nsCoreUtils::IsErrorPage(document)) {
#ifdef DEBUG
    if (logging::IsEnabled(logging::eDocLoad))
      logging::DocLoad("handled 'DOMContentLoaded' event", document);
#endif

    HandleDOMDocumentLoad(document,
                          nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE);
  }

  return NS_OK;
}




void
nsAccDocManager::HandleDOMDocumentLoad(nsIDocument *aDocument,
                                       PRUint32 aLoadEventType)
{
  
  
  DocAccessible* docAcc = mDocAccessibleCache.GetWeak(aDocument);
  if (!docAcc) {
    docAcc = CreateDocOrRootAccessible(aDocument);
    if (!docAcc)
      return;
  }

  docAcc->NotifyOfLoad(aLoadEventType);
}

void
nsAccDocManager::AddListeners(nsIDocument *aDocument,
                              bool aAddDOMContentLoadedListener)
{
  nsPIDOMWindow *window = aDocument->GetWindow();
  nsIDOMEventTarget *target = window->GetChromeEventHandler();
  nsEventListenerManager* elm = target->GetListenerManager(true);
  elm->AddEventListenerByType(this, NS_LITERAL_STRING("pagehide"),
                              NS_EVENT_FLAG_CAPTURE);

#ifdef DEBUG
  if (logging::IsEnabled(logging::eDocCreate))
    logging::Text("added 'pagehide' listener");
#endif

  if (aAddDOMContentLoadedListener) {
    elm->AddEventListenerByType(this, NS_LITERAL_STRING("DOMContentLoaded"),
                                NS_EVENT_FLAG_CAPTURE);
#ifdef DEBUG
    if (logging::IsEnabled(logging::eDocCreate))
      logging::Text("added 'DOMContentLoaded' listener");
#endif
  }
}

DocAccessible*
nsAccDocManager::CreateDocOrRootAccessible(nsIDocument* aDocument)
{
  
  
  if (aDocument->IsInitialDocument() || !aDocument->IsVisible() ||
      aDocument->IsResourceDoc() || !aDocument->IsActive())
    return nullptr;

  
  nsIPresShell* presShell = aDocument->GetShell();
  if (!presShell || !presShell->GetRootFrame())
    return nullptr;

  
  
  nsIContent *rootElm = nsCoreUtils::GetRoleContent(aDocument);
  if (!rootElm)
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

  
  
  nsRefPtr<DocAccessible> docAcc = isRootDoc ?
    new RootAccessibleWrap(aDocument, rootElm, presShell) :
    new DocAccessibleWrap(aDocument, rootElm, presShell);

  
  mDocAccessibleCache.Put(aDocument, docAcc);

  
  docAcc->Init();
  docAcc->SetRoleMapEntry(aria::GetRoleMap(aDocument));

  
  if (isRootDoc) {
    Accessible* appAcc = nsAccessNode::GetApplicationAccessible();
    if (!appAcc->AppendChild(docAcc)) {
      docAcc->Shutdown();
      return nullptr;
    }

    
    
    
    
    nsRefPtr<AccEvent> reorderEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_REORDER, appAcc, eAutoDetect,
                   AccEvent::eCoalesceFromSameSubtree);
    docAcc->FireDelayedAccessibleEvent(reorderEvent);

  } else {
    parentDocAcc->BindChildDocument(docAcc);
  }

#ifdef DEBUG
  if (logging::IsEnabled(logging::eDocCreate)) {
    logging::DocCreate("document creation finished", aDocument);
    logging::Stack();
  }
#endif

  AddListeners(aDocument, isRootDoc);
  return docAcc;
}




PLDHashOperator
nsAccDocManager::GetFirstEntryInDocCache(const nsIDocument* aKey,
                                         DocAccessible* aDocAccessible,
                                         void* aUserArg)
{
  NS_ASSERTION(aDocAccessible,
               "No doc accessible for the object in doc accessible cache!");
  *reinterpret_cast<DocAccessible**>(aUserArg) = aDocAccessible;

  return PL_DHASH_STOP;
}

void
nsAccDocManager::ClearDocCache()
{
  DocAccessible* docAcc = nullptr;
  while (mDocAccessibleCache.EnumerateRead(GetFirstEntryInDocCache, static_cast<void*>(&docAcc))) {
    if (docAcc)
      docAcc->Shutdown();
  }
}

PLDHashOperator
nsAccDocManager::SearchAccessibleInDocCache(const nsIDocument* aKey,
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
nsAccDocManager::SearchIfDocIsRefreshing(const nsIDocument* aKey,
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
