



#ifndef SANDBOX_LINUX_SECCOMP_BPF_BPF_TESTER_COMPATIBILITY_DELEGATE_H_
#define SANDBOX_LINUX_SECCOMP_BPF_BPF_TESTER_COMPATIBILITY_DELEGATE_H_

#include "base/memory/scoped_ptr.h"
#include "sandbox/linux/seccomp-bpf/sandbox_bpf_test_runner.h"

namespace sandbox {








template <class Policy, class Aux>
class BPFTesterCompatibilityDelegate : public BPFTesterDelegate {
 public:
  typedef void (*TestFunction)(Aux*);

  explicit BPFTesterCompatibilityDelegate(TestFunction test_function)
      : aux_(), test_function_(test_function) {}

  virtual ~BPFTesterCompatibilityDelegate() {}

  virtual scoped_ptr<bpf_dsl::Policy> GetSandboxBPFPolicy() override {
    
    
    
    
    return scoped_ptr<bpf_dsl::Policy>(new Policy(&aux_));
  }

  virtual void RunTestFunction() override {
    
    
    
    test_function_(&aux_);
  }

 private:
  Aux aux_;
  TestFunction test_function_;

  DISALLOW_COPY_AND_ASSIGN(BPFTesterCompatibilityDelegate);
};

}  

#endif  
