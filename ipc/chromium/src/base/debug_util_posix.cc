



#include "build/build_config.h"
#include "base/debug_util.h"

#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/basictypes.h"
#include "base/eintr_wrapper.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/string_piece.h"


bool DebugUtil::SpawnDebuggerOnProcess(unsigned ) {
  NOTIMPLEMENTED();
  return false;
}

#if defined(OS_MACOSX)




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
    getpid()
  };

  
  
  struct kinfo_proc info;
  size_t info_size = sizeof(info);

  int sysctl_result = sysctl(mib, arraysize(mib), &info, &info_size, NULL, 0);
  DCHECK(sysctl_result == 0);
  if (sysctl_result != 0) {
    is_set = true;
    being_debugged = false;
    return being_debugged;
  }

  
  is_set = true;
  being_debugged = (info.kp_proc.p_flag & P_TRACED) != 0;
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
#if !defined(ARCH_CPU_ARM_FAMILY)
  asm ("int3");
#endif
}

StackTrace::StackTrace() {
  const int kMaxCallers = 256;

  void* callers[kMaxCallers];
  int count = backtrace(callers, kMaxCallers);

  
  
  
  if (count > 0) {
    trace_.resize(count);
    memcpy(&trace_[0], callers, sizeof(callers[0]) * count);
  } else {
    trace_.resize(0);
  }
}

void StackTrace::PrintBacktrace() {
  fflush(stderr);
  backtrace_symbols_fd(&trace_[0], trace_.size(), STDERR_FILENO);
}

void StackTrace::OutputToStream(std::ostream* os) {
#ifdef CHROMIUM_MOZILLA_BUILD
  return;
#else
  scoped_ptr_malloc<char*> trace_symbols(
      backtrace_symbols(&trace_[0], trace_.size()));

  
  
  if (trace_symbols.get() == NULL) {
    (*os) << "Unable get symbols for backtrace (" << strerror(errno)
          << "). Dumping raw addresses in trace:\n";
    for (size_t i = 0; i < trace_.size(); ++i) {
      (*os) << "\t" << trace_[i] << "\n";
    }
  } else {
    (*os) << "Backtrace:\n";
    for (size_t i = 0; i < trace_.size(); ++i) {
      (*os) << "\t" << trace_symbols.get()[i] << "\n";
    }
  }
#endif
}
