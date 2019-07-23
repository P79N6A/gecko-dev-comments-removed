
























#include "nsMaemoNetworkLinkService.h"
#include "nsCOMPtr.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsMaemoNetworkManager.h"

NS_IMPL_ISUPPORTS2(nsMaemoNetworkLinkService,
                   nsINetworkLinkService,
                   nsIObserver)

nsMaemoNetworkLinkService::nsMaemoNetworkLinkService()
{
}

nsMaemoNetworkLinkService::~nsMaemoNetworkLinkService()
{
}

NS_IMETHODIMP
nsMaemoNetworkLinkService::GetIsLinkUp(PRBool *aIsUp)
{
  *aIsUp = nsMaemoNetworkManager::IsConnected();
  return NS_OK;
}

NS_IMETHODIMP
nsMaemoNetworkLinkService::GetLinkStatusKnown(PRBool *aIsKnown)
{
  *aIsKnown = nsMaemoNetworkManager::GetLinkStatusKnown();
  return NS_OK;
}

NS_IMETHODIMP
nsMaemoNetworkLinkService::Observe(nsISupports *aSubject,
                                   const char *aTopic,
                                   const PRUnichar *aData)
{
  if (!strcmp(aTopic, "xpcom-shutdown"))
    Shutdown();

  return NS_OK;
}

nsresult
nsMaemoNetworkLinkService::Init(void)
{
  nsresult rv;

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "xpcom-shutdown", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!nsMaemoNetworkManager::Startup())
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult
nsMaemoNetworkLinkService::Shutdown()
{
  nsMaemoNetworkManager::Shutdown();
  return NS_OK;
}
