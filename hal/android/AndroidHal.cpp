




#include "Hal.h"
#include "HalImpl.h"
#include "WindowIdentifier.h"
#include "AndroidBridge.h"
#include "mozilla/dom/network/Constants.h"
#include "mozilla/dom/ScreenOrientation.h"
#include "nsIScreenManager.h"

using namespace mozilla::dom;
using namespace mozilla::hal;
using namespace mozilla::widget::android;

namespace mozilla {
namespace hal_impl {

void
Vibrate(const nsTArray<uint32_t> &pattern, const WindowIdentifier &)
{
  
  
  

  
  
  
  bool allZero = true;
  for (uint32_t i = 0; i < pattern.Length(); i++) {
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
  

  GeckoAppShell::CancelVibrate();
}

void
EnableBatteryNotifications()
{
  GeckoAppShell::EnableBatteryNotifications();
}

void
DisableBatteryNotifications()
{
  GeckoAppShell::DisableBatteryNotifications();
}

void
GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo)
{
  AndroidBridge::Bridge()->GetCurrentBatteryInformation(aBatteryInfo);
}

void
EnableNetworkNotifications()
{
  GeckoAppShell::EnableNetworkNotifications();
}

void
DisableNetworkNotifications()
{
  GeckoAppShell::DisableNetworkNotifications();
}

void
GetCurrentNetworkInformation(hal::NetworkInformation* aNetworkInfo)
{
  AndroidBridge::Bridge()->GetCurrentNetworkInformation(aNetworkInfo);
}

void
EnableScreenConfigurationNotifications()
{
  GeckoAppShell::EnableScreenOrientationNotifications();
}

void
DisableScreenConfigurationNotifications()
{
  GeckoAppShell::DisableScreenOrientationNotifications();
}

void
GetCurrentScreenConfiguration(ScreenConfiguration* aScreenConfiguration)
{
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  nsresult rv;
  nsCOMPtr<nsIScreenManager> screenMgr =
    do_GetService("@mozilla.org/gfx/screenmanager;1", &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("Can't find nsIScreenManager!");
    return;
  }

  nsIntRect rect;
  int32_t colorDepth, pixelDepth;
  ScreenOrientation orientation;
  nsCOMPtr<nsIScreen> screen;

  screenMgr->GetPrimaryScreen(getter_AddRefs(screen));
  screen->GetRect(&rect.x, &rect.y, &rect.width, &rect.height);
  screen->GetColorDepth(&colorDepth);
  screen->GetPixelDepth(&pixelDepth);
  orientation = static_cast<ScreenOrientation>(bridge->GetScreenOrientation());

  *aScreenConfiguration =
    hal::ScreenConfiguration(rect, orientation, colorDepth, pixelDepth);
}

bool
LockScreenOrientation(const ScreenOrientation& aOrientation)
{
  switch (aOrientation) {
    
    case eScreenOrientation_PortraitPrimary:
    case eScreenOrientation_PortraitSecondary:
    case eScreenOrientation_PortraitPrimary | eScreenOrientation_PortraitSecondary:
    case eScreenOrientation_LandscapePrimary:
    case eScreenOrientation_LandscapeSecondary:
    case eScreenOrientation_LandscapePrimary | eScreenOrientation_LandscapeSecondary:
      GeckoAppShell::LockScreenOrientation(aOrientation);
      return true;
    default:
      return false;
  }
}

void
UnlockScreenOrientation()
{
  GeckoAppShell::UnlockScreenOrientation();
}

} 
} 

