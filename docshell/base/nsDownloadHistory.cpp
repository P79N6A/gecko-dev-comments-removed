






































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

nsresult
nsDownloadHistory::RegisterSelf(nsIComponentManager *aCompMgr,
                                nsIFile *aPath,
                                const char *aLoaderStr,
                                const char *aType,
                                const nsModuleComponentInfo *aInfo)
{
  nsCOMPtr<nsIComponentRegistrar> compReg(do_QueryInterface(aCompMgr));
  if (!compReg)
    return NS_ERROR_UNEXPECTED;
  
  PRBool registered;
  nsresult rv = compReg->IsContractIDRegistered(NS_DOWNLOADHISTORY_CONTRACTID,
                                                &registered);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  if (registered) {
    return compReg->RegisterFactoryLocation(GetCID(), "nsDownloadHistory",
                                            nsnull, aPath, aLoaderStr, aType);
  }

  return compReg->RegisterFactoryLocation(GetCID(), "nsDownloadHistory",
                                          NS_DOWNLOADHISTORY_CONTRACTID,
                                          aPath, aLoaderStr, aType);
}




NS_IMETHODIMP
nsDownloadHistory::AddDownload(nsIURI *aSource,
                               nsIURI *aReferrer,
                               PRTime aStartTime)
{
  NS_ENSURE_ARG_POINTER(aSource);

  nsCOMPtr<nsIGlobalHistory2> history =
    do_GetService("@mozilla.org/browser/global-history;2");
  if (!history)
    return NS_ERROR_NOT_AVAILABLE;

  PRBool visited;
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
