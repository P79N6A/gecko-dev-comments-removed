



#ifndef SANDBOX_LINUX_SECCOMP_BPF_SANDBOX_BPF_TEST_RUNNER_H_
#define SANDBOX_LINUX_SECCOMP_BPF_SANDBOX_BPF_TEST_RUNNER_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "sandbox/linux/tests/sandbox_test_runner.h"

namespace sandbox {
namespace bpf_dsl {
class Policy;
}





class BPFTesterDelegate {
 public:
  BPFTesterDelegate() {}
  virtual ~BPFTesterDelegate() {}

  
  
  
  virtual scoped_ptr<bpf_dsl::Policy> GetSandboxBPFPolicy() = 0;
  
  virtual void RunTestFunction() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(BPFTesterDelegate);
};







class SandboxBPFTestRunner : public SandboxTestRunner {
 public:
  
  
  explicit SandboxBPFTestRunner(BPFTesterDelegate* bpf_tester_delegate);
  virtual ~SandboxBPFTestRunner();

  virtual void Run() override;

  virtual bool ShouldCheckForLeaks() const override;

 private:
  scoped_ptr<BPFTesterDelegate> bpf_tester_delegate_;
  DISALLOW_COPY_AND_ASSIGN(SandboxBPFTestRunner);
};

}  

#endif  
