




































#include "nsQtNetworkManager.h"
#include "nsQtNetworkLinkService.h"
#include "nsCOMPtr.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
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
nsQtNetworkLinkService::GetIsLinkUp(PRBool* aIsUp)
{
  *aIsUp = gQtNetworkManager->isOnline();
  return NS_OK;
}

NS_IMETHODIMP
nsQtNetworkLinkService::GetLinkStatusKnown(PRBool* aIsKnown)
{
  *aIsKnown = gQtNetworkManager->isOnline();
  return NS_OK;
}

NS_IMETHODIMP
nsQtNetworkLinkService::Observe(nsISupports* aSubject,
                                const char* aTopic,
                                const PRUnichar* aData)
{
  if (!strcmp(aTopic, "xpcom-shutdown")) {
    Shutdown();
    delete gQtNetworkManager;
    gQtNetworkManager = 0;
  }

  if (!strcmp(aTopic, "browser-lastwindow-close-granted")) {
    Shutdown();
  }

  return NS_OK;
}

nsresult
nsQtNetworkLinkService::Init(void)
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService) {
    return NS_ERROR_FAILURE;
  }

  delete gQtNetworkManager;
  gQtNetworkManager = new nsQtNetworkManager();
  nsresult rv;

  rv = observerService->AddObserver(this, "xpcom-shutdown", PR_FALSE);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  rv = observerService->AddObserver(this, "browser-lastwindow-close-granted", PR_FALSE);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }


  return NS_OK;
}

nsresult
nsQtNetworkLinkService::Shutdown()
{
  gQtNetworkManager->closeSession();
  return NS_OK;
}
