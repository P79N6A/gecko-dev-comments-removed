






































#include "Hal.h"
#include "mozilla/dom/battery/Constants.h"

using mozilla::hal::WindowIdentifier;

namespace mozilla {
namespace hal_impl {

void
Vibrate(const nsTArray<uint32>& pattern, const hal::WindowIdentifier &)
{}

void
CancelVibrate(const hal::WindowIdentifier &)
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
  aBatteryInfo->remainingTime() = dom::battery::kUnknownRemainingTime;
}

} 
} 
