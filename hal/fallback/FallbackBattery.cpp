





#include "Hal.h"
#include "mozilla/dom/battery/Constants.h"

namespace mozilla {
namespace hal_impl {

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
  aBatteryInfo->remainingTime() = dom::battery::kDefaultRemainingTime;
}

} 
} 
