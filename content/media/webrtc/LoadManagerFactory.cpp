




#include "LoadManager.h"
#include "LoadManagerFactory.h"
#include "MainThreadUtils.h"

#include "mozilla/Preferences.h"

namespace mozilla {

LoadManager* LoadManagerBuild(void)
{
  MOZ_ASSERT(NS_IsMainThread());

#if defined(ANDROID) || defined(LINUX) || defined(XP_MACOSX)
  int loadMeasurementInterval =
    mozilla::Preferences::GetInt("media.navigator.load_adapt.measure_interval", 1000);
  int averagingSeconds =
    mozilla::Preferences::GetInt("media.navigator.load_adapt.avg_seconds", 3);
  float highLoadThreshold =
    mozilla::Preferences::GetFloat("media.navigator.load_adapt.high_load", 0.90);
  float lowLoadThreshold =
    mozilla::Preferences::GetFloat("media.navigator.load_adapt.low_load", 0.40);

  return new LoadManager(loadMeasurementInterval,
                         averagingSeconds,
                         highLoadThreshold,
                         lowLoadThreshold);
#else
  
  return nullptr;
#endif
}

void LoadManagerDestroy(mozilla::LoadManager* aLoadManager)
{
  delete aLoadManager;
}

}; 
