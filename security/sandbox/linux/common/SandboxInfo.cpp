





#include "SandboxInfo.h"
#include "LinuxSched.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include "base/posix/eintr_wrapper.h"
#include "mozilla/Assertions.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"
#include "sandbox/linux/services/linux_syscalls.h"

namespace mozilla {

static bool
HasSeccompBPF()
{
  
  if (getenv("MOZ_FAKE_NO_SANDBOX")) {
    return false;
  }
  
  
  
  
  if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, nullptr) != -1) {
    MOZ_CRASH("prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, nullptr)"
              " didn't fail");
  }
  MOZ_ASSERT(errno == EFAULT || errno == EINVAL);
  return errno == EFAULT;
}

static bool
HasSeccompTSync()
{
  
  
  if (getenv("MOZ_FAKE_NO_SECCOMP_TSYNC")) {
    return false;
  }
  if (syscall(__NR_seccomp, SECCOMP_SET_MODE_FILTER,
              SECCOMP_FILTER_FLAG_TSYNC, nullptr) != -1) {
    MOZ_CRASH("seccomp(..., SECCOMP_FILTER_FLAG_TSYNC, nullptr)"
              " didn't fail");
  }
  MOZ_ASSERT(errno == EFAULT || errno == EINVAL || errno == ENOSYS);
  return errno == EFAULT;
}

static bool
HasUserNamespaceSupport()
{
  
  
  
  
  
  
  if (access("/proc/self/ns/user", F_OK) == -1) {
    MOZ_ASSERT(errno == ENOENT);
    return false;
  }
  return true;
}

static bool
CanCreateUserNamespace()
{
  
  
  
  
  
  
  
  
  
  
  
  
  static const char kCacheEnvName[] = "MOZ_ASSUME_USER_NS";
  const char* cached = getenv(kCacheEnvName);
  if (cached) {
    return cached[0] > '0';
  }

  pid_t pid = syscall(__NR_clone, SIGCHLD | CLONE_NEWUSER);
  if (pid == 0) {
    
    _exit(0);
  }
  if (pid == -1) {
    
    MOZ_ASSERT(errno == EINVAL || 
               errno == EPERM  || 
               errno == EUSERS);  
    setenv(kCacheEnvName, "0", 1);
    return false;
  }
  
  DebugOnly<bool> ok = HANDLE_EINTR(waitpid(pid, nullptr, 0)) == pid;
  MOZ_ASSERT(ok);
  setenv(kCacheEnvName, "1", 1);
  return true;
}


const SandboxInfo SandboxInfo::sSingleton = SandboxInfo();

SandboxInfo::SandboxInfo() {
  int flags = 0;
  static_assert(sizeof(flags) >= sizeof(Flags), "enum Flags fits in an int");

  if (HasSeccompBPF()) {
    flags |= kHasSeccompBPF;
    if (HasSeccompTSync()) {
      flags |= kHasSeccompTSync;
    }
  }

  if (HasUserNamespaceSupport()) {
    flags |= kHasPrivilegedUserNamespaces;
    if (CanCreateUserNamespace()) {
      flags |= kHasUserNamespaces;
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
