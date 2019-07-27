




#include "Hal.h"
#include "AndroidBridge.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
EnableSensorNotifications(SensorType aSensor) {
  widget::GeckoAppShell::EnableSensor(aSensor);
}

void
DisableSensorNotifications(SensorType aSensor) {
  widget::GeckoAppShell::DisableSensor(aSensor);
}

} 
} 
