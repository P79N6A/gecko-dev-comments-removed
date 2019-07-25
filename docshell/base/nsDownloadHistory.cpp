






































#include "nsDownloadHistory.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsIGlobalHistory.h"
#include "nsIGlobalHistory2.h"
#include "nsIObserverService.h"
#include "nsIURI.h"
#include "nsIComponentRegistrar.h"
#include "nsDocShellCID.h"




NS_IMPL_ISUPPORTS1(nsDownloadHistory, nsIDownloadHistory)




NS_IMETHODIMP
nsDownloadHistory::AddDownload(nsIURI *aSource,
                               nsIURI *aReferrer,
                               PRTime aStartTime,
                               nsIURI *aDestination)
{
  NS_ENSURE_ARG_POINTER(aSource);

  nsCOMPtr<nsIGlobalHistory2> history =
    do_GetService("@mozilla.org/browser/global-history;2");
  if (!history)
    return NS_ERROR_NOT_AVAILABLE;

  bool visited;
  nsresult rv = history->IsVisited(aSource, &visited);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = history->AddURI(aSource, PR_FALSE, PR_TRUE, aReferrer);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!visited) {
    nsCOMPtr<nsIObserverService> os =
      do_GetService("@mozilla.org/observer-service;1");
    if (os)
      os->NotifyObservers(aSource, NS_LINK_VISITED_EVENT_TOPIC, nsnull);
  }

  return NS_OK;
}
