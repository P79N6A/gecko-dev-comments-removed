




#ifndef mozilla_hal_Types_h
#define mozilla_hal_Types_h

#include "IPCMessageUtils.h"

namespace mozilla {
namespace hal {






enum LightType {
    eHalLightID_Backlight = 0,
    eHalLightID_Keyboard = 1,
    eHalLightID_Buttons = 2,
    eHalLightID_Battery = 3,
    eHalLightID_Notifications = 4,
    eHalLightID_Attention = 5,
    eHalLightID_Bluetooth = 6,
    eHalLightID_Wifi = 7,
    eHalLightID_Count = 8         
};
enum LightMode {
    eHalLightMode_User = 0,       
    eHalLightMode_Sensor = 1      
};
enum FlashMode {
    eHalLightFlash_None = 0,
    eHalLightFlash_Timed = 1,     
    eHalLightFlash_Hardware = 2   
};
} 
} 

namespace IPC {




template <>
struct ParamTraits<mozilla::hal::LightType>
  : public EnumSerializer<mozilla::hal::LightType,
                          mozilla::hal::eHalLightID_Backlight,
                          mozilla::hal::eHalLightID_Count>
{};




template <>
struct ParamTraits<mozilla::hal::LightMode>
  : public EnumSerializer<mozilla::hal::LightMode,
                          mozilla::hal::eHalLightMode_User,
                          mozilla::hal::eHalLightMode_Sensor>
{};




template <>
struct ParamTraits<mozilla::hal::FlashMode>
  : public EnumSerializer<mozilla::hal::FlashMode,
                          mozilla::hal::eHalLightFlash_None,
                          mozilla::hal::eHalLightFlash_Hardware>
{};

} 

#endif 
