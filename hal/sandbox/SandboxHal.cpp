






































#include "Hal.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/hal_sandbox/PHalChild.h"
#include "mozilla/hal_sandbox/PHalParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/dom/battery/Types.h"
#include "mozilla/Observer.h"
#include "mozilla/unused.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::hal;

namespace mozilla {
namespace hal_sandbox {

static PHalChild* sHal;
static PHalChild*
Hal()
{
  if (!sHal) {
    sHal = ContentChild::GetSingleton()->SendPHalConstructor();
  }
  return sHal;
}

void
Vibrate(const nsTArray<uint32>& pattern, const WindowIdentifier &id)
{
  HAL_LOG(("Vibrate: Sending to parent process."));

  AutoInfallibleTArray<uint32, 8> p(pattern);

  WindowIdentifier newID(id);
  newID.AppendProcessID();
  Hal()->SendVibrate(p, newID.AsArray(), GetTabChildFrom(newID.GetWindow()));
}

void
CancelVibrate(const WindowIdentifier &id)
{
  HAL_LOG(("CancelVibrate: Sending to parent process."));

  WindowIdentifier newID(id);
  newID.AppendProcessID();
  Hal()->SendCancelVibrate(newID.AsArray(), GetTabChildFrom(newID.GetWindow()));
}

void
EnableBatteryNotifications()
{
  Hal()->SendEnableBatteryNotifications();
}

void
DisableBatteryNotifications()
{
  Hal()->SendDisableBatteryNotifications();
}

void
GetCurrentBatteryInformation(BatteryInformation* aBatteryInfo)
{
  Hal()->SendGetCurrentBatteryInformation(aBatteryInfo);
}

bool
GetScreenEnabled()
{
  bool enabled = false;
  Hal()->SendGetScreenEnabled(&enabled);
  return enabled;
}

void
SetScreenEnabled(bool enabled)
{
  Hal()->SendSetScreenEnabled(enabled);
}

double
GetScreenBrightness()
{
  double brightness = 0;
  Hal()->SendGetScreenBrightness(&brightness);
  return brightness;
}

void
SetScreenBrightness(double brightness)
{
  Hal()->SendSetScreenBrightness(brightness);
}

class HalParent : public PHalParent
                , public BatteryObserver {
public:
  NS_OVERRIDE virtual bool
  RecvVibrate(const InfallibleTArray<unsigned int>& pattern,
              const InfallibleTArray<uint64> &id,
              PBrowserParent *browserParent)
  {
    
    
    
    
    
    
    TabParent *tabParent = static_cast<TabParent*>(browserParent);
    if (!tabParent->Active()) {
      HAL_LOG(("RecvVibrate: Tab is not active. Cancelling."));
      return true;
    }

    
    
    
    nsCOMPtr<nsIDOMWindow> window =
      do_QueryInterface(tabParent->GetBrowserDOMWindow());
    WindowIdentifier newID(id, window);
    hal::Vibrate(pattern, newID);
    return true;
  }

  NS_OVERRIDE virtual bool
  RecvCancelVibrate(const InfallibleTArray<uint64> &id,
                    PBrowserParent *browserParent)
  {
    TabParent *tabParent = static_cast<TabParent*>(browserParent);
    nsCOMPtr<nsIDOMWindow> window =
      do_QueryInterface(tabParent->GetBrowserDOMWindow());
    WindowIdentifier newID(id, window);
    hal::CancelVibrate(newID);
    return true;
  }

  NS_OVERRIDE virtual bool
  RecvEnableBatteryNotifications() {
    hal::RegisterBatteryObserver(this);
    return true;
  }

  NS_OVERRIDE virtual bool
  RecvDisableBatteryNotifications() {
    hal::UnregisterBatteryObserver(this);
    return true;
  }

  NS_OVERRIDE virtual bool
  RecvGetCurrentBatteryInformation(BatteryInformation* aBatteryInfo) {
    hal::GetCurrentBatteryInformation(aBatteryInfo);
    return true;
  }

  void Notify(const BatteryInformation& aBatteryInfo) {
    unused << SendNotifyBatteryChange(aBatteryInfo);
  }

  NS_OVERRIDE virtual bool
  RecvGetScreenEnabled(bool *enabled)
  {
    *enabled = hal::GetScreenEnabled();
    return true;
  }

  NS_OVERRIDE virtual bool
  RecvSetScreenEnabled(const bool &enabled)
  {
    hal::SetScreenEnabled(enabled);
    return true;
  }

  NS_OVERRIDE virtual bool
  RecvGetScreenBrightness(double *brightness)
  {
    *brightness = hal::GetScreenBrightness();
    return true;
  }

  NS_OVERRIDE virtual bool
  RecvSetScreenBrightness(const double &brightness)
  {
    hal::SetScreenBrightness(brightness);
    return true;
  }
};

class HalChild : public PHalChild {
public:
  NS_OVERRIDE virtual bool
  RecvNotifyBatteryChange(const BatteryInformation& aBatteryInfo) {
    hal::NotifyBatteryChange(aBatteryInfo);
    return true;
  }
};

PHalChild* CreateHalChild() {
  return new HalChild();
}

PHalParent* CreateHalParent() {
  return new HalParent();
}

} 
} 
