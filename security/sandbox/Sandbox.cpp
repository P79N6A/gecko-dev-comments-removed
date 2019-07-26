





#include <unistd.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/prctl.h>
#include <signal.h>
#include <string.h>

#include "mozilla/Util.h"
#if defined(ANDROID)
#include "android_ucontext.h"
#include <android/log.h>
#endif
#include "seccomp_filter.h"

#include "linux_seccomp.h"
#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1
#endif
#include "prlog.h"

namespace mozilla {
#if defined(ANDROID)
#define LOG_ERROR(args...) __android_log_print(ANDROID_LOG_ERROR, "Sandbox", ## args)
#elif defined(PR_LOGGING)
static PRLogModuleInfo* gSeccompSandboxLog;
#define LOG_ERROR(args...) PR_LOG(gSeccompSandboxLog, PR_LOG_ERROR, ## args)
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










#ifdef MOZ_CONTENT_SANDBOX_REPORTER
static void
Reporter(int nr, siginfo_t *info, void *void_context)
{
  ucontext_t *ctx = static_cast<ucontext_t*>(void_context);
  unsigned int syscall, arg1;

  if (nr != SIGSYS) {
    return;
  }
  if (info->si_code != SYS_SECCOMP) {
    return;
  }
  if (!ctx) {
    return;
  }

  syscall = SECCOMP_SYSCALL(ctx);
  arg1 = SECCOMP_PARM1(ctx);

  LOG_ERROR("PID %u is missing syscall %u, arg1 %u\n", getpid(), syscall, arg1);

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
  if (sigaction(SIGSYS, &act, NULL) < 0) {
    return -1;
  }
  if (sigemptyset(&mask) ||
    sigaddset(&mask, SIGSYS) ||
    sigprocmask(SIG_UNBLOCK, &mask, NULL)) {
      return -1;
  }
  return 0;
}
#endif












static int
InstallSyscallFilter(void)
{
  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
    return 1;
  }

  if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &seccomp_prog, 0, 0)) {
    return 1;
  }
  return 0;
}








void
SetCurrentProcessSandbox(void)
{
#if !defined(ANDROID) && defined(PR_LOGGING)
  if (!gSeccompSandboxLog) {
    gSeccompSandboxLog = PR_NewLogModule("SeccompSandbox");
  }
  PR_ASSERT(gSeccompSandboxLog);
#endif

#ifdef MOZ_CONTENT_SANDBOX_REPORTER
  if (InstallSyscallReporter()) {
    LOG_ERROR("install_syscall_reporter() failed\n");
    




    
  }

#endif

  if (InstallSyscallFilter()) {
    LOG_ERROR("install_syscall_filter() failed\n");
    




    
  }

}
} 

