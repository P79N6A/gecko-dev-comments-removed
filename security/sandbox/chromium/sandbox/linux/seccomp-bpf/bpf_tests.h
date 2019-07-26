



#ifndef SANDBOX_LINUX_SECCOMP_BPF_BPF_TESTS_H__
#define SANDBOX_LINUX_SECCOMP_BPF_BPF_TESTS_H__

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "build/build_config.h"
#include "sandbox/linux/tests/unit_tests.h"
#include "sandbox/linux/seccomp-bpf/sandbox_bpf.h"

namespace sandbox {






#define BPF_DEATH_TEST(test_case_name, test_name, death, policy, aux...) \
  void BPF_TEST_##test_name(sandbox::BPFTests<aux>::AuxType& BPF_AUX);   \
  TEST(test_case_name, DISABLE_ON_TSAN(test_name)) {                     \
    sandbox::BPFTests<aux>::TestArgs arg(BPF_TEST_##test_name, policy);  \
    sandbox::BPFTests<aux>::RunTestInProcess(                            \
        sandbox::BPFTests<aux>::TestWrapper, &arg, death);               \
  }                                                                      \
  void BPF_TEST_##test_name(sandbox::BPFTests<aux>::AuxType& BPF_AUX)











#define BPF_TEST(test_case_name, test_name, policy, aux...) \
  BPF_DEATH_TEST(test_case_name, test_name, DEATH_SUCCESS(), policy, aux)


#define BPF_ASSERT SANDBOX_ASSERT




template <class Aux = int[0]>
class BPFTests : public UnitTests {
 public:
  typedef Aux AuxType;

  class TestArgs {
   public:
    TestArgs(void (*t)(AuxType&), sandbox::SandboxBPF::EvaluateSyscall p)
        : test_(t), policy_(p), aux_() {}

    void (*test() const)(AuxType&) { return test_; }
    sandbox::SandboxBPF::EvaluateSyscall policy() const { return policy_; }

   private:
    friend class BPFTests;

    void (*test_)(AuxType&);
    sandbox::SandboxBPF::EvaluateSyscall policy_;
    AuxType aux_;
  };

  static void TestWrapper(void* void_arg) {
    TestArgs* arg = reinterpret_cast<TestArgs*>(void_arg);
    sandbox::Die::EnableSimpleExit();
    if (sandbox::SandboxBPF::SupportsSeccompSandbox(-1) ==
        sandbox::SandboxBPF::STATUS_AVAILABLE) {
      
      int proc_fd;
      BPF_ASSERT((proc_fd = open("/proc", O_RDONLY | O_DIRECTORY)) >= 0);
      BPF_ASSERT(sandbox::SandboxBPF::SupportsSeccompSandbox(proc_fd) ==
                 sandbox::SandboxBPF::STATUS_AVAILABLE);

      
      sandbox::SandboxBPF sandbox;
      sandbox.set_proc_fd(proc_fd);
      sandbox.SetSandboxPolicyDeprecated(arg->policy(), &arg->aux_);
      BPF_ASSERT(sandbox.StartSandbox(
          sandbox::SandboxBPF::PROCESS_SINGLE_THREADED));

      arg->test()(arg->aux_);
    } else {
      printf("This BPF test is not fully running in this configuration!\n");
      
      
      if (!IsAndroid() && !IsRunningOnValgrind()) {
        const bool seccomp_bpf_is_supported = false;
        BPF_ASSERT(seccomp_bpf_is_supported);
      }
      
      
      sandbox::SandboxBPF sandbox;
      sandbox.SetSandboxPolicyDeprecated(arg->policy(), &arg->aux_);
      sandbox::SandboxBPF::Program* program =
          sandbox.AssembleFilter(true );
      delete program;
      sandbox::UnitTests::IgnoreThisTest();
    }
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BPFTests);
};

}  

#endif  
