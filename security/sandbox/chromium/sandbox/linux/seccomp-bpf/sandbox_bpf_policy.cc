



#include "sandbox/linux/seccomp-bpf/sandbox_bpf_policy.h"

#include <errno.h>

#include "sandbox/linux/seccomp-bpf/errorcode.h"

namespace sandbox {

ErrorCode SandboxBPFPolicy::InvalidSyscall(SandboxBPF* sandbox_compiler) const {
  return ErrorCode(ENOSYS);
}

}  
