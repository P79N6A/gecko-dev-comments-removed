



#ifndef SANDBOX_LINUX_BPF_DSL_BPF_DSL_IMPL_H_
#define SANDBOX_LINUX_BPF_DSL_BPF_DSL_IMPL_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {
class ErrorCode;

namespace bpf_dsl {
class PolicyCompiler;

namespace internal {


class BoolExprImpl : public base::RefCounted<BoolExprImpl> {
 public:
  
  
  
  virtual ErrorCode Compile(PolicyCompiler* pc,
                            ErrorCode true_ec,
                            ErrorCode false_ec) const = 0;

 protected:
  BoolExprImpl() {}
  virtual ~BoolExprImpl() {}

 private:
  friend class base::RefCounted<BoolExprImpl>;
  DISALLOW_COPY_AND_ASSIGN(BoolExprImpl);
};


class ResultExprImpl : public base::RefCounted<ResultExprImpl> {
 public:
  
  
  virtual ErrorCode Compile(PolicyCompiler* pc) const = 0;

  
  
  virtual bool HasUnsafeTraps() const;

 protected:
  ResultExprImpl() {}
  virtual ~ResultExprImpl() {}

 private:
  friend class base::RefCounted<ResultExprImpl>;
  DISALLOW_COPY_AND_ASSIGN(ResultExprImpl);
};

}  
}  
}  

#endif  
