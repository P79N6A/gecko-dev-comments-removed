



#include "sandbox/linux/bpf_dsl/policy.h"

#include <errno.h>

#include "sandbox/linux/bpf_dsl/bpf_dsl.h"

namespace sandbox {
namespace bpf_dsl {

ResultExpr Policy::InvalidSyscall() const {
  return Error(ENOSYS);
}

}  
}  
