





#include "mozilla/Sandbox.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <signal.h>
#include <string.h>
#include <linux/futex.h>
#include <sys/time.h>
#include <dirent.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include "mozilla/Atomics.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/NullPtr.h"
#include "mozilla/unused.h"
#include "mozilla/dom/Exceptions.h"
#include "nsString.h"
#include "nsThreadUtils.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

#if defined(ANDROID)
#include "android_ucontext.h"
#include <android/log.h>
#endif

#if defined(MOZ_CONTENT_SANDBOX)
#include "seccomp_filter.h"
#include "linux_seccomp.h"
#endif

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1
#endif
#include "prlog.h"
#include "prenv.h"

namespace mozilla {
#if defined(ANDROID)
#define LOG_ERROR(args...) __android_log_print(ANDROID_LOG_ERROR, "Sandbox", ## args)
#elif defined(PR_LOGGING)
static PRLogModuleInfo* gSeccompSandboxLog;
#define LOG_ERROR(args...) PR_LOG(gSeccompSandboxLog, PR_LOG_ERROR, (args))
#else
#define LOG_ERROR(args...)
#endif

struct sock_filter seccomp_filter[] = {
  VALIDATE_ARCHITECTURE,
  EXAMINE_SYSCALL,
  SECCOMP_WHITELIST,
#ifdef MOZ_CONTENT_SANDBOX_REPORTER
  TRAP_PROCESS,
#else
  KILL_PROCESS,
#endif
};

struct sock_fprog seccomp_prog = {
  (unsigned short)MOZ_ARRAY_LENGTH(seccomp_filter),
  seccomp_filter,
};






static void
SandboxLogJSStack(void)
{
  if (!NS_IsMainThread()) {
    
    
    
    return;
  }
  nsCOMPtr<nsIStackFrame> frame = dom::GetCurrentJSStack();
  for (int i = 0; frame != nullptr; ++i) {
    nsAutoCString fileName, funName;
    int32_t lineNumber;

    
    fileName.SetIsVoid(true);
    unused << frame->GetFilename(fileName);
    lineNumber = 0;
    unused << frame->GetLineNumber(&lineNumber);
    funName.SetIsVoid(true);
    unused << frame->GetName(funName);

    if (!funName.IsVoid() || !fileName.IsVoid()) {
      LOG_ERROR("JS frame %d: %s %s line %d", i,
                funName.IsVoid() ? "(anonymous)" : funName.get(),
                fileName.IsVoid() ? "(no file)" : fileName.get(),
                lineNumber);
    }

    nsCOMPtr<nsIStackFrame> nextFrame;
    nsresult rv = frame->GetCaller(getter_AddRefs(nextFrame));
    NS_ENSURE_SUCCESS_VOID(rv);
    frame = nextFrame;
  }
}










#ifdef MOZ_CONTENT_SANDBOX_REPORTER
static void
Reporter(int nr, siginfo_t *info, void *void_context)
{
  ucontext_t *ctx = static_cast<ucontext_t*>(void_context);
  unsigned long syscall_nr, args[6];
  pid_t pid = getpid(), tid = syscall(__NR_gettid);

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

  LOG_ERROR("seccomp sandbox violation: pid %d, syscall %lu, args %lu %lu %lu"
            " %lu %lu %lu.  Killing process.", pid, syscall_nr,
            args[0], args[1], args[2], args[3], args[4], args[5]);

#ifdef MOZ_CRASHREPORTER
  bool dumped = CrashReporter::WriteMinidumpForSigInfo(nr, info, void_context);
  if (!dumped) {
    LOG_ERROR("Failed to write minidump");
  }
#endif

  
  SandboxLogJSStack();

  
  
  
  signal(SIGSYS, SIG_DFL);
  syscall(__NR_tgkill, pid, tid, nr);
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
#endif












static int
InstallSyscallFilter(void)
{
#ifdef MOZ_DMD
  char* e = PR_GetEnv("DMD");
  if (e && strcmp(e, "") != 0 && strcmp(e, "0") != 0) {
    LOG_ERROR("SANDBOX DISABLED FOR DMD!  See bug 956961.");
    
    
    return 1;
  }
#endif
  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
    return 1;
  }

  if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &seccomp_prog, 0, 0)) {
    return 1;
  }
  return 0;
}



static mozilla::Atomic<int> sSetSandboxDone;


static const int sSetSandboxSignum = SIGRTMIN + 3;

static bool
SetThreadSandbox()
{
  bool didAnything = false;

  if (PR_GetEnv("MOZ_DISABLE_CONTENT_SANDBOX") == nullptr &&
      prctl(PR_GET_SECCOMP, 0, 0, 0, 0) == 0) {
    if (InstallSyscallFilter() == 0) {
      didAnything = true;
    }
    




  }
  return didAnything;
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
BroadcastSetThreadSandbox()
{
  pid_t pid, tid;
  DIR *taskdp;
  struct dirent *de;

  static_assert(sizeof(mozilla::Atomic<int>) == sizeof(int),
                "mozilla::Atomic<int> isn't represented by an int");
  MOZ_ASSERT(NS_IsMainThread());
  pid = getpid();
  taskdp = opendir("/proc/self/task");
  if (taskdp == nullptr) {
    LOG_ERROR("opendir /proc/self/task: %s\n", strerror(errno));
    MOZ_CRASH();
  }
  if (signal(sSetSandboxSignum, SetThreadSandboxHandler) != SIG_DFL) {
    LOG_ERROR("signal %d in use!\n", sSetSandboxSignum);
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
      if (tid == pid) {
        
        
        continue;
      }
      
      sSetSandboxDone = 0;
      if (syscall(__NR_tgkill, pid, tid, sSetSandboxSignum) != 0) {
        if (errno == ESRCH) {
          LOG_ERROR("Thread %d unexpectedly exited.", tid);
          
          sandboxProgress = true;
          continue;
        }
        LOG_ERROR("tgkill(%d,%d): %s\n", pid, tid, strerror(errno));
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
            LOG_ERROR("FUTEX_WAIT: %s\n", strerror(errno));
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
            LOG_ERROR("Thread %d unexpectedly exited.", tid);
          }
          
          
          
          sandboxProgress = true;
          break;
        }
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec > timeLimit.tv_nsec ||
            (now.tv_sec == timeLimit.tv_nsec &&
             now.tv_nsec > timeLimit.tv_nsec)) {
          LOG_ERROR("Thread %d unresponsive for %d seconds.  Killing process.",
                    tid, crashDelay);
          MOZ_CRASH();
        }
      }
    }
    rewinddir(taskdp);
  } while (sandboxProgress);
  unused << signal(sSetSandboxSignum, SIG_DFL);
  unused << closedir(taskdp);
  
  SetThreadSandbox();
}







void
SetCurrentProcessSandbox()
{
#if !defined(ANDROID) && defined(PR_LOGGING)
  if (!gSeccompSandboxLog) {
    gSeccompSandboxLog = PR_NewLogModule("SeccompSandbox");
  }
  PR_ASSERT(gSeccompSandboxLog);
#endif

#if defined(MOZ_CONTENT_SANDBOX_REPORTER)
  if (InstallSyscallReporter()) {
    LOG_ERROR("install_syscall_reporter() failed\n");
  }
#endif

  BroadcastSetThreadSandbox();
}

} 

