




#include "AndroidBridge.h"

namespace mozilla {
namespace dom {

void StartGamepadMonitoring()
{
  widget::GeckoAppShell::StartMonitoringGamepad();
}

void StopGamepadMonitoring()
{
  widget::GeckoAppShell::StopMonitoringGamepad();
}

} 
} 
