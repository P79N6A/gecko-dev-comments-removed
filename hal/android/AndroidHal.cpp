




































#include "Hal.h"
#include "mozilla/dom/battery/Constants.h"

namespace mozilla {
namespace hal_impl {

void
Vibrate(const nsTArray<uint32>& pattern)
{}

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

} 
} 

