




#include "PowerWakeLock.h"
#include <hardware_legacy/power.h>

const char* kPower_WAKE_LOCK_ID = "PowerKeyEvent";

namespace mozilla {
namespace hal_impl {
StaticRefPtr <PowerWakelock> gPowerWakelock;
PowerWakelock::PowerWakelock()
{
  acquire_wake_lock(PARTIAL_WAKE_LOCK, kPower_WAKE_LOCK_ID);
}

PowerWakelock::~PowerWakelock()
{
  release_wake_lock(kPower_WAKE_LOCK_ID);
}
} 
} 
