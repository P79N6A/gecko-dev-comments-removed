




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
    eHalLightMode_Sensor = 1,     
    eHalLightMode_Count
};
enum FlashMode {
    eHalLightFlash_None = 0,
    eHalLightFlash_Timed = 1,     
    eHalLightFlash_Hardware = 2,  
    eHalLightFlash_Count
};

class SwitchEvent;

enum SwitchDevice {
  SWITCH_DEVICE_UNKNOWN = -1,
  SWITCH_HEADPHONES,
  SWITCH_USB,
  NUM_SWITCH_DEVICE
};

enum SwitchState {
  SWITCH_STATE_UNKNOWN = -1,
  SWITCH_STATE_ON,
  SWITCH_STATE_OFF,
  NUM_SWITCH_STATE
};

typedef Observer<SwitchEvent> SwitchObserver;

enum ProcessPriority {
  PROCESS_PRIORITY_BACKGROUND,
  PROCESS_PRIORITY_FOREGROUND,
  PROCESS_PRIORITY_MASTER,
  NUM_PROCESS_PRIORITY
};




enum WakeLockControl {
  WAKE_LOCK_REMOVE_ONE = -1,
  WAKE_LOCK_NO_CHANGE  = 0,
  WAKE_LOCK_ADD_ONE    = 1,
  NUM_WAKE_LOCK
};

enum SystemTimeChange {
  SYS_TIME_CHANGE_UNKNOWN = -1,
  SYS_TIME_CHANGE_CLOCK,
  SYS_TIME_CHANGE_TZ,
  SYS_TIME_CHANGE_GUARD
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
                          mozilla::hal::eHalLightMode_Count>
{};




template <>
struct ParamTraits<mozilla::hal::FlashMode>
  : public EnumSerializer<mozilla::hal::FlashMode,
                          mozilla::hal::eHalLightFlash_None,
                          mozilla::hal::eHalLightFlash_Count>
{};




template <>
struct ParamTraits<mozilla::hal::WakeLockControl>
  : public EnumSerializer<mozilla::hal::WakeLockControl,
                          mozilla::hal::WAKE_LOCK_REMOVE_ONE,
                          mozilla::hal::NUM_WAKE_LOCK>
{};




template <>
struct ParamTraits<mozilla::hal::SwitchState>:
  public EnumSerializer<mozilla::hal::SwitchState,
                        mozilla::hal::SWITCH_STATE_UNKNOWN,
                        mozilla::hal::NUM_SWITCH_STATE> {
};




template <>
struct ParamTraits<mozilla::hal::SwitchDevice>:
  public EnumSerializer<mozilla::hal::SwitchDevice,
                        mozilla::hal::SWITCH_DEVICE_UNKNOWN,
                        mozilla::hal::NUM_SWITCH_DEVICE> {
};

template <>
struct ParamTraits<mozilla::hal::ProcessPriority>:
  public EnumSerializer<mozilla::hal::ProcessPriority,
                        mozilla::hal::PROCESS_PRIORITY_BACKGROUND,
                        mozilla::hal::NUM_PROCESS_PRIORITY> {
};




template <>
struct ParamTraits<mozilla::hal::SystemTimeChange>
  : public EnumSerializer<mozilla::hal::SystemTimeChange,
                          mozilla::hal::SYS_TIME_CHANGE_UNKNOWN,
                          mozilla::hal::SYS_TIME_CHANGE_GUARD>
{};
 
} 

#endif 
