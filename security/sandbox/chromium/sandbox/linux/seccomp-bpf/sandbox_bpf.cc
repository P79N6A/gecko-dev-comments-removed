



#include "sandbox/linux/seccomp-bpf/sandbox_bpf.h"



#if defined(ANDROID)
#include <sys/cdefs.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <linux/filter.h>
#include <signal.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/posix/eintr_wrapper.h"
#include "sandbox/linux/bpf_dsl/bpf_dsl.h"
#include "sandbox/linux/bpf_dsl/dump_bpf.h"
#include "sandbox/linux/bpf_dsl/policy.h"
#include "sandbox/linux/bpf_dsl/policy_compiler.h"
#include "sandbox/linux/seccomp-bpf/codegen.h"
#include "sandbox/linux/seccomp-bpf/die.h"
#include "sandbox/linux/seccomp-bpf/errorcode.h"
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"
#include "sandbox/linux/seccomp-bpf/syscall.h"
#include "sandbox/linux/seccomp-bpf/syscall_iterator.h"
#include "sandbox/linux/seccomp-bpf/trap.h"
#include "sandbox/linux/seccomp-bpf/verifier.h"
#include "sandbox/linux/services/linux_syscalls.h"

using sandbox::bpf_dsl::Allow;
using sandbox::bpf_dsl::Error;
using sandbox::bpf_dsl::ResultExpr;

namespace sandbox {

namespace {

const int kExpectedExitCode = 100;

#if !defined(NDEBUG)
void WriteFailedStderrSetupMessage(int out_fd) {
  const char* error_string = strerror(errno);
  static const char msg[] =
      "You have reproduced a puzzling issue.\n"
      "Please, report to crbug.com/152530!\n"
      "Failed to set up stderr: ";
  if (HANDLE_EINTR(write(out_fd, msg, sizeof(msg) - 1)) > 0 && error_string &&
      HANDLE_EINTR(write(out_fd, error_string, strlen(error_string))) > 0 &&
      HANDLE_EINTR(write(out_fd, "\n", 1))) {
  }
}
#endif  



class ProbePolicy : public bpf_dsl::Policy {
 public:
  ProbePolicy() {}
  virtual ~ProbePolicy() {}

  virtual ResultExpr EvaluateSyscall(int sysnum) const override {
    switch (sysnum) {
      case __NR_getpid:
        
        return Error(EPERM);
      case __NR_exit_group:
        
        return Allow();
      default:
        
        return Error(EINVAL);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ProbePolicy);
};

void ProbeProcess(void) {
  if (syscall(__NR_getpid) < 0 && errno == EPERM) {
    syscall(__NR_exit_group, static_cast<intptr_t>(kExpectedExitCode));
  }
}

class AllowAllPolicy : public bpf_dsl::Policy {
 public:
  AllowAllPolicy() {}
  virtual ~AllowAllPolicy() {}

  virtual ResultExpr EvaluateSyscall(int sysnum) const override {
    DCHECK(SandboxBPF::IsValidSyscallNumber(sysnum));
    return Allow();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AllowAllPolicy);
};

void TryVsyscallProcess(void) {
  time_t current_time;
  
  
  
  if (time(&current_time) != static_cast<time_t>(-1)) {
    syscall(__NR_exit_group, static_cast<intptr_t>(kExpectedExitCode));
  }
}

bool IsSingleThreaded(int proc_fd) {
  if (proc_fd < 0) {
    
    
    return true;
  }

  struct stat sb;
  int task = -1;
  if ((task = openat(proc_fd, "self/task", O_RDONLY | O_DIRECTORY)) < 0 ||
      fstat(task, &sb) != 0 || sb.st_nlink != 3 || IGNORE_EINTR(close(task))) {
    if (task >= 0) {
      if (IGNORE_EINTR(close(task))) {
      }
    }
    return false;
  }
  return true;
}

}  

SandboxBPF::SandboxBPF()
    : quiet_(false), proc_fd_(-1), sandbox_has_started_(false), policy_() {
}

SandboxBPF::~SandboxBPF() {
}

bool SandboxBPF::IsValidSyscallNumber(int sysnum) {
  return SyscallSet::IsValid(sysnum);
}

bool SandboxBPF::RunFunctionInPolicy(void (*code_in_sandbox)(),
                                     scoped_ptr<bpf_dsl::Policy> policy) {
  
  
  sigset_t old_mask, new_mask;
  if (sigfillset(&new_mask) || sigprocmask(SIG_BLOCK, &new_mask, &old_mask)) {
    SANDBOX_DIE("sigprocmask() failed");
  }
  int fds[2];
  if (pipe2(fds, O_NONBLOCK | O_CLOEXEC)) {
    SANDBOX_DIE("pipe() failed");
  }

  if (fds[0] <= 2 || fds[1] <= 2) {
    SANDBOX_DIE("Process started without standard file descriptors");
  }

  
  
  
  DCHECK(IsSingleThreaded(proc_fd_));
  pid_t pid = fork();
  if (pid < 0) {
    
    
    
    
    
    
    sigprocmask(SIG_SETMASK, &old_mask, NULL);  
    SANDBOX_DIE("fork() failed unexpectedly");
  }

  
  if (!pid) {
    
    
    Die::EnableSimpleExit();

    errno = 0;
    if (IGNORE_EINTR(close(fds[0]))) {
      
      
#if !defined(NDEBUG)
      WriteFailedStderrSetupMessage(fds[1]);
      SANDBOX_DIE(NULL);
#endif
    }
    if (HANDLE_EINTR(dup2(fds[1], 2)) != 2) {
      
      
      
      
      
      
      
#if !defined(NDEBUG)
      
      WriteFailedStderrSetupMessage(fds[1]);
      SANDBOX_DIE(NULL);
#endif
    }
    if (IGNORE_EINTR(close(fds[1]))) {
      
      
#if !defined(NDEBUG)
      WriteFailedStderrSetupMessage(fds[1]);
      SANDBOX_DIE(NULL);
#endif
    }

    SetSandboxPolicy(policy.release());
    if (!StartSandbox(PROCESS_SINGLE_THREADED)) {
      SANDBOX_DIE(NULL);
    }

    
    code_in_sandbox();

    
    SANDBOX_DIE(NULL);
  }

  
  if (IGNORE_EINTR(close(fds[1]))) {
    SANDBOX_DIE("close() failed");
  }
  if (sigprocmask(SIG_SETMASK, &old_mask, NULL)) {
    SANDBOX_DIE("sigprocmask() failed");
  }
  int status;
  if (HANDLE_EINTR(waitpid(pid, &status, 0)) != pid) {
    SANDBOX_DIE("waitpid() failed unexpectedly");
  }
  bool rc = WIFEXITED(status) && WEXITSTATUS(status) == kExpectedExitCode;

  
  
  
  
  
  if (!rc) {
    char buf[4096];
    ssize_t len = HANDLE_EINTR(read(fds[0], buf, sizeof(buf) - 1));
    if (len > 0) {
      while (len > 1 && buf[len - 1] == '\n') {
        --len;
      }
      buf[len] = '\000';
      SANDBOX_DIE(buf);
    }
  }
  if (IGNORE_EINTR(close(fds[0]))) {
    SANDBOX_DIE("close() failed");
  }

  return rc;
}

bool SandboxBPF::KernelSupportSeccompBPF() {
  return RunFunctionInPolicy(ProbeProcess,
                             scoped_ptr<bpf_dsl::Policy>(new ProbePolicy())) &&
         RunFunctionInPolicy(TryVsyscallProcess,
                             scoped_ptr<bpf_dsl::Policy>(new AllowAllPolicy()));
}


SandboxBPF::SandboxStatus SandboxBPF::SupportsSeccompSandbox(int proc_fd) {
  
  
  if (status_ == STATUS_ENABLED) {
    return status_;
  }

  
  
  if (status_ == STATUS_AVAILABLE) {
    if (!IsSingleThreaded(proc_fd)) {
      status_ = STATUS_UNAVAILABLE;
    }
    return status_;
  }

  if (status_ == STATUS_UNAVAILABLE && IsSingleThreaded(proc_fd)) {
    
    
    
    
    
    
    
    status_ = STATUS_AVAILABLE;
    return status_;
  }

  
  
  
  if (status_ == STATUS_UNKNOWN) {
    
    
    
    SandboxBPF sandbox;

    
    
    sandbox.quiet_ = true;
    sandbox.set_proc_fd(proc_fd);
    status_ = sandbox.KernelSupportSeccompBPF() ? STATUS_AVAILABLE
                                                : STATUS_UNSUPPORTED;

    
    
    
    
    if (status_ == STATUS_AVAILABLE && !IsSingleThreaded(proc_fd)) {
      status_ = STATUS_UNAVAILABLE;
    }
  }
  return status_;
}


SandboxBPF::SandboxStatus
SandboxBPF::SupportsSeccompThreadFilterSynchronization() {
  
  
  const int rv = syscall(
      __NR_seccomp, SECCOMP_SET_MODE_FILTER, SECCOMP_FILTER_FLAG_TSYNC, NULL);

  if (rv == -1 && errno == EFAULT) {
    return STATUS_AVAILABLE;
  } else {
    
    CHECK_EQ(-1, rv);
    CHECK(ENOSYS == errno || EINVAL == errno);
    return STATUS_UNSUPPORTED;
  }
}

void SandboxBPF::set_proc_fd(int proc_fd) { proc_fd_ = proc_fd; }

bool SandboxBPF::StartSandbox(SandboxThreadState thread_state) {
  CHECK(thread_state == PROCESS_SINGLE_THREADED ||
        thread_state == PROCESS_MULTI_THREADED);

  if (status_ == STATUS_UNSUPPORTED || status_ == STATUS_UNAVAILABLE) {
    SANDBOX_DIE(
        "Trying to start sandbox, even though it is known to be "
        "unavailable");
    return false;
  } else if (sandbox_has_started_) {
    SANDBOX_DIE(
        "Cannot repeatedly start sandbox. Create a separate Sandbox "
        "object instead.");
    return false;
  }
  if (proc_fd_ < 0) {
    proc_fd_ = open("/proc", O_RDONLY | O_DIRECTORY);
  }
  if (proc_fd_ < 0) {
    
    
  }

  bool supports_tsync =
      SupportsSeccompThreadFilterSynchronization() == STATUS_AVAILABLE;

  if (thread_state == PROCESS_SINGLE_THREADED) {
    if (!IsSingleThreaded(proc_fd_)) {
      SANDBOX_DIE("Cannot start sandbox; process is already multi-threaded");
      return false;
    }
  } else if (thread_state == PROCESS_MULTI_THREADED) {
    if (IsSingleThreaded(proc_fd_)) {
      SANDBOX_DIE("Cannot start sandbox; "
                  "process may be single-threaded when reported as not");
      return false;
    }
    if (!supports_tsync) {
      SANDBOX_DIE("Cannot start sandbox; kernel does not support synchronizing "
                  "filters for a threadgroup");
      return false;
    }
  }

  
  
  
  if (proc_fd_ >= 0) {
    if (IGNORE_EINTR(close(proc_fd_))) {
      SANDBOX_DIE("Failed to close file descriptor for /proc");
      return false;
    }
    proc_fd_ = -1;
  }

  
  InstallFilter(supports_tsync || thread_state == PROCESS_MULTI_THREADED);

  
  status_ = STATUS_ENABLED;

  return true;
}


void SandboxBPF::SetSandboxPolicy(bpf_dsl::Policy* policy) {
  DCHECK(!policy_);
  if (sandbox_has_started_) {
    SANDBOX_DIE("Cannot change policy after sandbox has started");
  }
  policy_.reset(policy);
}

void SandboxBPF::InstallFilter(bool must_sync_threads) {
  
  
  
  
  
  
  
  
  
  
  
  CodeGen::Program* program = AssembleFilter(false).release();

  struct sock_filter bpf[program->size()];
  const struct sock_fprog prog = {static_cast<unsigned short>(program->size()),
                                  bpf};
  memcpy(bpf, &(*program)[0], sizeof(bpf));
  delete program;

  
  
  
  policy_.reset();

  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
    SANDBOX_DIE(quiet_ ? NULL : "Kernel refuses to enable no-new-privs");
  }

  
  
  
  if (must_sync_threads) {
    int rv = syscall(__NR_seccomp, SECCOMP_SET_MODE_FILTER,
        SECCOMP_FILTER_FLAG_TSYNC, reinterpret_cast<const char*>(&prog));
    if (rv) {
      SANDBOX_DIE(quiet_ ? NULL :
          "Kernel refuses to turn on and synchronize threads for BPF filters");
    }
  } else {
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog)) {
      SANDBOX_DIE(quiet_ ? NULL : "Kernel refuses to turn on BPF filters");
    }
  }

  sandbox_has_started_ = true;
}

scoped_ptr<CodeGen::Program> SandboxBPF::AssembleFilter(
    bool force_verification) {
#if !defined(NDEBUG)
  force_verification = true;
#endif

  bpf_dsl::PolicyCompiler compiler(policy_.get(), Trap::Registry());
  scoped_ptr<CodeGen::Program> program = compiler.Compile();

  
  
  
  if (force_verification) {
    
    
    

    const char* err = NULL;
    if (!Verifier::VerifyBPF(&compiler, *program, *policy_, &err)) {
      bpf_dsl::DumpBPF::PrintProgram(*program);
      SANDBOX_DIE(err);
    }
  }

  return program.Pass();
}

bool SandboxBPF::IsRequiredForUnsafeTrap(int sysno) {
  return bpf_dsl::PolicyCompiler::IsRequiredForUnsafeTrap(sysno);
}

intptr_t SandboxBPF::ForwardSyscall(const struct arch_seccomp_data& args) {
  return Syscall::Call(args.nr,
                       static_cast<intptr_t>(args.args[0]),
                       static_cast<intptr_t>(args.args[1]),
                       static_cast<intptr_t>(args.args[2]),
                       static_cast<intptr_t>(args.args[3]),
                       static_cast<intptr_t>(args.args[4]),
                       static_cast<intptr_t>(args.args[5]));
}

SandboxBPF::SandboxStatus SandboxBPF::status_ = STATUS_UNKNOWN;

}  
