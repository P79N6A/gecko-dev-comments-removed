




#include "mozilla/Hal.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
EnableSwitchNotifications(SwitchDevice aDevice)
{
}

void
DisableSwitchNotifications(SwitchDevice aDevice)
{
}

SwitchState
GetCurrentSwitchState(SwitchDevice aDevice) {
  return SWITCH_STATE_UNKNOWN;
}

void
NotifySwitchStateFromInputDevice(SwitchDevice aDevice, SwitchState aState)
{
}

bool IsHeadphoneEventFromInputDev()
{
  return false;
}

} 
} 
