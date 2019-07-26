



#ifndef SANDBOX_LINUX_SECCOMP_BPF_VERIFIER_H__
#define SANDBOX_LINUX_SECCOMP_BPF_VERIFIER_H__

#include <linux/filter.h>

#include <utility>
#include <vector>

namespace sandbox {

class SandboxBPFPolicy;

class Verifier {
 public:
  
  
  
  
  
  
  
  static bool VerifyBPF(SandboxBPF* sandbox,
                        const std::vector<struct sock_filter>& program,
                        const SandboxBPFPolicy& policy,
                        const char** err);

  
  
  
  
  
  
  
  
  static uint32_t EvaluateBPF(const std::vector<struct sock_filter>& program,
                              const struct arch_seccomp_data& data,
                              const char** err);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Verifier);
};

}  

#endif  
