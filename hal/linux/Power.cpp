




































#include "Hal.h"

#include <unistd.h>
#include <sys/reboot.h>

namespace mozilla {
namespace hal_impl {

void
Reboot()
{
  reboot(RB_AUTOBOOT);
}

void
PowerOff()
{
  reboot(RB_POWER_OFF);
}

} 
} 
