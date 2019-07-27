





#ifndef __HAL_SENSOR_H_
#define __HAL_SENSOR_H_

#include "mozilla/Observer.h"

namespace mozilla {
namespace hal {






enum SensorType {
  SENSOR_UNKNOWN = -1,
  SENSOR_ORIENTATION = 0,
  SENSOR_ACCELERATION = 1,
  SENSOR_PROXIMITY = 2,
  SENSOR_LINEAR_ACCELERATION = 3,
  SENSOR_GYROSCOPE = 4,
  SENSOR_LIGHT = 5,
  SENSOR_ROTATION_VECTOR = 6,
  SENSOR_GAME_ROTATION_VECTOR = 7,
  NUM_SENSOR_TYPE
};

class SensorData;

typedef Observer<SensorData> ISensorObserver;




enum SensorAccuracyType {
  SENSOR_ACCURACY_UNKNOWN = -1,
  SENSOR_ACCURACY_UNRELIABLE,
  SENSOR_ACCURACY_LOW,
  SENSOR_ACCURACY_MED,
  SENSOR_ACCURACY_HIGH,
  NUM_SENSOR_ACCURACY_TYPE
};

class SensorAccuracy;

typedef Observer<SensorAccuracy> ISensorAccuracyObserver;

}
}

#include "ipc/IPCMessageUtils.h"

namespace IPC {
  


  template <>
  struct ParamTraits<mozilla::hal::SensorType>:
    public ContiguousEnumSerializer<
             mozilla::hal::SensorType,
             mozilla::hal::SENSOR_UNKNOWN,
             mozilla::hal::NUM_SENSOR_TYPE> {
  };

  template <>
  struct ParamTraits<mozilla::hal::SensorAccuracyType>:
    public ContiguousEnumSerializer<
             mozilla::hal::SensorAccuracyType,
             mozilla::hal::SENSOR_ACCURACY_UNKNOWN,
             mozilla::hal::NUM_SENSOR_ACCURACY_TYPE> {

  };
} 

#endif 
