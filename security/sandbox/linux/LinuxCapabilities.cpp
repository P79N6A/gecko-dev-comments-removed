





#include "LinuxCapabilities.h"

#include <unistd.h>
#include <sys/syscall.h>

namespace mozilla {

bool
LinuxCapabilities::GetCurrent() {
  __user_cap_header_struct header = { _LINUX_CAPABILITY_VERSION_3, 0 };
  return syscall(__NR_capget, &header, &mBits) == 0
    && header.version == _LINUX_CAPABILITY_VERSION_3;
}

bool
LinuxCapabilities::SetCurrentRaw() const {
  __user_cap_header_struct header = { _LINUX_CAPABILITY_VERSION_3, 0 };
  return syscall(__NR_capset, &header, &mBits) == 0
    && header.version == _LINUX_CAPABILITY_VERSION_3;
}

} 
