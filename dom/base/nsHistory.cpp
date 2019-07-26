





#include "nsHistory.h"

#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIDocShell.h"
#include "nsIWebNavigation.h"
#include "nsIHistoryEntry.h"
#include "nsIURI.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsDOMClassInfoID.h"
#include "nsError.h"
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
nsHistory::GetLength(int32_t* aLength)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !nsContentUtils::CanCallerAccess(win->GetOuterWindow()))
    return NS_ERROR_DOM_SECURITY_ERR;

  nsCOMPtr<nsISHistory>   sHistory;

  
  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);
  return sHistory->GetCount(aLength);
}

NS_IMETHODIMP
nsHistory::GetCurrent(nsAString& aCurrent)
{
  if (!nsContentUtils::IsCallerChrome())
    return NS_ERROR_DOM_SECURITY_ERR;

  int32_t curIndex=0;
  nsAutoCString curURL;
  nsCOMPtr<nsISHistory> sHistory;

  
  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);

  
  sHistory->GetIndex(&curIndex);
  nsCOMPtr<nsIHistoryEntry> curEntry;
  nsCOMPtr<nsIURI>     uri;

  
  sHistory->GetEntryAtIndex(curIndex, false, getter_AddRefs(curEntry));
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
  if (!nsContentUtils::IsCallerChrome())
    return NS_ERROR_DOM_SECURITY_ERR;

  int32_t curIndex;
  nsAutoCString prevURL;
  nsCOMPtr<nsISHistory>  sHistory;

  
  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);

  
  sHistory->GetIndex(&curIndex);
  nsCOMPtr<nsIHistoryEntry> prevEntry;
  nsCOMPtr<nsIURI>     uri;

  
  sHistory->GetEntryAtIndex((curIndex-1), false, getter_AddRefs(prevEntry));
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
  if (!nsContentUtils::IsCallerChrome())
    return NS_ERROR_DOM_SECURITY_ERR;

  int32_t curIndex;
  nsAutoCString nextURL;
  nsCOMPtr<nsISHistory>  sHistory;

  
  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);

  
  sHistory->GetIndex(&curIndex);
  nsCOMPtr<nsIHistoryEntry> nextEntry;
  nsCOMPtr<nsIURI>     uri;

  
  sHistory->GetEntryAtIndex((curIndex+1), false, getter_AddRefs(nextEntry));
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
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !nsContentUtils::CanCallerAccess(win->GetOuterWindow()))
    return NS_ERROR_DOM_SECURITY_ERR;

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
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !nsContentUtils::CanCallerAccess(win->GetOuterWindow()))
    return NS_ERROR_DOM_SECURITY_ERR;

  nsCOMPtr<nsISHistory>  sHistory;

  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(sHistory));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);
  webNav->GoForward();

  return NS_OK;
}

NS_IMETHODIMP
nsHistory::Go(int32_t aDelta)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !nsContentUtils::CanCallerAccess(win->GetOuterWindow()))
    return NS_ERROR_DOM_SECURITY_ERR;

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

  int32_t curIndex=-1;
  int32_t len = 0;
  session_history->GetIndex(&curIndex);
  session_history->GetCount(&len);

  int32_t index = curIndex + aDelta;
  if (index > -1  &&  index < len)
    webnav->GotoIndex(index);

  
  
  

  return NS_OK;
}

NS_IMETHODIMP
nsHistory::PushState(nsIVariant *aData, const nsAString& aTitle,
                     const nsAString& aURL, JSContext* aCx)
{
  
  if (!Preferences::GetBool(sAllowPushStatePrefStr, false)) {
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win)
    return NS_ERROR_NOT_AVAILABLE;

  if (!nsContentUtils::CanCallerAccess(win->GetOuterWindow()))
    return NS_ERROR_DOM_SECURITY_ERR;

  
  
  nsCOMPtr<nsIDocShell> docShell = win->GetDocShell();

  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  
  
  nsresult rv = docShell->AddState(aData, aTitle, aURL, false, aCx);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsHistory::ReplaceState(nsIVariant *aData, const nsAString& aTitle,
                        const nsAString& aURL, JSContext* aCx)
{
  
  if (!Preferences::GetBool(sAllowReplaceStatePrefStr, false)) {
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win)
    return NS_ERROR_NOT_AVAILABLE;

  if (!nsContentUtils::CanCallerAccess(win->GetOuterWindow()))
    return NS_ERROR_DOM_SECURITY_ERR;

  
  
  nsCOMPtr<nsIDocShell> docShell = win->GetDocShell();

  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  
  
  return docShell->AddState(aData, aTitle, aURL, true, aCx);
}

NS_IMETHODIMP
nsHistory::GetState(nsIVariant **aState)
{
  *aState = nullptr;

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
nsHistory::Item(uint32_t aIndex, nsAString& aReturn)
{
  aReturn.Truncate();
  if (!nsContentUtils::IsCallerChrome()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsresult rv = NS_OK;
  nsCOMPtr<nsISHistory>  session_history;

  GetSessionHistoryFromDocShell(GetDocShell(), getter_AddRefs(session_history));
  NS_ENSURE_TRUE(session_history, NS_ERROR_FAILURE);

  nsCOMPtr<nsIHistoryEntry> sh_entry;
  nsCOMPtr<nsIURI> uri;

  rv = session_history->GetEntryAtIndex(aIndex, false,
                                        getter_AddRefs(sh_entry));

  if (sh_entry) {
    rv = sh_entry->GetURI(getter_AddRefs(uri));
  }

  if (uri) {
    nsAutoCString urlCString;
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
  



  
  
  NS_ENSURE_TRUE(aDocShell, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIDocShellTreeItem> root;
  aDocShell->GetSameTypeRootTreeItem(getter_AddRefs(root));
  NS_ENSURE_TRUE(root, NS_ERROR_FAILURE);
  
  
  nsCOMPtr<nsIWebNavigation>   webNav(do_QueryInterface(root));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);

  
  return webNav->GetSessionHistory(aReturn);
  
}

