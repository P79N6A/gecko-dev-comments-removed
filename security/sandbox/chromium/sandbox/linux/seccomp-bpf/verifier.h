



#ifndef SANDBOX_LINUX_SECCOMP_BPF_VERIFIER_H__
#define SANDBOX_LINUX_SECCOMP_BPF_VERIFIER_H__

#include <stdint.h>

#include <vector>

#include "base/macros.h"

struct sock_filter;

namespace sandbox {
struct arch_seccomp_data;
namespace bpf_dsl {
class Policy;
class PolicyCompiler;
}

class Verifier {
 public:
  
  
  
  
  
  
  
  static bool VerifyBPF(bpf_dsl::PolicyCompiler* compiler,
                        const std::vector<struct sock_filter>& program,
                        const bpf_dsl::Policy& policy,
                        const char** err);

  
  
  
  
  
  
  
  
  static uint32_t EvaluateBPF(const std::vector<struct sock_filter>& program,
                              const struct arch_seccomp_data& data,
                              const char** err);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Verifier);
};

}  

#endif  
