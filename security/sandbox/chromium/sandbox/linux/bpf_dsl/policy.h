



#ifndef SANDBOX_LINUX_BPF_DSL_POLICY_H_
#define SANDBOX_LINUX_BPF_DSL_POLICY_H_

#include "base/macros.h"
#include "sandbox/linux/bpf_dsl/bpf_dsl_forward.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {
namespace bpf_dsl {


class SANDBOX_EXPORT Policy {
 public:
  Policy() {}
  virtual ~Policy() {}

  
  
  
  virtual ResultExpr EvaluateSyscall(int sysno) const = 0;

  
  
  virtual ResultExpr InvalidSyscall() const;

 private:
  DISALLOW_COPY_AND_ASSIGN(Policy);
};

}  
}  

#endif  
