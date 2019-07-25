






































#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsHistory.h"
#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWebNavigation.h"
#include "nsIHistoryEntry.h"
#include "nsIURI.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsDOMClassInfo.h"
#include "nsContentUtils.h"
#include "nsISHistoryInternal.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

static const char* sAllowPushStatePrefStr  =
  "browser.history.allowPushState";
static const char* sAllowReplaceStatePrefStr =
  "browser.history.allowReplaceState";




nsHistory::nsHistory(nsPIDOMWindow* aInnerWindow)
  : mInnerWindow(do_GetWeakReference(aInnerWindow))
{
}

nsHistory::~nsHistory()
{
}


DOMCI_DATA(History, nsHistory)


NS_INTERFACE_MAP_BEGIN(nsHistory)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMHistory)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHistory)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(History)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsHistory)
NS_IMPL_RELEASE(nsHistory)


NS_IMETHODIMP
nsHistory::GetLength(PRInt32* aLength)
{
  nsCOMPtr<nsISHistory>   sHistory;

  
  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);
  return sHistory->GetCount(aLength);
}

NS_IMETHODIMP
nsHistory::GetCurrent(nsAString& aCurrent)
{
  if (!nsContentUtils::IsCallerTrustedForRead())
    return NS_ERROR_DOM_SECURITY_ERR;

  PRInt32 curIndex=0;
  nsCAutoString curURL;
  nsCOMPtr<nsISHistory> sHistory;

  
  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);

  
  sHistory->GetIndex(&curIndex);
  nsCOMPtr<nsIHistoryEntry> curEntry;
  nsCOMPtr<nsIURI>     uri;

  
  sHistory->GetEntryAtIndex(curIndex, PR_FALSE, getter_AddRefs(curEntry));
  NS_ENSURE_TRUE(curEntry, NS_ERROR_FAILURE);

  
  curEntry->GetURI(getter_AddRefs(uri));
  NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);
  uri->GetSpec(curURL);
  CopyUTF8toUTF16(curURL, aCurrent);

  return NS_OK;
}

NS_IMETHODIMP
nsHistory::GetPrevious(nsAString& aPrevious)
{
  if (!nsContentUtils::IsCallerTrustedForRead())
    return NS_ERROR_DOM_SECURITY_ERR;

  PRInt32 curIndex;
  nsCAutoString prevURL;
  nsCOMPtr<nsISHistory>  sHistory;

  
  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);

  
  sHistory->GetIndex(&curIndex);
  nsCOMPtr<nsIHistoryEntry> prevEntry;
  nsCOMPtr<nsIURI>     uri;

  
  sHistory->GetEntryAtIndex((curIndex-1), PR_FALSE, getter_AddRefs(prevEntry));
  NS_ENSURE_TRUE(prevEntry, NS_ERROR_FAILURE);

  
  prevEntry->GetURI(getter_AddRefs(uri));
  NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);
  uri->GetSpec(prevURL);
  CopyUTF8toUTF16(prevURL, aPrevious);

  return NS_OK;
}

NS_IMETHODIMP
nsHistory::GetNext(nsAString& aNext)
{
  PRInt32 curIndex;
  nsCAutoString nextURL;
  nsCOMPtr<nsISHistory>  sHistory;

  
  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);

  
  sHistory->GetIndex(&curIndex);
  nsCOMPtr<nsIHistoryEntry> nextEntry;
  nsCOMPtr<nsIURI>     uri;

  
  sHistory->GetEntryAtIndex((curIndex+1), PR_FALSE, getter_AddRefs(nextEntry));
  NS_ENSURE_TRUE(nextEntry, NS_ERROR_FAILURE);

  
  nextEntry->GetURI(getter_AddRefs(uri));
  NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);
  uri->GetSpec(nextURL); 
  CopyUTF8toUTF16(nextURL, aNext);

  return NS_OK;
}

NS_IMETHODIMP
nsHistory::Back()
{
  nsCOMPtr<nsISHistory>  sHistory;

  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(sHistory));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);
  webNav->GoBack();

  return NS_OK;
}

NS_IMETHODIMP
nsHistory::Forward()
{
  nsCOMPtr<nsISHistory>  sHistory;

  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(sHistory));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);
  webNav->GoForward();

  return NS_OK;
}

NS_IMETHODIMP
nsHistory::Go(PRInt32 aDelta)
{
  if (aDelta == 0) {
    nsCOMPtr<nsPIDOMWindow> window(do_GetInterface(GetDocShell()));

    if (window && window->IsHandlingResizeEvent()) {
      
      
      
      
      
      
      

      nsCOMPtr<nsIDocument> doc =
        do_QueryInterface(window->GetExtantDocument());

      nsIPresShell *shell;
      nsPresContext *pcx;
      if (doc && (shell = doc->GetShell()) && (pcx = shell->GetPresContext())) {
        pcx->RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
      }

      return NS_OK;
    }
  }

  nsCOMPtr<nsISHistory> session_history;

  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(session_history));
  NS_ENSURE_TRUE(session_history, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIWebNavigation> webnav(do_QueryInterface(session_history));
  NS_ENSURE_TRUE(webnav, NS_ERROR_FAILURE);

  PRInt32 curIndex=-1;
  PRInt32 len = 0;
  nsresult rv = session_history->GetIndex(&curIndex);
  rv = session_history->GetCount(&len);

  PRInt32 index = curIndex + aDelta;
  if (index > -1  &&  index < len)
    webnav->GotoIndex(index);

  
  
  

  return NS_OK;
}

NS_IMETHODIMP
nsHistory::PushState(nsIVariant *aData, const nsAString& aTitle,
                     const nsAString& aURL, JSContext* aCx)
{
  
  if (!Preferences::GetBool(sAllowPushStatePrefStr, PR_FALSE)) {
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win)
    return NS_ERROR_NOT_AVAILABLE;

  if (!nsContentUtils::CanCallerAccess(win->GetOuterWindow()))
    return NS_ERROR_DOM_SECURITY_ERR;

  
  
  nsCOMPtr<nsIDocShell> docShell = win->GetDocShell();

  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  
  
  nsresult rv = docShell->AddState(aData, aTitle, aURL, PR_FALSE, aCx);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsHistory::ReplaceState(nsIVariant *aData, const nsAString& aTitle,
                        const nsAString& aURL, JSContext* aCx)
{
  
  if (!Preferences::GetBool(sAllowReplaceStatePrefStr, PR_FALSE)) {
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win)
    return NS_ERROR_NOT_AVAILABLE;

  if (!nsContentUtils::CanCallerAccess(win->GetOuterWindow()))
    return NS_ERROR_DOM_SECURITY_ERR;

  
  
  nsCOMPtr<nsIDocShell> docShell = win->GetDocShell();

  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  
  
  return docShell->AddState(aData, aTitle, aURL, PR_TRUE, aCx);
}

NS_IMETHODIMP
nsHistory::GetState(nsIVariant **aState)
{
  *aState = nsnull;

  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win)
    return NS_ERROR_NOT_AVAILABLE;

  if (!nsContentUtils::CanCallerAccess(win->GetOuterWindow()))
    return NS_ERROR_DOM_SECURITY_ERR;

  nsCOMPtr<nsIDocument> doc =
    do_QueryInterface(win->GetExtantDocument());
  if (!doc)
    return NS_ERROR_NOT_AVAILABLE;

  return doc->GetStateObject(aState);
}

NS_IMETHODIMP
nsHistory::Item(PRUint32 aIndex, nsAString& aReturn)
{
  aReturn.Truncate();
  if (!nsContentUtils::IsCallerTrustedForRead()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsresult rv = NS_OK;
  nsCOMPtr<nsISHistory>  session_history;

  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(session_history));
  NS_ENSURE_TRUE(session_history, NS_ERROR_FAILURE);

  nsCOMPtr<nsIHistoryEntry> sh_entry;
  nsCOMPtr<nsIURI> uri;

  rv = session_history->GetEntryAtIndex(aIndex, PR_FALSE,
                                        getter_AddRefs(sh_entry));

  if (sh_entry) {
    rv = sh_entry->GetURI(getter_AddRefs(uri));
  }

  if (uri) {
    nsCAutoString urlCString;
    rv = uri->GetSpec(urlCString);

    CopyUTF8toUTF16(urlCString, aReturn);
  }

  return rv;
}

nsresult
nsHistory::GetSessionHistoryFromDocShell(nsIDocShell * aDocShell, 
                                         nsISHistory ** aReturn)
{

  NS_ENSURE_TRUE(aDocShell, NS_ERROR_FAILURE);
  



  
  
  nsCOMPtr<nsIDocShellTreeItem> dsTreeItem(do_QueryInterface(aDocShell));
  NS_ENSURE_TRUE(dsTreeItem, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIDocShellTreeItem> root;
  dsTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(root));
  NS_ENSURE_TRUE(root, NS_ERROR_FAILURE);
  
  
  nsCOMPtr<nsIWebNavigation>   webNav(do_QueryInterface(root));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);

  
  return webNav->GetSessionHistory(aReturn);
  
}

