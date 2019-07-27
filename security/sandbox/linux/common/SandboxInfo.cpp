





#include "SandboxInfo.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/prctl.h>

#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"
#include "mozilla/Assertions.h"
#include "mozilla/NullPtr.h"

namespace mozilla {


const SandboxInfo SandboxInfo::sSingleton = SandboxInfo();

SandboxInfo::SandboxInfo() {
  int flags = 0;
  static_assert(sizeof(flags) >= sizeof(Flags), "enum Flags fits in an int");

  
  if (!getenv("MOZ_FAKE_NO_SANDBOX")) {
    
    
    
    
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, nullptr) != -1) {
      MOZ_CRASH("prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, nullptr)"
                " didn't fail");
    }
    if (errno == EFAULT) {
      flags |= kHasSeccompBPF;
    }
  }

#ifdef MOZ_CONTENT_SANDBOX
  if (!getenv("MOZ_DISABLE_CONTENT_SANDBOX")) {
    flags |= kEnabledForContent;
  }
#endif
#ifdef MOZ_GMP_SANDBOX
  if (!getenv("MOZ_DISABLE_GMP_SANDBOX")) {
    flags |= kEnabledForMedia;
  }
#endif
  if (getenv("MOZ_SANDBOX_VERBOSE")) {
    flags |= kVerbose;
  }

  mFlags = static_cast<Flags>(flags);
}

} 
