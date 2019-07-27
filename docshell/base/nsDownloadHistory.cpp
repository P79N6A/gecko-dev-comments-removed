





#include "nsDownloadHistory.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsIGlobalHistory2.h"
#include "nsIObserverService.h"
#include "nsIURI.h"




NS_IMPL_ISUPPORTS(nsDownloadHistory, nsIDownloadHistory)




NS_IMETHODIMP
nsDownloadHistory::AddDownload(nsIURI* aSource,
                               nsIURI* aReferrer,
                               PRTime aStartTime,
                               nsIURI* aDestination)
{
  NS_ENSURE_ARG_POINTER(aSource);

  nsCOMPtr<nsIGlobalHistory2> history =
    do_GetService("@mozilla.org/browser/global-history;2");
  if (!history) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  bool visited;
  nsresult rv = history->IsVisited(aSource, &visited);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = history->AddURI(aSource, false, true, aReferrer);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!visited) {
    nsCOMPtr<nsIObserverService> os =
      do_GetService("@mozilla.org/observer-service;1");
    if (os) {
      os->NotifyObservers(aSource, NS_LINK_VISITED_EVENT_TOPIC, nullptr);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadHistory::RemoveAllDownloads()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
