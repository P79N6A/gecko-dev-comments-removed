






































#ifndef __HAL_SENSOR_H_
#define __HAL_SENSOR_H_

#include "mozilla/Observer.h"

namespace mozilla {
namespace hal {





enum SensorType {
  SENSOR_UNKNOWN = -1,
  SENSOR_ORIENTATION,
  SENSOR_ACCELERATION,
  SENSOR_PROXIMITY,
  NUM_SENSOR_TYPE
};

class SensorData;

typedef Observer<SensorData> ISensorObserver;

}
}


#include "IPC/IPCMessageUtils.h"

namespace IPC {
  


  template <>
  struct ParamTraits<mozilla::hal::SensorType>:
    public EnumSerializer<mozilla::hal::SensorType,
                          mozilla::hal::SENSOR_UNKNOWN,
                          mozilla::hal::NUM_SENSOR_TYPE> {
  };

} 

#endif 
