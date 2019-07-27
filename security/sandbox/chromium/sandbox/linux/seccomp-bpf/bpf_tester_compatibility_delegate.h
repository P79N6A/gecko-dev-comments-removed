



#ifndef SANDBOX_LINUX_SECCOMP_BPF_BPF_TESTER_COMPATIBILITY_DELEGATE_H_
#define SANDBOX_LINUX_SECCOMP_BPF_BPF_TESTER_COMPATIBILITY_DELEGATE_H_

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "base/memory/scoped_ptr.h"
#include "sandbox/linux/seccomp-bpf/sandbox_bpf_compatibility_policy.h"
#include "sandbox/linux/seccomp-bpf/sandbox_bpf_test_runner.h"
#include "sandbox/linux/tests/sandbox_test_runner.h"
#include "sandbox/linux/tests/unit_tests.h"

namespace sandbox {








template <class Aux>
class BPFTesterCompatibilityDelegate : public BPFTesterDelegate {
 public:
  typedef Aux AuxType;
  BPFTesterCompatibilityDelegate(
      void (*test_function)(AuxType*),
      typename CompatibilityPolicy<AuxType>::SyscallEvaluator policy_function)
      : aux_(),
        test_function_(test_function),
        policy_function_(policy_function) {}

  virtual ~BPFTesterCompatibilityDelegate() {}

  virtual scoped_ptr<SandboxBPFPolicy> GetSandboxBPFPolicy() OVERRIDE {
    
    
    
    
    return scoped_ptr<SandboxBPFPolicy>(
        new CompatibilityPolicy<AuxType>(policy_function_, &aux_));
  }

  virtual void RunTestFunction() OVERRIDE {
    
    
    
    test_function_(&aux_);
  }

 private:
  AuxType aux_;
  void (*test_function_)(AuxType*);
  typename CompatibilityPolicy<AuxType>::SyscallEvaluator policy_function_;
  DISALLOW_COPY_AND_ASSIGN(BPFTesterCompatibilityDelegate);
};

}  

#endif  
