





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
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"
#include "sandbox/linux/services/linux_syscalls.h"















namespace mozilla {

static bool
HasSeccompBPF()
{
  
  if (getenv("MOZ_FAKE_NO_SANDBOX")) {
    return false;
  }
  
  
  
  

  int rv = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, nullptr);
  MOZ_DIAGNOSTIC_ASSERT(rv == -1, "prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER,"
                        " nullptr) didn't fail");
  MOZ_DIAGNOSTIC_ASSERT(errno == EFAULT || errno == EINVAL);
  return rv == -1 && errno == EFAULT;
}

static bool
HasSeccompTSync()
{
  
  
  if (getenv("MOZ_FAKE_NO_SECCOMP_TSYNC")) {
    return false;
  }
  int rv = syscall(__NR_seccomp, SECCOMP_SET_MODE_FILTER,
                   SECCOMP_FILTER_FLAG_TSYNC, nullptr);
  MOZ_DIAGNOSTIC_ASSERT(rv == -1, "seccomp(..., SECCOMP_FILTER_FLAG_TSYNC,"
                        " nullptr) didn't fail");
  MOZ_DIAGNOSTIC_ASSERT(errno == EFAULT || errno == EINVAL || errno == ENOSYS);
  return rv == -1 && errno == EFAULT;
}

static bool
HasUserNamespaceSupport()
{
  
  
  
  
  
  
  
  
  
  static const char* const paths[] = {
    "/proc/self/ns/user",
    "/proc/self/ns/pid",
    "/proc/self/ns/net",
    "/proc/self/ns/ipc",
  };
  for (size_t i = 0; i < ArrayLength(paths); ++i) {
    if (access(paths[i], F_OK) == -1) {
      MOZ_ASSERT(errno == ENOENT);
      return false;
    }
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

  
  
  
  
  if (syscall(__NR_unshare, 0) != 0) {
#ifdef MOZ_VALGRIND
    MOZ_ASSERT(errno == ENOSYS);
#else
    
    
    MOZ_ASSERT(false);
#endif
    return false;
  }

  pid_t pid = syscall(__NR_clone, SIGCHLD | CLONE_NEWUSER,
                      nullptr, nullptr, nullptr, nullptr);
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
  
  bool waitpid_ok = HANDLE_EINTR(waitpid(pid, nullptr, 0)) == pid;
  MOZ_ASSERT(waitpid_ok);
  if (!waitpid_ok) {
    return false;
  }
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
