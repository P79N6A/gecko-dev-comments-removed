



#include "sandbox/linux/seccomp-bpf/die.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <string>

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "sandbox/linux/seccomp-bpf/syscall.h"

namespace sandbox {

void Die::ExitGroup() {
  
  
  
  
  
  Syscall::Call(__NR_exit_group, 1);

  
  
  
  
  
  signal(SIGSEGV, SIG_DFL);
  Syscall::Call(__NR_prctl, PR_SET_DUMPABLE, (void*)0, (void*)0, (void*)0);
  if (*(volatile char*)0) {
  }

  
  
  
  
  
  for (;;) {
    Syscall::Call(__NR_exit_group, 1);
  }
}

void Die::SandboxDie(const char* msg, const char* file, int line) {
  if (simple_exit_) {
    LogToStderr(msg, file, line);
  } else {
    logging::LogMessage(file, line, logging::LOG_FATAL).stream() << msg;
  }
  ExitGroup();
}

void Die::RawSandboxDie(const char* msg) {
  if (!msg)
    msg = "";
  RAW_LOG(FATAL, msg);
  ExitGroup();
}

void Die::SandboxInfo(const char* msg, const char* file, int line) {
  if (!suppress_info_) {
    logging::LogMessage(file, line, logging::LOG_INFO).stream() << msg;
  }
}

void Die::LogToStderr(const char* msg, const char* file, int line) {
  if (msg) {
    char buf[40];
    snprintf(buf, sizeof(buf), "%d", line);
    std::string s = std::string(file) + ":" + buf + ":" + msg + "\n";

    
    
    ignore_result(
        HANDLE_EINTR(Syscall::Call(__NR_write, 2, s.c_str(), s.length())));
  }
}

bool Die::simple_exit_ = false;
bool Die::suppress_info_ = false;

}  
