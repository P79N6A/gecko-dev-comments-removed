



#ifndef SANDBOX_LINUX_BPF_DSL_POLICY_COMPILER_H_
#define SANDBOX_LINUX_BPF_DSL_POLICY_COMPILER_H_

#include <stdint.h>

#include <map>
#include <set>
#include <vector>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "sandbox/linux/seccomp-bpf/codegen.h"
#include "sandbox/linux/seccomp-bpf/errorcode.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {
struct Instruction;

namespace bpf_dsl {
class Policy;




class SANDBOX_EXPORT PolicyCompiler {
 public:
  PolicyCompiler(const Policy* policy, TrapRegistry* registry);
  ~PolicyCompiler();

  
  
  scoped_ptr<CodeGen::Program> Compile();

  
  
  ErrorCode Error(int err);

  
  
  
  
  
  
  ErrorCode Trap(TrapRegistry::TrapFnc fnc, const void* aux);

  
  
  
  
  
  
  
  
  
  
  ErrorCode UnsafeTrap(TrapRegistry::TrapFnc fnc, const void* aux);

  
  
  static bool IsRequiredForUnsafeTrap(int sysno);

  
  
  
  
  
  
  
  
  
  ErrorCode CondMaskedEqual(int argno,
                            ErrorCode::ArgType is_32bit,
                            uint64_t mask,
                            uint64_t value,
                            const ErrorCode& passed,
                            const ErrorCode& failed);

  
  ErrorCode Kill(const char* msg);

  
  
  
  ErrorCode Unexpected64bitArgument();

 private:
  struct Range;
  typedef std::vector<Range> Ranges;
  typedef std::map<uint32_t, ErrorCode> ErrMap;
  typedef std::set<ErrorCode, struct ErrorCode::LessThan> Conds;

  
  
  enum ArgHalf {
    LowerHalf,
    UpperHalf,
  };

  
  Instruction* AssemblePolicy();

  
  
  
  Instruction* CheckArch(Instruction* passed);

  
  
  
  Instruction* MaybeAddEscapeHatch(Instruction* rest);

  
  
  
  
  Instruction* DispatchSyscall();

  
  
  
  Instruction* CheckSyscallNumber(Instruction* passed);

  
  
  
  
  void FindRanges(Ranges* ranges);

  
  
  Instruction* AssembleJumpTable(Ranges::const_iterator start,
                                 Ranges::const_iterator stop);

  
  
  
  
  
  Instruction* RetExpression(const ErrorCode& err);

  
  
  
  
  Instruction* CondExpression(const ErrorCode& cond);

  
  
  Instruction* CondExpressionHalf(const ErrorCode& cond,
                                  ArgHalf half,
                                  Instruction* passed,
                                  Instruction* failed);

  
  ErrorCode MakeTrap(TrapRegistry::TrapFnc fnc, const void* aux, bool safe);

  const Policy* policy_;
  TrapRegistry* registry_;

  Conds conds_;
  CodeGen gen_;
  bool has_unsafe_traps_;

  DISALLOW_COPY_AND_ASSIGN(PolicyCompiler);
};

}  
}  

#endif  
