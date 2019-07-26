




#include "Hal.h"
#include "AndroidBridge.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
StartMonitoringGamepadStatus()
{
  mozilla::widget::android::GeckoAppShell::StartMonitoringGamepad();
}

void
StopMonitoringGamepadStatus()
{
  mozilla::widget::android::GeckoAppShell::StopMonitoringGamepad();
}

} 
} 
