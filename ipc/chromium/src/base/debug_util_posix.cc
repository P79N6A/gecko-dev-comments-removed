



#include "build/build_config.h"
#include "base/debug_util.h"

#define MOZ_HAVE_EXECINFO_H (defined(OS_LINUX) && !defined(ANDROID))

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#if MOZ_HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#if defined(OS_MACOSX) || defined(OS_BSD)
#include <sys/sysctl.h>
#endif

#if defined(OS_DRAGONFLY) || defined(OS_FREEBSD)
#include <sys/user.h>
#endif

#include "base/basictypes.h"
#include "base/eintr_wrapper.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/string_piece.h"

#if defined(OS_NETBSD)
#undef KERN_PROC
#define KERN_PROC KERN_PROC2
#define KINFO_PROC struct kinfo_proc2
#else
#define KINFO_PROC struct kinfo_proc
#endif

#if defined(OS_MACOSX)
#define KP_FLAGS kp_proc.p_flag
#elif defined(OS_DRAGONFLY)
#define KP_FLAGS kp_flags
#elif defined(OS_FREEBSD)
#define KP_FLAGS ki_flag
#else
#define KP_FLAGS p_flag
#endif


bool DebugUtil::SpawnDebuggerOnProcess(unsigned ) {
  NOTIMPLEMENTED();
  return false;
}

#if defined(OS_MACOSX) || defined(OS_BSD)




bool DebugUtil::BeingDebugged() {
  
  
  static bool is_set = false;
  static bool being_debugged = false;

  if (is_set) {
    return being_debugged;
  }

  
  
  int mib[] = {
    CTL_KERN,
    KERN_PROC,
    KERN_PROC_PID,
    getpid(),
#if defined(OS_NETBSD) || defined(OS_OPENBSD)
    sizeof(KINFO_PROC),
    1,
#endif
  };

  
  
  KINFO_PROC info;
  size_t info_size = sizeof(info);

  int sysctl_result = sysctl(mib, arraysize(mib), &info, &info_size, NULL, 0);
  DCHECK(sysctl_result == 0);
  if (sysctl_result != 0) {
    is_set = true;
    being_debugged = false;
    return being_debugged;
  }

  
  is_set = true;
  being_debugged = (info.KP_FLAGS & P_TRACED) != 0;
  return being_debugged;
}

#elif defined(OS_LINUX)






bool DebugUtil::BeingDebugged() {
  int status_fd = open("/proc/self/status", O_RDONLY);
  if (status_fd == -1)
    return false;

  
  
  
  char buf[1024];

  ssize_t num_read = HANDLE_EINTR(read(status_fd, buf, sizeof(buf)));
  HANDLE_EINTR(close(status_fd));

  if (num_read <= 0)
    return false;

  StringPiece status(buf, num_read);
  StringPiece tracer("TracerPid:\t");

  StringPiece::size_type pid_index = status.find(tracer);
  if (pid_index == StringPiece::npos)
    return false;

  
  pid_index += tracer.size();
  return pid_index < status.size() && status[pid_index] != '0';
}

#endif  


void DebugUtil::BreakDebugger() {
#if defined(ARCH_CPU_X86_FAMILY)
  asm ("int3");
#endif
}

StackTrace::StackTrace() {
  const int kMaxCallers = 256;

  void* callers[kMaxCallers];
#if MOZ_HAVE_EXECINFO_H
  int count = backtrace(callers, kMaxCallers);
#else
  int count = 0;
#endif

  
  
  
  if (count > 0) {
    trace_.resize(count);
    memcpy(&trace_[0], callers, sizeof(callers[0]) * count);
  } else {
    trace_.resize(0);
  }
}

void StackTrace::PrintBacktrace() {
  fflush(stderr);
#if MOZ_HAVE_EXECINFO_H
  backtrace_symbols_fd(&trace_[0], trace_.size(), STDERR_FILENO);
#endif
}

void StackTrace::OutputToStream(std::ostream* os) {
  return;
}
