





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
#include "sandbox/linux/bpf_dsl/dump_bpf.h"
#include "sandbox/linux/bpf_dsl/policy.h"
#include "sandbox/linux/bpf_dsl/policy_compiler.h"
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"
#include "sandbox/linux/seccomp-bpf/trap.h"
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



static SandboxOpenedFile gMediaPluginFile;
#endif

static UniquePtr<SandboxChroot> gChrootHelper;
static void (*gChromiumSigSysHandler)(int, siginfo_t*, void*);



static bool
ContextIsError(const ucontext_t *aContext, int aError)
{
  
  
  typedef decltype(+SECCOMP_RESULT(aContext)) reg_t;

#ifdef __mips__
  return SECCOMP_PARM4(aContext) != 0
    && SECCOMP_RESULT(aContext) == static_cast<reg_t>(aError);
#else
  return SECCOMP_RESULT(aContext) == static_cast<reg_t>(-aError);
#endif
}













static void
SigSysHandler(int nr, siginfo_t *info, void *void_context)
{
  ucontext_t *ctx = static_cast<ucontext_t*>(void_context);
  
  
  MOZ_DIAGNOSTIC_ASSERT(ctx);
  if (!ctx) {
    return;
  }

  
  
  ucontext_t savedCtx = *ctx;

  gChromiumSigSysHandler(nr, info, ctx);
  if (!ContextIsError(ctx, ENOSYS)) {
    return;
  }

  pid_t pid = getpid();
  unsigned long syscall_nr = SECCOMP_SYSCALL(&savedCtx);
  unsigned long args[6];
  args[0] = SECCOMP_PARM1(&savedCtx);
  args[1] = SECCOMP_PARM2(&savedCtx);
  args[2] = SECCOMP_PARM3(&savedCtx);
  args[3] = SECCOMP_PARM4(&savedCtx);
  args[4] = SECCOMP_PARM5(&savedCtx);
  args[5] = SECCOMP_PARM6(&savedCtx);

  
  
  SANDBOX_LOG_ERROR("seccomp sandbox violation: pid %d, syscall %lu,"
                    " args %lu %lu %lu %lu %lu %lu.  Killing process.",
                    pid, syscall_nr,
                    args[0], args[1], args[2], args[3], args[4], args[5]);

  
  info->si_addr = reinterpret_cast<void*>(syscall_nr);

  gSandboxCrashFunc(nr, info, &savedCtx);
  _exit(127);
}









static void
InstallSigSysHandler(void)
{
  struct sigaction act;

  
  unused << sandbox::Trap::Registry();

  
  

  if (sigaction(SIGSYS, nullptr, &act) != 0) {
    MOZ_CRASH("Couldn't read old SIGSYS disposition");
  }
  if ((act.sa_flags & SA_SIGINFO) != SA_SIGINFO) {
    MOZ_CRASH("SIGSYS not already set to a siginfo handler?");
  }
  MOZ_RELEASE_ASSERT(act.sa_sigaction);
  gChromiumSigSysHandler = act.sa_sigaction;
  act.sa_sigaction = SigSysHandler;
  
  
  MOZ_ASSERT(act.sa_flags & SA_NODEFER);
  act.sa_flags |= SA_NODEFER;
  if (sigaction(SIGSYS, &act, nullptr) < 0) {
    MOZ_CRASH("Couldn't change SIGSYS disposition");
  }
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



static mozilla::Atomic<int> gSetSandboxDone;

static sock_fprog gSetSandboxFilter;








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
    InstallSyscallFilter(&gSetSandboxFilter);
    return true;
  }
  return false;
}

static void
SetThreadSandboxHandler(int signum)
{
  
  
  if (SetThreadSandbox()) {
    gSetSandboxDone = 2;
  } else {
    gSetSandboxDone = 1;
  }
  
  
  syscall(__NR_futex, reinterpret_cast<int*>(&gSetSandboxDone),
          FUTEX_WAKE, 1);
}

static void
BroadcastSetThreadSandbox(UniquePtr<sock_filter[]> aProgram, size_t aProgLen)
{
  int signum;
  pid_t pid, tid, myTid;
  DIR *taskdp;
  struct dirent *de;

  
  
  
  gSetSandboxFilter.filter = aProgram.get();
  gSetSandboxFilter.len = static_cast<unsigned short>(aProgLen);
  MOZ_RELEASE_ASSERT(static_cast<size_t>(gSetSandboxFilter.len) == aProgLen);

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
      
      gSetSandboxDone = 0;
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
        
        if (syscall(__NR_futex, reinterpret_cast<int*>(&gSetSandboxDone),
                  FUTEX_WAIT, 0, &futexTimeout) != 0) {
          if (errno != EWOULDBLOCK && errno != ETIMEDOUT && errno != EINTR) {
            SANDBOX_LOG_ERROR("FUTEX_WAIT: %s\n", strerror(errno));
            MOZ_CRASH();
          }
        }
        
        if (gSetSandboxDone > 0) {
          if (gSetSandboxDone == 2) {
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
  gSetSandboxFilter.filter = nullptr;
}


static void
SetCurrentProcessSandbox(UniquePtr<sandbox::bpf_dsl::Policy> aPolicy)
{
  MOZ_ASSERT(gSandboxCrashFunc);

  
  
  sandbox::bpf_dsl::PolicyCompiler compiler(aPolicy.get(),
                                            sandbox::Trap::Registry());
  auto program = compiler.Compile();
  if (SandboxInfo::Get().Test(SandboxInfo::kVerbose)) {
    sandbox::bpf_dsl::DumpBPF::PrintProgram(*program);
  }

  InstallSigSysHandler();

#ifdef MOZ_ASAN
  __sanitizer_sandbox_arguments asanArgs;
  asanArgs.coverage_sandboxed = 1;
  asanArgs.coverage_fd = -1;
  asanArgs.coverage_max_block_size = 0;
  __sanitizer_sandbox_on_notify(&asanArgs);
#endif

  
  UniquePtr<sock_filter[]> flatProgram(new sock_filter[program->size()]);
  for (auto i = program->begin(); i != program->end(); ++i) {
    flatProgram[i - program->begin()] = *i;
  }

  BroadcastSetThreadSandbox(Move(flatProgram), program->size());
}

void
SandboxEarlyInit(GeckoProcessType aType, bool aIsNuwa)
{
  
  
  
  
  
  
  
  if (aIsNuwa) {
    return;
  }

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

  SetCurrentProcessSandbox(GetContentSandboxPolicy());
}
#endif 

#ifdef MOZ_GMP_SANDBOX











void
SetMediaPluginSandbox(const char *aFilePath)
{
  if (!SandboxInfo::Get().Test(SandboxInfo::kEnabledForMedia)) {
    return;
  }

  MOZ_ASSERT(!gMediaPluginFile.mPath);
  if (aFilePath) {
    gMediaPluginFile.mPath = strdup(aFilePath);
    gMediaPluginFile.mFd = open(aFilePath, O_RDONLY | O_CLOEXEC);
    if (gMediaPluginFile.mFd == -1) {
      SANDBOX_LOG_ERROR("failed to open plugin file %s: %s",
                        aFilePath, strerror(errno));
      MOZ_CRASH();
    }
  } else {
    gMediaPluginFile.mFd = -1;
  }
  
  SetCurrentProcessSandbox(GetMediaSandboxPolicy(&gMediaPluginFile));
}
#endif 

} 
