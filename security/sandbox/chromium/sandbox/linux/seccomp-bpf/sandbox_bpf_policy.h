



#ifndef SANDBOX_LINUX_SECCOMP_BPF_SANDBOX_BPF_POLICY_H_
#define SANDBOX_LINUX_SECCOMP_BPF_SANDBOX_BPF_POLICY_H_

#include "base/basictypes.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {

class ErrorCode;
class SandboxBPF;


class SANDBOX_EXPORT SandboxBPFPolicy {
 public:
  SandboxBPFPolicy() {}
  virtual ~SandboxBPFPolicy() {}

  
  
  
  
  
  
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox_compiler,
                                    int system_call_number) const = 0;

  
  
  virtual ErrorCode InvalidSyscall(SandboxBPF* sandbox_compiler) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(SandboxBPFPolicy);
};

}  

#endif  
