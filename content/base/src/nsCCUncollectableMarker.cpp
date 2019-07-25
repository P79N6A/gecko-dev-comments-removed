




































#include "nsCCUncollectableMarker.h"
#include "nsIObserverService.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsServiceManagerUtils.h"
#include "nsIDOMDocument.h"
#include "nsIContentViewer.h"
#include "nsIDocument.h"
#include "nsIWindowMediator.h"
#include "nsPIDOMWindow.h"
#include "nsIWebNavigation.h"
#include "nsISHistory.h"
#include "nsISHEntry.h"
#include "nsISHContainer.h"
#include "nsIWindowWatcher.h"
#include "mozilla/Services.h"

static bool sInited = 0;
PRUint32 nsCCUncollectableMarker::sGeneration = 0;

NS_IMPL_ISUPPORTS1(nsCCUncollectableMarker, nsIObserver)


nsresult
nsCCUncollectableMarker::Init()
{
  if (sInited) {
    return NS_OK;
  }
  
  nsCOMPtr<nsIObserver> marker = new nsCCUncollectableMarker;
  NS_ENSURE_TRUE(marker, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIObserverService> obs =
    mozilla::services::GetObserverService();
  if (!obs)
    return NS_ERROR_FAILURE;

  nsresult rv;

  
  rv = obs->AddObserver(marker, "xpcom-shutdown", false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = obs->AddObserver(marker, "cycle-collector-begin", false);
  NS_ENSURE_SUCCESS(rv, rv);

  sInited = true;

  return NS_OK;
}

void
MarkContentViewer(nsIContentViewer* aViewer)
{
  if (!aViewer) {
    return;
  }

  nsCOMPtr<nsIDOMDocument> domDoc;
  aViewer->GetDOMDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (doc) {
    doc->MarkUncollectableForCCGeneration(nsCCUncollectableMarker::sGeneration);
  }
}

void MarkDocShell(nsIDocShellTreeNode* aNode);

void
MarkSHEntry(nsISHEntry* aSHEntry)
{
  if (!aSHEntry) {
    return;
  }

  nsCOMPtr<nsIContentViewer> cview;
  aSHEntry->GetContentViewer(getter_AddRefs(cview));
  MarkContentViewer(cview);

  nsCOMPtr<nsIDocShellTreeItem> child;
  PRInt32 i = 0;
  while (NS_SUCCEEDED(aSHEntry->ChildShellAt(i++, getter_AddRefs(child))) &&
         child) {
    MarkDocShell(child);
  }

  nsCOMPtr<nsISHContainer> shCont = do_QueryInterface(aSHEntry);
  PRInt32 count;
  shCont->GetChildCount(&count);
  for (i = 0; i < count; ++i) {
    nsCOMPtr<nsISHEntry> childEntry;
    shCont->GetChildAt(i, getter_AddRefs(childEntry));
    MarkSHEntry(childEntry);
  }
  
}

void
MarkDocShell(nsIDocShellTreeNode* aNode)
{
  nsCOMPtr<nsIDocShell> shell = do_QueryInterface(aNode);
  if (!shell) {
    return;
  }

  nsCOMPtr<nsIContentViewer> cview;
  shell->GetContentViewer(getter_AddRefs(cview));
  MarkContentViewer(cview);

  nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(shell);
  nsCOMPtr<nsISHistory> history;
  webNav->GetSessionHistory(getter_AddRefs(history));
  if (history) {
    PRInt32 i, historyCount;
    history->GetCount(&historyCount);
    for (i = 0; i < historyCount; ++i) {
      nsCOMPtr<nsIHistoryEntry> historyEntry;
      history->GetEntryAtIndex(i, false, getter_AddRefs(historyEntry));
      nsCOMPtr<nsISHEntry> shEntry = do_QueryInterface(historyEntry);

      MarkSHEntry(shEntry);
    }
  }

  PRInt32 i, childCount;
  aNode->GetChildCount(&childCount);
  for (i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIDocShellTreeItem> child;
    aNode->GetChildAt(i, getter_AddRefs(child));
    MarkDocShell(child);
  }
}

void
MarkWindowList(nsISimpleEnumerator* aWindowList)
{
  nsCOMPtr<nsISupports> iter;
  while (NS_SUCCEEDED(aWindowList->GetNext(getter_AddRefs(iter))) &&
         iter) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(iter);
    if (window) {
      nsCOMPtr<nsIDocShellTreeNode> rootDocShell =
        do_QueryInterface(window->GetDocShell());

      MarkDocShell(rootDocShell);
    }
  }
}

nsresult
nsCCUncollectableMarker::Observe(nsISupports* aSubject, const char* aTopic,
                                 const PRUnichar* aData)
{
  if (!strcmp(aTopic, "xpcom-shutdown")) {
    nsCOMPtr<nsIObserverService> obs =
      mozilla::services::GetObserverService();
    if (!obs)
      return NS_ERROR_FAILURE;

    
    obs->RemoveObserver(this, "xpcom-shutdown");
    obs->RemoveObserver(this, "cycle-collector-begin");
    
    sGeneration = 0;
    
    return NS_OK;
  }

  NS_ASSERTION(!strcmp(aTopic, "cycle-collector-begin"), "wrong topic");

  
  if (!++sGeneration) {
    ++sGeneration;
  }

  nsresult rv;

  
  nsCOMPtr<nsISimpleEnumerator> windowList;
  nsCOMPtr<nsIWindowMediator> med =
    do_GetService(NS_WINDOWMEDIATOR_CONTRACTID);
  if (med) {
    rv = med->GetEnumerator(nsnull, getter_AddRefs(windowList));
    NS_ENSURE_SUCCESS(rv, rv);

    MarkWindowList(windowList);
  }

  nsCOMPtr<nsIWindowWatcher> ww =
    do_GetService(NS_WINDOWWATCHER_CONTRACTID);
  if (ww) {
    rv = ww->GetWindowEnumerator(getter_AddRefs(windowList));
    NS_ENSURE_SUCCESS(rv, rv);

    MarkWindowList(windowList);
  }

  return NS_OK;
}

