




































#include "Hal.h"
#include "AndroidBridge.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {







static int
MapSensorType(SensorType aSensorType) {
  return (SENSOR_UNKNOWN <= aSensorType && aSensorType < NUM_SENSOR_TYPE) ?
    aSensorType + 1 : -1;
}

void
EnableSensorNotifications(SensorType aSensor) {
  int androidSensor = MapSensorType(aSensor);
  
  MOZ_ASSERT(androidSensor != -1);
  AndroidBridge::Bridge()->EnableSensor(androidSensor);
}

void
DisableSensorNotifications(SensorType aSensor) {
  int androidSensor = MapSensorType(aSensor);
  
  MOZ_ASSERT(androidSensor != -1);
  AndroidBridge::Bridge()->DisableSensor(androidSensor);
}

} 
} 
