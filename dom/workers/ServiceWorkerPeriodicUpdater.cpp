





#include "ServiceWorkerPeriodicUpdater.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/unused.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/ContentParent.h"
#include "nsIServiceWorkerManager.h"

#define OBSERVER_TOPIC_IDLE_DAILY "idle-daily"

namespace mozilla {
namespace dom {
namespace workers {

NS_IMPL_ISUPPORTS(ServiceWorkerPeriodicUpdater, nsIObserver)

StaticRefPtr<ServiceWorkerPeriodicUpdater>
ServiceWorkerPeriodicUpdater::sInstance;
bool
ServiceWorkerPeriodicUpdater::sPeriodicUpdatesEnabled = true;

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
  Preferences::AddBoolVarCache(&sPeriodicUpdatesEnabled,
                               "dom.serviceWorkers.periodic-updates.enabled",
                               true);
}

ServiceWorkerPeriodicUpdater::~ServiceWorkerPeriodicUpdater()
{
}

NS_IMETHODIMP
ServiceWorkerPeriodicUpdater::Observe(nsISupports* aSubject,
                                      const char* aTopic,
                                      const char16_t* aData)
{
  if (strcmp(aTopic, OBSERVER_TOPIC_IDLE_DAILY) == 0 &&
      sPeriodicUpdatesEnabled) {
    
    nsCOMPtr<nsIServiceWorkerManager> swm =
      mozilla::services::GetServiceWorkerManager();
    if (swm) {
        swm->UpdateAllRegistrations();
    }

    
    nsTArray<ContentParent*> children;
    ContentParent::GetAll(children);
    for (uint32_t i = 0; i < children.Length(); i++) {
      unused << children[i]->SendUpdateServiceWorkerRegistrations();
    }
  }

  return NS_OK;
}

} 
} 
} 
