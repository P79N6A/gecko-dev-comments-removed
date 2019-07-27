



#ifndef SANDBOX_LINUX_SECCOMP_BPF_SANDBOX_BPF_H__
#define SANDBOX_LINUX_SECCOMP_BPF_SANDBOX_BPF_H__

#include <stdint.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "sandbox/linux/seccomp-bpf/codegen.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {
struct arch_seccomp_data;
namespace bpf_dsl {
class Policy;
}

class SANDBOX_EXPORT SandboxBPF {
 public:
  enum SandboxStatus {
    STATUS_UNKNOWN,      
    STATUS_UNSUPPORTED,  
    STATUS_UNAVAILABLE,  
    STATUS_AVAILABLE,    
    STATUS_ENABLED       
  };

  
  
  
  
  enum SandboxThreadState {
    PROCESS_INVALID,
    PROCESS_SINGLE_THREADED,  
    
    
    PROCESS_MULTI_THREADED,   
  };

  
  
  
  
  
  
  
  
  
  
  SandboxBPF();
  ~SandboxBPF();

  
  
  
  static bool IsValidSyscallNumber(int sysnum);

  
  
  
  
  
  static SandboxStatus SupportsSeccompSandbox(int proc_fd);

  
  
  static SandboxStatus SupportsSeccompThreadFilterSynchronization();

  
  
  
  
  
  void set_proc_fd(int proc_fd);

  
  
  void SetSandboxPolicy(bpf_dsl::Policy* policy);

  
  
  static bool IsRequiredForUnsafeTrap(int sysno);

  
  
  
  
  
  
  
  
  static intptr_t ForwardSyscall(const struct arch_seccomp_data& args);

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool StartSandbox(SandboxThreadState thread_state) WARN_UNUSED_RESULT;

  
  
  
  
  
  
  
  
  scoped_ptr<CodeGen::Program> AssembleFilter(bool force_verification);

 private:
  
  int proc_fd() { return proc_fd_; }

  
  
  
  bool RunFunctionInPolicy(void (*code_in_sandbox)(),
                           scoped_ptr<bpf_dsl::Policy> policy);

  
  
  
  
  bool KernelSupportSeccompBPF();

  
  
  void InstallFilter(bool must_sync_threads);

  
  
  
  void VerifyProgram(const CodeGen::Program& program);

  static SandboxStatus status_;

  bool quiet_;
  int proc_fd_;
  bool sandbox_has_started_;
  scoped_ptr<bpf_dsl::Policy> policy_;

  DISALLOW_COPY_AND_ASSIGN(SandboxBPF);
};

}  

#endif
