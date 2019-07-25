





































#include "nsAccDocManager.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsApplicationAccessible.h"
#include "nsOuterDocAccessible.h"
#include "nsRootAccessibleWrap.h"

#include "nsCURILoader.h"
#include "nsDocShellLoadTypes.h"
#include "nsIChannel.h"
#include "nsIContentViewer.h"
#include "nsIDOMDocument.h"
#include "nsIEventListenerManager.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebNavigation.h"
#include "nsServiceManagerUtils.h"








nsDocAccessible*
nsAccDocManager::GetDocAccessible(nsIDocument *aDocument)
{
  if (!aDocument)
    return nsnull;

  nsDocAccessible *docAcc =
    mDocAccessibleCache.GetWeak(static_cast<void*>(aDocument));
  if (docAcc)
    return docAcc;

  return CreateDocOrRootAccessible(aDocument);
}

nsAccessible*
nsAccDocManager::FindAccessibleInCache(void *aUniqueID) const
{
  nsSearchAccessibleInCacheArg arg;
    arg.mUniqueID = aUniqueID;

  mDocAccessibleCache.EnumerateRead(SearchAccessibleInDocCache,
                                    static_cast<void*>(&arg));

  return arg.mAccessible;
}

void
nsAccDocManager::ShutdownDocAccessiblesInTree(nsIDocument *aDocument)
{
  nsCOMPtr<nsISupports> container = aDocument->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(container);
  ShutdownDocAccessiblesInTree(treeItem, aDocument);
}





PRBool
nsAccDocManager::Init()
{
  mDocAccessibleCache.Init(4);

  nsCOMPtr<nsIWebProgress> progress =
    do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID);

  if (!progress)
    return PR_FALSE;

  progress->AddProgressListener(static_cast<nsIWebProgressListener*>(this),
                                nsIWebProgress::NOTIFY_STATE_DOCUMENT);

  return PR_TRUE;
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

void
nsAccDocManager::ShutdownDocAccessible(nsIDocument *aDocument)
{
  nsDocAccessible* docAccessible =
    mDocAccessibleCache.GetWeak(static_cast<void*>(aDocument));
  if (!docAccessible)
    return;

  
  
  

  docAccessible->Shutdown();
  mDocAccessibleCache.Remove(static_cast<void*>(aDocument));
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
    NS_LOG_ACCDOCLOAD("document loaded", aWebProgress, aRequest, aStateFlags)

    
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

  
  NS_LOG_ACCDOCLOAD("start document loading", aWebProgress, aRequest,
                    aStateFlags)

  if (!IsEventTargetDocument(document))
    return NS_OK;

  nsDocAccessible *docAcc =
    mDocAccessibleCache.GetWeak(static_cast<void*>(document));
  if (!docAcc)
    return NS_OK;

  nsCOMPtr<nsIWebNavigation> webNav(do_GetInterface(DOMWindow));
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(webNav));
  NS_ENSURE_STATE(docShell);

  
  
  
  PRUint32 loadType;
  docShell->GetLoadType(&loadType);
  if (loadType == LOAD_RELOAD_NORMAL ||
      loadType == LOAD_RELOAD_BYPASS_CACHE ||
      loadType == LOAD_RELOAD_BYPASS_PROXY ||
      loadType == LOAD_RELOAD_BYPASS_PROXY_AND_CACHE) {

    
    nsRefPtr<nsAccEvent> reloadEvent =
      new nsAccEvent(nsIAccessibleEvent::EVENT_DOCUMENT_RELOAD, docAcc);
    nsEventShell::FireEvent(reloadEvent);
  }

  
  
  nsRefPtr<nsAccEvent> stateEvent =
    new nsAccStateChangeEvent(document, nsIAccessibleStates::STATE_BUSY,
                              PR_FALSE, PR_TRUE);
  docAcc->FireDelayedAccessibleEvent(stateEvent);

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
                                  nsIRequest *aRequest, nsIURI *aLocation)
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
    
    
    
    

    NS_LOG_ACCDOCDESTROY("received 'pagehide' event", document)

    
    
    if (document->IsInitialDocument())
      return NS_OK;

    
    ShutdownDocAccessiblesInTree(document);
    return NS_OK;
  }

  
  
  if (type.EqualsLiteral("DOMContentLoaded") &&
      nsCoreUtils::IsErrorPage(document)) {
    NS_LOG_ACCDOCLOAD2("handled 'DOMContentLoaded' event", document)
    HandleDOMDocumentLoad(document,
                          nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE,
                          PR_TRUE);
  }

  return NS_OK;
}




void
nsAccDocManager::HandleDOMDocumentLoad(nsIDocument *aDocument,
                                       PRUint32 aLoadEventType,
                                       PRBool aMarkAsLoaded)
{
  
  
  nsDocAccessible *docAcc =
    mDocAccessibleCache.GetWeak(static_cast<void*>(aDocument));

  if (!docAcc) {
    docAcc = CreateDocOrRootAccessible(aDocument);
    NS_ASSERTION(docAcc, "Can't create document accessible!");
    if (!docAcc)
      return;
  }

  if (aMarkAsLoaded)
    docAcc->MarkAsLoaded();

  
  
  
  
  
  
  if (!IsEventTargetDocument(aDocument)) {
    
    
    
    if (!nsCoreUtils::IsRootDocument(aDocument)) {
      docAcc->InvalidateCacheSubtree(nsnull,
                                     nsIAccessibilityService::NODE_SIGNIFICANT_CHANGE);
    }
    return;
  }

  
  if (aLoadEventType) {
    nsRefPtr<nsAccEvent> loadEvent = new nsAccEvent(aLoadEventType, aDocument);
    docAcc->FireDelayedAccessibleEvent(loadEvent);
  }

  
  nsRefPtr<nsAccEvent> stateEvent =
    new nsAccStateChangeEvent(aDocument, nsIAccessibleStates::STATE_BUSY,
                              PR_FALSE, PR_FALSE);
  docAcc->FireDelayedAccessibleEvent(stateEvent);
}

PRBool
nsAccDocManager::IsEventTargetDocument(nsIDocument *aDocument) const
{
  nsCOMPtr<nsISupports> container = aDocument->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    do_QueryInterface(container);
  NS_ASSERTION(docShellTreeItem, "No document shell for document!");

  nsCOMPtr<nsIDocShellTreeItem> parentTreeItem;
  docShellTreeItem->GetParent(getter_AddRefs(parentTreeItem));

  
  if (parentTreeItem) {
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    docShellTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));

    
    return (sameTypeRoot == docShellTreeItem);
  }

  
  PRInt32 contentType;
  docShellTreeItem->GetItemType(&contentType);
  return (contentType == nsIDocShellTreeItem::typeContent);
}

void
nsAccDocManager::AddListeners(nsIDocument *aDocument,
                              PRBool aAddDOMContentLoadedListener)
{
  nsPIDOMWindow *window = aDocument->GetWindow();
  nsPIDOMEventTarget *target = window->GetChromeEventHandler();
  nsIEventListenerManager* elm = target->GetListenerManager(PR_TRUE);
  elm->AddEventListenerByType(this, NS_LITERAL_STRING("pagehide"),
                              NS_EVENT_FLAG_CAPTURE, nsnull);

  NS_LOG_ACCDOCCREATE_TEXT("  added 'pagehide' listener")

  if (aAddDOMContentLoadedListener) {
    elm->AddEventListenerByType(this, NS_LITERAL_STRING("DOMContentLoaded"),
                                NS_EVENT_FLAG_CAPTURE, nsnull);
    NS_LOG_ACCDOCCREATE_TEXT("  added 'DOMContentLoaded' listener")
  }
}

nsDocAccessible*
nsAccDocManager::CreateDocOrRootAccessible(nsIDocument *aDocument)
{
  
  if (aDocument->IsInitialDocument() || !aDocument->IsVisible() ||
      aDocument->GetDisplayDocument())
    return nsnull;

  
  nsIPresShell *presShell = aDocument->GetShell();
  if (!presShell)
    return nsnull;

  
  
  nsIContent *rootElm = nsCoreUtils::GetRoleContent(aDocument);
  if (!rootElm)
    return nsnull;

  PRBool isRootDoc = nsCoreUtils::IsRootDocument(aDocument);

  
  
  nsAccessible *outerDocAcc = nsnull;
  if (isRootDoc) {
    outerDocAcc = nsAccessNode::GetApplicationAccessible();

  } else {
    nsIDocument* parentDoc = aDocument->GetParentDocument();
    if (!parentDoc)
      return nsnull;

    nsIContent* ownerContent = parentDoc->FindContentForSubDocument(aDocument);
    if (!ownerContent)
      return nsnull;

    
    
    
    
    
    outerDocAcc = GetAccService()->GetAccessible(ownerContent);
  }

  if (!outerDocAcc)
    return nsnull;

  
  
  nsCOMPtr<nsIWeakReference> weakShell(do_GetWeakReference(presShell));
  nsDocAccessible *docAcc = isRootDoc ?
    new nsRootAccessibleWrap(aDocument, rootElm, weakShell) :
    new nsDocAccessibleWrap(aDocument, rootElm, weakShell);

  if (!docAcc)
    return nsnull;

  
  if (!mDocAccessibleCache.Put(static_cast<void*>(aDocument), docAcc)) {
    delete docAcc;
    return nsnull;
  }

  
  
  
  if (!outerDocAcc->AppendChild(docAcc) ||
      !GetAccService()->InitAccessible(docAcc, nsAccUtils::GetRoleMapEntry(aDocument))) {
    mDocAccessibleCache.Remove(static_cast<void*>(aDocument));
    return nsnull;
  }

  NS_LOG_ACCDOCCREATE("document creation finished", aDocument)

  AddListeners(aDocument, isRootDoc);
  return docAcc;
}

void
nsAccDocManager::ShutdownDocAccessiblesInTree(nsIDocShellTreeItem *aTreeItem,
                                              nsIDocument *aDocument)
{
  nsCOMPtr<nsIDocShellTreeNode> treeNode(do_QueryInterface(aTreeItem));

  if (treeNode) {
    PRInt32 subDocumentsCount = 0;
    treeNode->GetChildCount(&subDocumentsCount);
    for (PRInt32 idx = 0; idx < subDocumentsCount; idx++) {
      nsCOMPtr<nsIDocShellTreeItem> treeItemChild;
      treeNode->GetChildAt(idx, getter_AddRefs(treeItemChild));
      NS_ASSERTION(treeItemChild, "No tree item when there should be");
      if (!treeItemChild)
        continue;

      nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(treeItemChild));
      nsCOMPtr<nsIContentViewer> contentViewer;
      docShell->GetContentViewer(getter_AddRefs(contentViewer));
      if (!contentViewer)
        continue;

      ShutdownDocAccessiblesInTree(treeItemChild, contentViewer->GetDocument());
    }
  }

  ShutdownDocAccessible(aDocument);
}




PLDHashOperator
nsAccDocManager::ClearDocCacheEntry(const void* aKey,
                                    nsRefPtr<nsDocAccessible>& aDocAccessible,
                                    void* aUserArg)
{
  NS_ASSERTION(aDocAccessible,
               "Calling ClearDocCacheEntry with a NULL pointer!");

  if (aDocAccessible)
    aDocAccessible->Shutdown();

  return PL_DHASH_REMOVE;
}

PLDHashOperator
nsAccDocManager::SearchAccessibleInDocCache(const void* aKey,
                                            nsDocAccessible* aDocAccessible,
                                            void* aUserArg)
{
  NS_ASSERTION(aDocAccessible,
               "No doc accessible for the object in doc accessible cache!");

  if (aDocAccessible) {
    nsSearchAccessibleInCacheArg* arg =
      static_cast<nsSearchAccessibleInCacheArg*>(aUserArg);
    arg->mAccessible = aDocAccessible->GetCachedAccessible(arg->mUniqueID);
    if (arg->mAccessible)
      return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}
