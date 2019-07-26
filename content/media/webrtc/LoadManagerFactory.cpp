




#include "LoadManager.h"
#include "LoadManagerFactory.h"
#include "MainThreadUtils.h"
#include "nsIObserverService.h"

#include "mozilla/Preferences.h"

namespace mozilla {


LoadManager *
LoadManagerBuild(void)
{
  return new LoadManager(LoadManagerSingleton::Get());
}

  LoadManagerSingleton*
LoadManagerSingleton::Get() {
  if (!sSingleton) {
    MOZ_ASSERT(NS_IsMainThread());

    int loadMeasurementInterval =
      mozilla::Preferences::GetInt("media.navigator.load_adapt.measure_interval", 1000);
    int averagingSeconds =
      mozilla::Preferences::GetInt("media.navigator.load_adapt.avg_seconds", 3);
    float highLoadThreshold =
      mozilla::Preferences::GetFloat("media.navigator.load_adapt.high_load", 0.90f);
    float lowLoadThreshold =
      mozilla::Preferences::GetFloat("media.navigator.load_adapt.low_load", 0.40f);

    sSingleton = new LoadManagerSingleton(loadMeasurementInterval,
                                          averagingSeconds,
                                          highLoadThreshold,
                                          lowLoadThreshold);

    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    if (obs) {
      obs->AddObserver(sSingleton, "xpcom-shutdown", false);
    }
  }
  return sSingleton;
}

}; 
