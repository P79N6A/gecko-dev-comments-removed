



#ifndef SANDBOX_LINUX_SECCOMP_BPF_BPF_TESTS_H__
#define SANDBOX_LINUX_SECCOMP_BPF_BPF_TESTS_H__

#include "base/logging.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "sandbox/linux/seccomp-bpf/bpf_tester_compatibility_delegate.h"
#include "sandbox/linux/tests/unit_tests.h"

namespace sandbox {











#define BPF_TEST_C(test_case_name, test_name, bpf_policy_class_name)     \
  BPF_DEATH_TEST_C(                                                      \
      test_case_name, test_name, DEATH_SUCCESS(), bpf_policy_class_name)


#define BPF_DEATH_TEST_C(                                            \
    test_case_name, test_name, death, bpf_policy_class_name)         \
  void BPF_TEST_C_##test_name();                                     \
  TEST(test_case_name, DISABLE_ON_TSAN(test_name)) {                 \
    sandbox::SandboxBPFTestRunner bpf_test_runner(                   \
        new sandbox::BPFTesterSimpleDelegate<bpf_policy_class_name>( \
            BPF_TEST_C_##test_name));                                \
    sandbox::UnitTests::RunTestInProcess(&bpf_test_runner, death);   \
  }                                                                  \
  void BPF_TEST_C_##test_name()





#define BPF_TEST_D(test_case_name, test_name, bpf_tester_delegate_class)     \
  BPF_DEATH_TEST_D(                                                          \
      test_case_name, test_name, DEATH_SUCCESS(), bpf_tester_delegate_class)


#define BPF_DEATH_TEST_D(                                          \
    test_case_name, test_name, death, bpf_tester_delegate_class)   \
  TEST(test_case_name, DISABLE_ON_TSAN(test_name)) {               \
    sandbox::SandboxBPFTestRunner bpf_test_runner(                 \
        new bpf_tester_delegate_class());                          \
    sandbox::UnitTests::RunTestInProcess(&bpf_test_runner, death); \
  }


#define BPF_ASSERT SANDBOX_ASSERT
#define BPF_ASSERT_EQ(x, y) BPF_ASSERT((x) == (y))
#define BPF_ASSERT_NE(x, y) BPF_ASSERT((x) != (y))
#define BPF_ASSERT_LT(x, y) BPF_ASSERT((x) < (y))
#define BPF_ASSERT_GT(x, y) BPF_ASSERT((x) > (y))
#define BPF_ASSERT_LE(x, y) BPF_ASSERT((x) <= (y))
#define BPF_ASSERT_GE(x, y) BPF_ASSERT((x) >= (y))













#define BPF_TEST(test_case_name, test_name, policy, aux) \
  BPF_DEATH_TEST(test_case_name, test_name, DEATH_SUCCESS(), policy, aux)




#define BPF_DEATH_TEST(test_case_name, test_name, death, policy, aux) \
  void BPF_TEST_##test_name(aux* BPF_AUX);                            \
  TEST(test_case_name, DISABLE_ON_TSAN(test_name)) {                  \
    sandbox::SandboxBPFTestRunner bpf_test_runner(                    \
        new sandbox::BPFTesterCompatibilityDelegate<policy, aux>(     \
            BPF_TEST_##test_name));                                   \
    sandbox::UnitTests::RunTestInProcess(&bpf_test_runner, death);    \
  }                                                                   \
  void BPF_TEST_##test_name(aux* BPF_AUX)





template <class PolicyClass>
class BPFTesterSimpleDelegate : public BPFTesterDelegate {
 public:
  explicit BPFTesterSimpleDelegate(void (*test_function)(void))
      : test_function_(test_function) {}
  virtual ~BPFTesterSimpleDelegate() {}

  virtual scoped_ptr<bpf_dsl::Policy> GetSandboxBPFPolicy() override {
    return scoped_ptr<bpf_dsl::Policy>(new PolicyClass());
  }
  virtual void RunTestFunction() override {
    DCHECK(test_function_);
    test_function_();
  }

 private:
  void (*test_function_)(void);
  DISALLOW_COPY_AND_ASSIGN(BPFTesterSimpleDelegate);
};

}  

#endif  
