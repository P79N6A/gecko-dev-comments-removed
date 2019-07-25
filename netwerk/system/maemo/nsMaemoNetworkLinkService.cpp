





































#include "nsMaemoNetworkLinkService.h"
#include "nsCOMPtr.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsMaemoNetworkManager.h"
#include "mozilla/Services.h"
#include "nsCRT.h"

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
nsMaemoNetworkLinkService::GetIsLinkUp(bool *aIsUp)
{
  *aIsUp = nsMaemoNetworkManager::IsConnected();
  return NS_OK;
}

NS_IMETHODIMP
nsMaemoNetworkLinkService::GetLinkStatusKnown(bool *aIsKnown)
{
  *aIsKnown = nsMaemoNetworkManager::GetLinkStatusKnown();
  return NS_OK;
}

NS_IMETHODIMP
nsMaemoNetworkLinkService::GetLinkType(PRUint32 *aLinkType)
{
  NS_ENSURE_ARG_POINTER(aLinkType);

  
  *aLinkType = nsINetworkLinkService::LINK_TYPE_UNKNOWN;
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
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  nsresult rv = observerService->AddObserver(this, "xpcom-shutdown", PR_FALSE);
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
