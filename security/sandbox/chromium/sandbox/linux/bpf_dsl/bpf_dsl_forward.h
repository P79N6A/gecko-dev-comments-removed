



#ifndef SANDBOX_LINUX_BPF_DSL_BPF_DSL_FORWARD_H_
#define SANDBOX_LINUX_BPF_DSL_BPF_DSL_FORWARD_H_

#include "base/memory/ref_counted.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {
namespace bpf_dsl {





namespace internal {
class ResultExprImpl;
class BoolExprImpl;
}

typedef scoped_refptr<const internal::ResultExprImpl> ResultExpr;
typedef scoped_refptr<const internal::BoolExprImpl> BoolExpr;

template <typename T>
class Arg;

class Elser;

template <typename T>
class Caser;

}  
}  

extern template class SANDBOX_EXPORT
    scoped_refptr<const sandbox::bpf_dsl::internal::BoolExprImpl>;
extern template class SANDBOX_EXPORT
    scoped_refptr<const sandbox::bpf_dsl::internal::ResultExprImpl>;

#endif  
