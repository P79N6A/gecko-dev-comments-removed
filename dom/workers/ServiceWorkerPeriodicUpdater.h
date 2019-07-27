





#ifndef mozilla_ServiceWorkerPeriodicUpdater_h
#define mozilla_ServiceWorkerPeriodicUpdater_h

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "mozilla/StaticPtr.h"

namespace mozilla {
namespace dom {
namespace workers {









class ServiceWorkerPeriodicUpdater final : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static already_AddRefed<ServiceWorkerPeriodicUpdater> GetSingleton();

private:
  ServiceWorkerPeriodicUpdater();
  ~ServiceWorkerPeriodicUpdater();

  static StaticRefPtr<ServiceWorkerPeriodicUpdater> sInstance;
  static bool sPeriodicUpdatesEnabled;
};

} 
} 
} 

#endif
