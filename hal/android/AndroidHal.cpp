




































#include "Hal.h"
#include "AndroidBridge.h"

using mozilla::hal::WindowIdentifier;

namespace mozilla {
namespace hal_impl {

void
Vibrate(const nsTArray<uint32> &pattern, const WindowIdentifier &)
{
  
  
  

  AndroidBridge* b = AndroidBridge::Bridge();
  if (!b) {
    return;
  }

  if (pattern.Length() == 0) {
    b->CancelVibrate();
  } else {
    b->Vibrate(pattern);
  }
}

void
CancelVibrate(const WindowIdentifier &)
{
  

  AndroidBridge* b = AndroidBridge::Bridge();
  if (b)
    b->CancelVibrate();
}

void
EnableBatteryNotifications()
{
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  bridge->EnableBatteryNotifications();
}

void
DisableBatteryNotifications()
{
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  bridge->DisableBatteryNotifications();
}

void
GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo)
{
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  bridge->GetCurrentBatteryInformation(aBatteryInfo);
}

bool
GetScreenEnabled()
{
  return true;
}

void
SetScreenEnabled(bool enabled)
{}

double
GetScreenBrightness()
{
  return 1;
}

void
SetScreenBrightness(double brightness)
{}

} 
} 

