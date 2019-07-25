




































#include "nsQtNetworkLinkService.h"
#include "nsCOMPtr.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsQtNetworkManager.h"
#include "mozilla/Services.h"

NS_IMPL_ISUPPORTS2(nsQtNetworkLinkService,
                   nsINetworkLinkService,
                   nsIObserver)

nsQtNetworkLinkService::nsQtNetworkLinkService()
{
}

nsQtNetworkLinkService::~nsQtNetworkLinkService()
{
}

NS_IMETHODIMP
nsQtNetworkLinkService::GetIsLinkUp(PRBool *aIsUp)
{
  *aIsUp = nsQtNetworkManager::IsConnected();
  return NS_OK;
}

NS_IMETHODIMP
nsQtNetworkLinkService::GetLinkStatusKnown(PRBool *aIsKnown)
{
  *aIsKnown = nsQtNetworkManager::GetLinkStatusKnown();
  return NS_OK;
}

NS_IMETHODIMP
nsQtNetworkLinkService::Observe(nsISupports* aSubject,
                                const char* aTopic,
                                const PRUnichar* aData)
{
  if (!strcmp(aTopic, "xpcom-shutdown"))
    Shutdown();

  return NS_OK;
}

nsresult
nsQtNetworkLinkService::Init(void)
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  nsresult rv = observerService->AddObserver(this, "xpcom-shutdown", PR_FALSE);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  if (!nsQtNetworkManager::Startup())
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult
nsQtNetworkLinkService::Shutdown()
{
  nsQtNetworkManager::Shutdown();
  return NS_OK;
}
