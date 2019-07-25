




































#include "Hal.h"
#include "HalImpl.h"
#include "WindowIdentifier.h"
#include "AndroidBridge.h"
#include "mozilla/dom/network/Constants.h"
#include "mozilla/dom/ScreenOrientation.h"

using mozilla::hal::WindowIdentifier;

namespace mozilla {
namespace hal_impl {

void
Vibrate(const nsTArray<uint32> &pattern, const WindowIdentifier &)
{
  
  
  

  
  
  
  bool allZero = true;
  for (uint32 i = 0; i < pattern.Length(); i++) {
    if (pattern[i] != 0) {
      allZero = false;
      break;
    }
  }

  if (allZero) {
    hal_impl::CancelVibrate(WindowIdentifier());
    return;
  }

  AndroidBridge* b = AndroidBridge::Bridge();
  if (!b) {
    return;
  }

  b->Vibrate(pattern);
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

void
EnableNetworkNotifications()
{
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  bridge->EnableNetworkNotifications();
}

void
DisableNetworkNotifications()
{
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  bridge->DisableNetworkNotifications();
}

void
GetCurrentNetworkInformation(hal::NetworkInformation* aNetworkInfo)
{
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  bridge->GetCurrentNetworkInformation(aNetworkInfo);
}

void
Reboot()
{}

void
PowerOff()
{}

void
EnableScreenOrientationNotifications()
{
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  bridge->EnableScreenOrientationNotifications();
}

void
DisableScreenOrientationNotifications()
{
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  bridge->DisableScreenOrientationNotifications();
}

void
GetCurrentScreenOrientation(dom::ScreenOrientation* aScreenOrientation)
{
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  dom::ScreenOrientationWrapper orientationWrapper;
  bridge->GetScreenOrientation(orientationWrapper);
  *aScreenOrientation = orientationWrapper.orientation;
}

} 
} 

