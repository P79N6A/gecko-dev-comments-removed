





#include "Sandbox.h"

#include "LinuxCapabilities.h"
#include "LinuxSched.h"
#include "SandboxChroot.h"
#include "SandboxFilter.h"
#include "SandboxInternal.h"
#include "SandboxLogging.h"
#include "SandboxUtil.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/futex.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "mozilla/Atomics.h"
#include "mozilla/SandboxInfo.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/unused.h"
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"
#if defined(ANDROID)
#include "sandbox/linux/services/android_ucontext.h"
#endif
#include "sandbox/linux/services/linux_syscalls.h"

#ifdef MOZ_ASAN


extern "C" {
namespace __sanitizer {

typedef signed long sptr;
} 

typedef struct {
  int coverage_sandboxed;
  __sanitizer::sptr coverage_fd;
  unsigned int coverage_max_block_size;
} __sanitizer_sandbox_arguments;

MOZ_IMPORT_API void
__sanitizer_sandbox_on_notify(__sanitizer_sandbox_arguments *args);
} 
#endif 

namespace mozilla {

#ifdef ANDROID
SandboxCrashFunc gSandboxCrashFunc;
#endif

#ifdef MOZ_GMP_SANDBOX



static int gMediaPluginFileDesc = -1;
static const char *gMediaPluginFilePath;
#endif

static UniquePtr<SandboxChroot> gChrootHelper;










static void
Reporter(int nr, siginfo_t *info, void *void_context)
{
  ucontext_t *ctx = static_cast<ucontext_t*>(void_context);
  unsigned long syscall_nr, args[6];
  pid_t pid = getpid();

  if (nr != SIGSYS) {
    return;
  }
  if (info->si_code != SYS_SECCOMP) {
    return;
  }
  if (!ctx) {
    return;
  }

  syscall_nr = SECCOMP_SYSCALL(ctx);
  args[0] = SECCOMP_PARM1(ctx);
  args[1] = SECCOMP_PARM2(ctx);
  args[2] = SECCOMP_PARM3(ctx);
  args[3] = SECCOMP_PARM4(ctx);
  args[4] = SECCOMP_PARM5(ctx);
  args[5] = SECCOMP_PARM6(ctx);

#if defined(ANDROID) && ANDROID_VERSION < 16
  
  
  if (syscall_nr == __NR_tkill) {
    intptr_t ret = syscall(__NR_tgkill, getpid(), args[0], args[1]);
    if (ret < 0) {
      ret = -errno;
    }
    SECCOMP_RESULT(ctx) = ret;
    return;
  }
#endif

#ifdef MOZ_GMP_SANDBOX
  if (syscall_nr == __NR_open && gMediaPluginFilePath) {
    const char *path = reinterpret_cast<const char*>(args[0]);
    int flags = int(args[1]);

    if ((flags & O_ACCMODE) != O_RDONLY) {
      SANDBOX_LOG_ERROR("non-read-only open of file %s attempted (flags=0%o)",
                        path, flags);
    } else if (strcmp(path, gMediaPluginFilePath) != 0) {
      SANDBOX_LOG_ERROR("attempt to open file %s which is not the media plugin"
                        " %s", path, gMediaPluginFilePath);
    } else if (gMediaPluginFileDesc == -1) {
      SANDBOX_LOG_ERROR("multiple opens of media plugin file unimplemented");
    } else {
      SECCOMP_RESULT(ctx) = gMediaPluginFileDesc;
      gMediaPluginFileDesc = -1;
      return;
    }
  }
#endif

  SANDBOX_LOG_ERROR("seccomp sandbox violation: pid %d, syscall %lu,"
                    " args %lu %lu %lu %lu %lu %lu.  Killing process.",
                    pid, syscall_nr,
                    args[0], args[1], args[2], args[3], args[4], args[5]);

  
  info->si_addr = reinterpret_cast<void*>(syscall_nr);

  gSandboxCrashFunc(nr, info, void_context);
  _exit(127);
}















static int
InstallSyscallReporter(void)
{
  struct sigaction act;
  sigset_t mask;
  memset(&act, 0, sizeof(act));
  sigemptyset(&mask);
  sigaddset(&mask, SIGSYS);

  act.sa_sigaction = &Reporter;
  act.sa_flags = SA_SIGINFO | SA_NODEFER;
  if (sigaction(SIGSYS, &act, nullptr) < 0) {
    return -1;
  }
  if (sigemptyset(&mask) ||
    sigaddset(&mask, SIGSYS) ||
    sigprocmask(SIG_UNBLOCK, &mask, nullptr)) {
      return -1;
  }
  return 0;
}













static void
InstallSyscallFilter(const sock_fprog *prog)
{
  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
    SANDBOX_LOG_ERROR("prctl(PR_SET_NO_NEW_PRIVS) failed: %s", strerror(errno));
    MOZ_CRASH("prctl(PR_SET_NO_NEW_PRIVS)");
  }

  if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, (unsigned long)prog, 0, 0)) {
    SANDBOX_LOG_ERROR("prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER) failed: %s",
                      strerror(errno));
    MOZ_CRASH("prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER)");
  }
}



static mozilla::Atomic<int> sSetSandboxDone;

static const sock_fprog *sSetSandboxFilter;








static int
FindFreeSignalNumber()
{
  for (int signum = SIGRTMIN; signum <= SIGRTMAX; ++signum) {
    struct sigaction sa;

    if (sigaction(signum, nullptr, &sa) == 0 &&
        (sa.sa_flags & SA_SIGINFO) == 0 &&
        sa.sa_handler == SIG_DFL) {
      return signum;
    }
  }
  return 0;
}



static bool
SetThreadSandbox()
{
  if (prctl(PR_GET_SECCOMP, 0, 0, 0, 0) == 0) {
    InstallSyscallFilter(sSetSandboxFilter);
    return true;
  }
  return false;
}

static void
SetThreadSandboxHandler(int signum)
{
  
  
  if (SetThreadSandbox()) {
    sSetSandboxDone = 2;
  } else {
    sSetSandboxDone = 1;
  }
  
  
  syscall(__NR_futex, reinterpret_cast<int*>(&sSetSandboxDone),
          FUTEX_WAKE, 1);
}

static void
BroadcastSetThreadSandbox(SandboxType aType)
{
  int signum;
  pid_t pid, tid, myTid;
  DIR *taskdp;
  struct dirent *de;
  SandboxFilter filter(&sSetSandboxFilter, aType,
                       SandboxInfo::Get().Test(SandboxInfo::kVerbose));

  static_assert(sizeof(mozilla::Atomic<int>) == sizeof(int),
                "mozilla::Atomic<int> isn't represented by an int");
  pid = getpid();
  myTid = syscall(__NR_gettid);
  taskdp = opendir("/proc/self/task");
  if (taskdp == nullptr) {
    SANDBOX_LOG_ERROR("opendir /proc/self/task: %s\n", strerror(errno));
    MOZ_CRASH();
  }

  if (gChrootHelper) {
    gChrootHelper->Invoke();
    gChrootHelper = nullptr;
  }

  signum = FindFreeSignalNumber();
  if (signum == 0) {
    SANDBOX_LOG_ERROR("No available signal numbers!");
    MOZ_CRASH();
  }
  void (*oldHandler)(int);
  oldHandler = signal(signum, SetThreadSandboxHandler);
  if (oldHandler != SIG_DFL) {
    
    SANDBOX_LOG_ERROR("signal %d in use by handler %p!\n", signum, oldHandler);
    MOZ_CRASH();
  }

  
  
  
  bool sandboxProgress;
  do {
    sandboxProgress = false;
    
    while ((de = readdir(taskdp))) {
      char *endptr;
      tid = strtol(de->d_name, &endptr, 10);
      if (*endptr != '\0' || tid <= 0) {
        
        continue;
      }
      if (tid == myTid) {
        
        
        continue;
      }
      
      sSetSandboxDone = 0;
      if (syscall(__NR_tgkill, pid, tid, signum) != 0) {
        if (errno == ESRCH) {
          SANDBOX_LOG_ERROR("Thread %d unexpectedly exited.", tid);
          
          sandboxProgress = true;
          continue;
        }
        SANDBOX_LOG_ERROR("tgkill(%d,%d): %s\n", pid, tid, strerror(errno));
        MOZ_CRASH();
      }
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      static const int crashDelay = 10; 
      struct timespec timeLimit;
      clock_gettime(CLOCK_MONOTONIC, &timeLimit);
      timeLimit.tv_sec += crashDelay;
      while (true) {
        static const struct timespec futexTimeout = { 0, 10*1000*1000 }; 
        
        if (syscall(__NR_futex, reinterpret_cast<int*>(&sSetSandboxDone),
                  FUTEX_WAIT, 0, &futexTimeout) != 0) {
          if (errno != EWOULDBLOCK && errno != ETIMEDOUT && errno != EINTR) {
            SANDBOX_LOG_ERROR("FUTEX_WAIT: %s\n", strerror(errno));
            MOZ_CRASH();
          }
        }
        
        if (sSetSandboxDone > 0) {
          if (sSetSandboxDone == 2) {
            sandboxProgress = true;
          }
          break;
        }
        
        if (syscall(__NR_tgkill, pid, tid, 0) != 0) {
          if (errno == ESRCH) {
            SANDBOX_LOG_ERROR("Thread %d unexpectedly exited.", tid);
          }
          
          
          
          sandboxProgress = true;
          break;
        }
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec > timeLimit.tv_nsec ||
            (now.tv_sec == timeLimit.tv_nsec &&
             now.tv_nsec > timeLimit.tv_nsec)) {
          SANDBOX_LOG_ERROR("Thread %d unresponsive for %d seconds."
                            "  Killing process.",
                            tid, crashDelay);
          MOZ_CRASH();
        }
      }
    }
    rewinddir(taskdp);
  } while (sandboxProgress);
  oldHandler = signal(signum, SIG_DFL);
  if (oldHandler != SetThreadSandboxHandler) {
    
    SANDBOX_LOG_ERROR("handler for signal %d was changed to %p!",
                      signum, oldHandler);
    MOZ_CRASH();
  }
  unused << closedir(taskdp);
  
  SetThreadSandbox();
}


static void
SetCurrentProcessSandbox(SandboxType aType)
{
  MOZ_ASSERT(gSandboxCrashFunc);

  if (InstallSyscallReporter()) {
    SANDBOX_LOG_ERROR("install_syscall_reporter() failed\n");
  }

#ifdef MOZ_ASAN
  __sanitizer_sandbox_arguments asanArgs;
  asanArgs.coverage_sandboxed = 1;
  asanArgs.coverage_fd = -1;
  asanArgs.coverage_max_block_size = 0;
  __sanitizer_sandbox_on_notify(&asanArgs);
#endif

  BroadcastSetThreadSandbox(aType);
}

void
SandboxEarlyInit(GeckoProcessType aType, bool aIsNuwa)
{
  MOZ_RELEASE_ASSERT(IsSingleThreaded());

  
  
  bool canChroot = false;
  bool canUnshareNet = false;
  bool canUnshareIPC = false;

  switch (aType) {
  case GeckoProcessType_Default:
    MOZ_ASSERT(false, "SandboxEarlyInit in parent process");
    return;
#ifdef MOZ_GMP_SANDBOX
  case GeckoProcessType_GMPlugin:
    canUnshareNet = true;
    canUnshareIPC = true;
    canChroot = true;
    break;
#endif
    
    
  default:
    
    break;
  }

  
  if (!canChroot && !canUnshareNet && !canUnshareIPC) {
    return;
  }

  
  const SandboxInfo info = SandboxInfo::Get();
  if (!info.Test(SandboxInfo::kHasUserNamespaces)) {
    return;
  }

  
  
  
  
  
  
  
  if (!UnshareUserNamespace()) {
    SANDBOX_LOG_ERROR("unshare(CLONE_NEWUSER): %s", strerror(errno));
    
    
    MOZ_CRASH("unshare(CLONE_NEWUSER)");
  }
  
  

  if (canUnshareIPC && syscall(__NR_unshare, CLONE_NEWIPC) != 0) {
    SANDBOX_LOG_ERROR("unshare(CLONE_NEWIPC): %s", strerror(errno));
    MOZ_CRASH("unshare(CLONE_NEWIPC)");
  }

  if (canUnshareNet && syscall(__NR_unshare, CLONE_NEWNET) != 0) {
    SANDBOX_LOG_ERROR("unshare(CLONE_NEWNET): %s", strerror(errno));
    MOZ_CRASH("unshare(CLONE_NEWNET)");
  }

  if (canChroot) {
    gChrootHelper = MakeUnique<SandboxChroot>();
    if (!gChrootHelper->Prepare()) {
      SANDBOX_LOG_ERROR("failed to set up chroot helper");
      MOZ_CRASH("SandboxChroot::Prepare");
    }
  }

  if (!LinuxCapabilities().SetCurrent()) {
    SANDBOX_LOG_ERROR("dropping capabilities: %s", strerror(errno));
    MOZ_CRASH("can't drop capabilities");
  }
}

#ifdef MOZ_CONTENT_SANDBOX






void
SetContentProcessSandbox()
{
  if (!SandboxInfo::Get().Test(SandboxInfo::kEnabledForContent)) {
    return;
  }

  SetCurrentProcessSandbox(kSandboxContentProcess);
}
#endif 

#ifdef MOZ_GMP_SANDBOX











void
SetMediaPluginSandbox(const char *aFilePath)
{
  if (!SandboxInfo::Get().Test(SandboxInfo::kEnabledForMedia)) {
    return;
  }

  if (aFilePath) {
    gMediaPluginFilePath = strdup(aFilePath);
    gMediaPluginFileDesc = open(aFilePath, O_RDONLY | O_CLOEXEC);
    if (gMediaPluginFileDesc == -1) {
      SANDBOX_LOG_ERROR("failed to open plugin file %s: %s",
                        aFilePath, strerror(errno));
      MOZ_CRASH();
    }
  }
  
  SetCurrentProcessSandbox(kSandboxMediaPlugin);
}
#endif 

} 
