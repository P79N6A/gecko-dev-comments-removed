





#include "OfflineObserver.h"
#include "nsNetUtil.h"
#include "nsIOService.h"
#include "mozilla/net/NeckoCommon.h"
#include "nsIObserverService.h"
#include "nsThreadUtils.h"
namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS(OfflineObserver, nsIObserver)

void
OfflineObserver::RegisterOfflineObserver()
{
  if (NS_IsMainThread()) {
    RegisterOfflineObserverMainThread();
  } else {
    nsRefPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &OfflineObserver::RegisterOfflineObserverMainThread);
    NS_DispatchToMainThread(event);
  }
}

void
OfflineObserver::RemoveOfflineObserver()
{
  if (NS_IsMainThread()) {
    RemoveOfflineObserverMainThread();
  } else {
    nsRefPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &OfflineObserver::RemoveOfflineObserverMainThread);
    NS_DispatchToMainThread(event);
  }
}

void
OfflineObserver::RegisterOfflineObserverMainThread()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService) {
    return;
  }
  nsresult rv = observerService->AddObserver(this,
    NS_IOSERVICE_APP_OFFLINE_STATUS_TOPIC, false);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to register observer");
  }
}

void
OfflineObserver::RemoveOfflineObserverMainThread()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->RemoveObserver(this, NS_IOSERVICE_APP_OFFLINE_STATUS_TOPIC);
  }
}

OfflineObserver::OfflineObserver(DisconnectableParent * parent)
{
  mParent = parent;
  RegisterOfflineObserver();
}

void
OfflineObserver::RemoveObserver()
{
  RemoveOfflineObserver();
  mParent = nullptr;
}

NS_IMETHODIMP
OfflineObserver::Observe(nsISupports *aSubject,
                         const char *aTopic,
                         const char16_t *aData)
{
  if (mParent &&
      !strcmp(aTopic, NS_IOSERVICE_APP_OFFLINE_STATUS_TOPIC)) {
    mParent->OfflineNotification(aSubject);
  }
  return NS_OK;
}

nsresult
DisconnectableParent::OfflineNotification(nsISupports *aSubject)
{
  nsCOMPtr<nsIAppOfflineInfo> info(do_QueryInterface(aSubject));
  if (!info) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  uint32_t targetAppId = NECKO_UNKNOWN_APP_ID;
  info->GetAppId(&targetAppId);

  
  uint32_t appId = GetAppId();
  if (appId != targetAppId) {
    return NS_OK;
  }

  
  if (NS_IsAppOffline(appId)) {
    OfflineDisconnect();
  }

  return NS_OK;
}

} 
} 
