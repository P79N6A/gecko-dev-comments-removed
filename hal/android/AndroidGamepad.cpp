




#include "Hal.h"
#include "AndroidBridge.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
StartMonitoringGamepadStatus()
{
  widget::GeckoAppShell::StartMonitoringGamepad();
}

void
StopMonitoringGamepadStatus()
{
  widget::GeckoAppShell::StopMonitoringGamepad();
}

} 
} 
