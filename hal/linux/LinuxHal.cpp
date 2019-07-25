




































#include "Hal.h"

#ifndef MOZ_ENABLE_DBUS
#include <mozilla/dom/battery/Constants.h>
#endif 

namespace mozilla {
namespace hal_impl {

void
Vibrate(const nsTArray<uint32>& pattern)
{}

#ifndef MOZ_ENABLE_DBUS
void
EnableBatteryNotifications()
{}

void
DisableBatteryNotifications()
{}

void
GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo)
{
  aBatteryInfo->level() = dom::battery::kDefaultLevel;
  aBatteryInfo->charging() = dom::battery::kDefaultCharging;
}
#endif 

} 
} 

