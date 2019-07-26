




#include "Hal.h"
#include "AndroidBridge.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
EnableSensorNotifications(SensorType aSensor) {
  AndroidBridge::Bridge()->EnableSensor(aSensor);
}

void
DisableSensorNotifications(SensorType aSensor) {
  AndroidBridge::Bridge()->DisableSensor(aSensor);
}

} 
} 
