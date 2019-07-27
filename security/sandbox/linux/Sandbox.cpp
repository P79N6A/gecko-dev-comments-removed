





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
#include <fcntl.h>

#include "mozilla/Atomics.h"
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

#include "linux_seccomp.h"
#include "SandboxFilter.h"

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1
#endif
#include "prlog.h"
#include "prenv.h"


#include "sandbox/linux/seccomp-bpf/die.h"

namespace mozilla {
#if defined(ANDROID)
#define LOG_ERROR(args...) __android_log_print(ANDROID_LOG_ERROR, "Sandbox", ## args)
#elif defined(PR_LOGGING)
static PRLogModuleInfo* gSeccompSandboxLog;
#define LOG_ERROR(args...) PR_LOG(mozilla::gSeccompSandboxLog, PR_LOG_ERROR, (args))
#else
#define LOG_ERROR(args...)
#endif

#ifdef MOZ_GMP_SANDBOX



static int gMediaPluginFileDesc = -1;
static nsCString gMediaPluginFilePath;
#endif






static void
SandboxLogJSStack(void)
{
  if (!NS_IsMainThread()) {
    
    
    
    return;
  }
  nsCOMPtr<nsIStackFrame> frame = dom::GetCurrentJSStack();
  for (int i = 0; frame != nullptr; ++i) {
    nsAutoString fileName, funName;
    int32_t lineNumber;

    
    fileName.SetIsVoid(true);
    unused << frame->GetFilename(fileName);
    lineNumber = 0;
    unused << frame->GetLineNumber(&lineNumber);
    funName.SetIsVoid(true);
    unused << frame->GetName(funName);

    if (!funName.IsVoid() || !fileName.IsVoid()) {
      LOG_ERROR("JS frame %d: %s %s line %d", i,
                funName.IsVoid() ?
                  "(anonymous)" : NS_ConvertUTF16toUTF8(funName).get(),
                fileName.IsVoid() ?
                  "(no file)" : NS_ConvertUTF16toUTF8(fileName).get(),
                lineNumber);
    }

    nsCOMPtr<nsIStackFrame> nextFrame;
    nsresult rv = frame->GetCaller(getter_AddRefs(nextFrame));
    NS_ENSURE_SUCCESS_VOID(rv);
    frame = nextFrame;
  }
}










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

#ifdef MOZ_GMP_SANDBOX
  if (syscall_nr == __NR_open && !gMediaPluginFilePath.IsEmpty()) {
    const char *path = reinterpret_cast<const char*>(args[0]);
    int flags = int(args[1]);

    if ((flags & O_ACCMODE) != O_RDONLY) {
      LOG_ERROR("non-read-only open of file %s attempted (flags=0%o)",
                path, flags);
    } else if (strcmp(path, gMediaPluginFilePath.get()) != 0) {
      LOG_ERROR("attempt to open file %s which is not the media plugin %s",
                path, gMediaPluginFilePath.get());
    } else if (gMediaPluginFileDesc == -1) {
      LOG_ERROR("multiple opens of media plugin file unimplemented");
    } else {
      SECCOMP_RESULT(ctx) = gMediaPluginFileDesc;
      gMediaPluginFileDesc = -1;
      return;
    }
  }
#endif

  LOG_ERROR("seccomp sandbox violation: pid %d, syscall %lu, args %lu %lu %lu"
            " %lu %lu %lu.  Killing process.", pid, syscall_nr,
            args[0], args[1], args[2], args[3], args[4], args[5]);

#ifdef MOZ_CRASHREPORTER
  
  info->si_addr = reinterpret_cast<void*>(syscall_nr);
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












static int
InstallSyscallFilter(const sock_fprog *prog)
{
  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
    return 1;
  }

  if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, (unsigned long)prog, 0, 0)) {
    return 1;
  }
  return 0;
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
  bool didAnything = false;

  if (prctl(PR_GET_SECCOMP, 0, 0, 0, 0) == 0) {
    if (InstallSyscallFilter(sSetSandboxFilter) == 0) {
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
BroadcastSetThreadSandbox(SandboxType aType)
{
  int signum;
  pid_t pid, tid, myTid;
  DIR *taskdp;
  struct dirent *de;
  SandboxFilter filter(&sSetSandboxFilter, aType,
                       PR_GetEnv("MOZ_SANDBOX_VERBOSE"));

  static_assert(sizeof(mozilla::Atomic<int>) == sizeof(int),
                "mozilla::Atomic<int> isn't represented by an int");
  pid = getpid();
  myTid = syscall(__NR_gettid);
  taskdp = opendir("/proc/self/task");
  if (taskdp == nullptr) {
    LOG_ERROR("opendir /proc/self/task: %s\n", strerror(errno));
    MOZ_CRASH();
  }
  signum = FindFreeSignalNumber();
  if (signum == 0) {
    LOG_ERROR("No available signal numbers!");
    MOZ_CRASH();
  }
  void (*oldHandler)(int);
  oldHandler = signal(signum, SetThreadSandboxHandler);
  if (oldHandler != SIG_DFL) {
    
    LOG_ERROR("signal %d in use by handler %p!\n", signum, oldHandler);
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
  oldHandler = signal(signum, SIG_DFL);
  if (oldHandler != SetThreadSandboxHandler) {
    
    LOG_ERROR("handler for signal %d was changed to %p!", signum, oldHandler);
    MOZ_CRASH();
  }
  unused << closedir(taskdp);
  
  SetThreadSandbox();
}



static bool
IsSandboxingSupported(void)
{
  return prctl(PR_GET_SECCOMP) != -1;
}


static void
SetCurrentProcessSandbox(SandboxType aType)
{
#if !defined(ANDROID) && defined(PR_LOGGING)
  if (!gSeccompSandboxLog) {
    gSeccompSandboxLog = PR_NewLogModule("SeccompSandbox");
  }
  PR_ASSERT(gSeccompSandboxLog);
#endif

  if (InstallSyscallReporter()) {
    LOG_ERROR("install_syscall_reporter() failed\n");
  }

  if (IsSandboxingSupported()) {
    BroadcastSetThreadSandbox(aType);
  }
}

#ifdef MOZ_CONTENT_SANDBOX






void
SetContentProcessSandbox()
{
  if (PR_GetEnv("MOZ_DISABLE_CONTENT_SANDBOX")) {
    return;
  }

  SetCurrentProcessSandbox(kSandboxContentProcess);
}
#endif 

#ifdef MOZ_GMP_SANDBOX











void
SetMediaPluginSandbox(const char *aFilePath)
{
  if (PR_GetEnv("MOZ_DISABLE_GMP_SANDBOX")) {
    return;
  }

  if (aFilePath) {
    gMediaPluginFilePath.Assign(aFilePath);
    gMediaPluginFileDesc = open(aFilePath, O_RDONLY | O_CLOEXEC);
    if (gMediaPluginFileDesc == -1) {
      LOG_ERROR("failed to open plugin file %s: %s", aFilePath, strerror(errno));
      MOZ_CRASH();
    }
  }
  
  SetCurrentProcessSandbox(kSandboxMediaPlugin);
}
#endif 

} 




namespace sandbox {

void
Die::SandboxDie(const char* msg, const char* file, int line)
{
  LOG_ERROR("%s:%d: %s\n", file, line, msg);
  _exit(127);
}

} 



#include "base/logging.h"
namespace logging {

LogMessage::LogMessage(const char *file, int line, int)
  : file_(file), line_(line)
{
  MOZ_CRASH("Unexpected call to logging::LogMessage::LogMessage");
}

LogMessage::~LogMessage() {
  MOZ_CRASH("Unexpected call to logging::LogMessage::~LogMessage");
}

} 
