




#include "Hal.h"
#include "AndroidBridge.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
EnableSensorNotifications(SensorType aSensor) {
  mozilla::widget::android::GeckoAppShell::EnableSensor(aSensor);
}

void
DisableSensorNotifications(SensorType aSensor) {
  mozilla::widget::android::GeckoAppShell::DisableSensor(aSensor);
}

} 
} 
