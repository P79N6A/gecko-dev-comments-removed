






































#include "Hal.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/hal_sandbox/PHalChild.h"
#include "mozilla/hal_sandbox/PHalParent.h"
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
Vibrate(const nsTArray<uint32>& pattern)
{
  AutoInfallibleTArray<uint32, 8> p(pattern);
  Hal()->SendVibrate(p);
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

class HalParent : public PHalParent
                , public BatteryObserver {
public:
  NS_OVERRIDE virtual bool
  RecvVibrate(const InfallibleTArray<unsigned int>& pattern) {
    
    
    
    hal::Vibrate(pattern);
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
