




#include "Hal.h"
#include "AndroidBridge.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
EnableSensorNotifications(SensorType aSensor) {
  GeckoAppShell::EnableSensor(aSensor);
}

void
DisableSensorNotifications(SensorType aSensor) {
  GeckoAppShell::DisableSensor(aSensor);
}

} 
} 
