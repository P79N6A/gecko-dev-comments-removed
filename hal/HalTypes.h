




#ifndef mozilla_hal_Types_h
#define mozilla_hal_Types_h

#include "IPCMessageUtils.h"

namespace mozilla {
namespace hal {






enum LightType {
  eHalLightID_Backlight     = 0,
  eHalLightID_Keyboard      = 1,
  eHalLightID_Buttons       = 2,
  eHalLightID_Battery       = 3,
  eHalLightID_Notifications = 4,
  eHalLightID_Attention     = 5,
  eHalLightID_Bluetooth     = 6,
  eHalLightID_Wifi          = 7,
  eHalLightID_Count         = 8  
};
enum LightMode {
  eHalLightMode_User   = 0,  
  eHalLightMode_Sensor = 1,  
  eHalLightMode_Count
};
enum FlashMode {
  eHalLightFlash_None     = 0,
  eHalLightFlash_Timed    = 1,  
  eHalLightFlash_Hardware = 2,  
  eHalLightFlash_Count
};

enum ShutdownMode {
  eHalShutdownMode_Unknown  = -1,
  eHalShutdownMode_PowerOff = 0,
  eHalShutdownMode_Reboot   = 1,
  eHalShutdownMode_Restart  = 2,
  eHalShutdownMode_Count    = 3
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
  SWITCH_STATE_HEADSET,          
  SWITCH_STATE_HEADPHONE,        
  NUM_SWITCH_STATE
};

typedef Observer<SwitchEvent> SwitchObserver;

enum ProcessPriority {
  PROCESS_PRIORITY_BACKGROUND,
  PROCESS_PRIORITY_BACKGROUND_HOMESCREEN,
  PROCESS_PRIORITY_BACKGROUND_PERCEIVABLE,
  
  
  
  PROCESS_PRIORITY_FOREGROUND,
  PROCESS_PRIORITY_MASTER,
  NUM_PROCESS_PRIORITY
};






const char*
ProcessPriorityToString(ProcessPriority aPriority);




enum WakeLockControl {
  WAKE_LOCK_REMOVE_ONE = -1,
  WAKE_LOCK_NO_CHANGE  = 0,
  WAKE_LOCK_ADD_ONE    = 1,
  NUM_WAKE_LOCK
};

class FMRadioOperationInformation;

enum FMRadioOperation {
  FM_RADIO_OPERATION_UNKNOWN = -1,
  FM_RADIO_OPERATION_ENABLE,
  FM_RADIO_OPERATION_DISABLE,
  FM_RADIO_OPERATION_SEEK,
  NUM_FM_RADIO_OPERATION
};

enum FMRadioOperationStatus {
  FM_RADIO_OPERATION_STATUS_UNKNOWN = -1,
  FM_RADIO_OPERATION_STATUS_SUCCESS,
  FM_RADIO_OPERATION_STATUS_FAIL,
  NUM_FM_RADIO_OPERATION_STATUS
};

enum FMRadioSeekDirection {
  FM_RADIO_SEEK_DIRECTION_UNKNOWN = -1,
  FM_RADIO_SEEK_DIRECTION_UP,
  FM_RADIO_SEEK_DIRECTION_DOWN,
  NUM_FM_RADIO_SEEK_DIRECTION
};

enum FMRadioCountry {
  FM_RADIO_COUNTRY_UNKNOWN = -1,
  FM_RADIO_COUNTRY_US,  
  FM_RADIO_COUNTRY_EU,
  FM_RADIO_COUNTRY_JP_STANDARD,
  FM_RADIO_COUNTRY_JP_WIDE,
  FM_RADIO_COUNTRY_DE,  
  FM_RADIO_COUNTRY_AW,  
  FM_RADIO_COUNTRY_AU,  
  FM_RADIO_COUNTRY_BS,  
  FM_RADIO_COUNTRY_BD,  
  FM_RADIO_COUNTRY_CY,  
  FM_RADIO_COUNTRY_VA,  
  FM_RADIO_COUNTRY_CO,  
  FM_RADIO_COUNTRY_KR,  
  FM_RADIO_COUNTRY_DK,  
  FM_RADIO_COUNTRY_EC,  
  FM_RADIO_COUNTRY_ES,  
  FM_RADIO_COUNTRY_FI,  
  FM_RADIO_COUNTRY_FR,  
  FM_RADIO_COUNTRY_GM,  
  FM_RADIO_COUNTRY_HU,  
  FM_RADIO_COUNTRY_IN,  
  FM_RADIO_COUNTRY_IR,  
  FM_RADIO_COUNTRY_IT,  
  FM_RADIO_COUNTRY_KW,  
  FM_RADIO_COUNTRY_LT,  
  FM_RADIO_COUNTRY_ML,  
  FM_RADIO_COUNTRY_MA,  
  FM_RADIO_COUNTRY_NO,  
  FM_RADIO_COUNTRY_NZ,  
  FM_RADIO_COUNTRY_OM,  
  FM_RADIO_COUNTRY_PG,  
  FM_RADIO_COUNTRY_NL,  
  FM_RADIO_COUNTRY_QA,  
  FM_RADIO_COUNTRY_CZ,  
  FM_RADIO_COUNTRY_UK,  
  FM_RADIO_COUNTRY_RW,  
  FM_RADIO_COUNTRY_SN,  
  FM_RADIO_COUNTRY_SG,  
  FM_RADIO_COUNTRY_SI,  
  FM_RADIO_COUNTRY_ZA,  
  FM_RADIO_COUNTRY_SE,  
  FM_RADIO_COUNTRY_CH,  
  FM_RADIO_COUNTRY_TW,  
  FM_RADIO_COUNTRY_TR,  
  FM_RADIO_COUNTRY_UA,  
  FM_RADIO_COUNTRY_USER_DEFINED,
  NUM_FM_RADIO_COUNTRY
};

typedef Observer<FMRadioOperationInformation> FMRadioObserver;
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
struct ParamTraits<mozilla::hal::ShutdownMode>
  : public EnumSerializer<mozilla::hal::ShutdownMode,
                          mozilla::hal::eHalShutdownMode_Unknown,
                          mozilla::hal::eHalShutdownMode_Count>
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
struct ParamTraits<mozilla::hal::FMRadioOperation>:
  public EnumSerializer<mozilla::hal::FMRadioOperation,
                        mozilla::hal::FM_RADIO_OPERATION_UNKNOWN,
                        mozilla::hal::NUM_FM_RADIO_OPERATION>
{};




template <>
struct ParamTraits<mozilla::hal::FMRadioOperationStatus>:
  public EnumSerializer<mozilla::hal::FMRadioOperationStatus,
                        mozilla::hal::FM_RADIO_OPERATION_STATUS_UNKNOWN,
                        mozilla::hal::NUM_FM_RADIO_OPERATION_STATUS>
{};




template <>
struct ParamTraits<mozilla::hal::FMRadioSeekDirection>:
  public EnumSerializer<mozilla::hal::FMRadioSeekDirection,
                        mozilla::hal::FM_RADIO_SEEK_DIRECTION_UNKNOWN,
                        mozilla::hal::NUM_FM_RADIO_SEEK_DIRECTION>
{};




template <>
struct ParamTraits<mozilla::hal::FMRadioCountry>:
  public EnumSerializer<mozilla::hal::FMRadioCountry,
                        mozilla::hal::FM_RADIO_COUNTRY_UNKNOWN,
                        mozilla::hal::NUM_FM_RADIO_COUNTRY>
{};
} 

#endif 
