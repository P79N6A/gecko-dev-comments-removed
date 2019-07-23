





































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




nsHistory::nsHistory(nsIDocShell* aDocShell) : mDocShell(aDocShell)
{
}

nsHistory::~nsHistory()
{
}



NS_INTERFACE_MAP_BEGIN(nsHistory)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMHistory)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHistory)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHistory)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(History)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsHistory)
NS_IMPL_RELEASE(nsHistory)


void
nsHistory::SetDocShell(nsIDocShell *aDocShell)
{
  mDocShell = aDocShell; 
}

NS_IMETHODIMP
nsHistory::GetLength(PRInt32* aLength)
{
  nsCOMPtr<nsISHistory>   sHistory;

  
  GetSessionHistoryFromDocShell(mDocShell, getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);
  return sHistory->GetCount(aLength);
}

NS_IMETHODIMP
nsHistory::GetCurrent(nsAString& aCurrent)
{
  PRInt32 curIndex=0;
  nsCAutoString curURL;
  nsCOMPtr<nsISHistory> sHistory;

  
  GetSessionHistoryFromDocShell(mDocShell, getter_AddRefs(sHistory));
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
  PRInt32 curIndex;
  nsCAutoString prevURL;
  nsCOMPtr<nsISHistory>  sHistory;

  
  GetSessionHistoryFromDocShell(mDocShell, getter_AddRefs(sHistory));
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

  
  GetSessionHistoryFromDocShell(mDocShell, getter_AddRefs(sHistory));
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

  GetSessionHistoryFromDocShell(mDocShell, getter_AddRefs(sHistory));
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

  GetSessionHistoryFromDocShell(mDocShell, getter_AddRefs(sHistory));
  NS_ENSURE_TRUE(sHistory, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(sHistory));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);
  webNav->GoForward();

  return NS_OK;
}

NS_IMETHODIMP
nsHistory::Go(PRInt32 aDelta)
{
  nsCOMPtr<nsISHistory> session_history;

  GetSessionHistoryFromDocShell(mDocShell, getter_AddRefs(session_history));
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
nsHistory::Go()
{
  nsCOMPtr<nsIXPCNativeCallContext> ncc;
  nsresult rv = nsContentUtils::XPConnect()->
    GetCurrentNativeCallContext(getter_AddRefs(ncc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_NOT_AVAILABLE;

  PRUint32 argc;
  ncc->GetArgc(&argc);

  PRInt32 delta = 0;

  if (argc > 0) {
    jsval *argv = nsnull;

    ncc->GetArgvPtr(&argv);
    NS_ENSURE_TRUE(argv, NS_ERROR_UNEXPECTED);

    if (!JSVAL_IS_INT(argv[0])) {
      

      return NS_OK;
    }

    delta = JSVAL_TO_INT(argv[0]);
  }

  if (delta == 0) {
    nsCOMPtr<nsPIDOMWindow> window(do_GetInterface(mDocShell));

    if (window && window->IsHandlingResizeEvent()) {
      
      
      
      
      
      
      

      nsCOMPtr<nsIDocument> doc =
        do_QueryInterface(window->GetExtantDocument());

      nsIPresShell *shell;
      nsPresContext *pcx;
      if (doc && (shell = doc->GetShellAt(0)) &&
          (pcx = shell->GetPresContext())) {
        pcx->ClearStyleDataAndReflow();
      }

      return NS_OK;
    }
  }

  return Go(delta);
}

NS_IMETHODIMP
nsHistory::Item(PRUint32 aIndex, nsAString& aReturn)
{
  aReturn.Truncate();

  nsresult rv = NS_OK;
  nsCOMPtr<nsISHistory>  session_history;

  GetSessionHistoryFromDocShell(mDocShell, getter_AddRefs(session_history));
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
  



  
  
  nsCOMPtr<nsIDocShellTreeItem> dsTreeItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(dsTreeItem, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIDocShellTreeItem> root;
  dsTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(root));
  NS_ENSURE_TRUE(root, NS_ERROR_FAILURE);
  
  
  nsCOMPtr<nsIWebNavigation>   webNav(do_QueryInterface(root));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);

  
  return webNav->GetSessionHistory(aReturn);
  
}

