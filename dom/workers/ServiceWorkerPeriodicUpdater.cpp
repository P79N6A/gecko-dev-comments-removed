





#include "ServiceWorkerPeriodicUpdater.h"
#include "mozilla/ClearOnShutdown.h"

#define OBSERVER_TOPIC_IDLE_DAILY "idle-daily"

namespace mozilla {
namespace dom {
namespace workers {

NS_IMPL_ISUPPORTS(ServiceWorkerPeriodicUpdater, nsIObserver)

StaticRefPtr<ServiceWorkerPeriodicUpdater>
ServiceWorkerPeriodicUpdater::sInstance;

already_AddRefed<ServiceWorkerPeriodicUpdater>
ServiceWorkerPeriodicUpdater::GetSingleton()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  if (!sInstance) {
    sInstance = new ServiceWorkerPeriodicUpdater();
    ClearOnShutdown(&sInstance);
  }
  nsRefPtr<ServiceWorkerPeriodicUpdater> copy(sInstance.get());
  return copy.forget();
}

ServiceWorkerPeriodicUpdater::ServiceWorkerPeriodicUpdater()
{
}

ServiceWorkerPeriodicUpdater::~ServiceWorkerPeriodicUpdater()
{
}

NS_IMETHODIMP
ServiceWorkerPeriodicUpdater::Observe(nsISupports* aSubject,
                                      const char* aTopic,
                                      const char16_t* aData)
{
  if (strcmp(aTopic, OBSERVER_TOPIC_IDLE_DAILY) == 0) {
    
  }

  return NS_OK;
}

} 
} 
} 
