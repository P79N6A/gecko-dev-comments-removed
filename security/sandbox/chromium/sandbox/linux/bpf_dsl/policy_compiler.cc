



#include "sandbox/linux/bpf_dsl/policy_compiler.h"

#include <errno.h>
#include <linux/filter.h>
#include <sys/syscall.h>

#include <limits>

#include "base/logging.h"
#include "base/macros.h"
#include "sandbox/linux/bpf_dsl/bpf_dsl.h"
#include "sandbox/linux/bpf_dsl/bpf_dsl_impl.h"
#include "sandbox/linux/bpf_dsl/policy.h"
#include "sandbox/linux/seccomp-bpf/codegen.h"
#include "sandbox/linux/seccomp-bpf/die.h"
#include "sandbox/linux/seccomp-bpf/errorcode.h"
#include "sandbox/linux/seccomp-bpf/instruction.h"
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"
#include "sandbox/linux/seccomp-bpf/syscall.h"
#include "sandbox/linux/seccomp-bpf/syscall_iterator.h"

namespace sandbox {
namespace bpf_dsl {

namespace {

#if defined(__i386__) || defined(__x86_64__)
const bool kIsIntel = true;
#else
const bool kIsIntel = false;
#endif
#if defined(__x86_64__) && defined(__ILP32__)
const bool kIsX32 = true;
#else
const bool kIsX32 = false;
#endif

const int kSyscallsRequiredForUnsafeTraps[] = {
    __NR_rt_sigprocmask,
    __NR_rt_sigreturn,
#if defined(__NR_sigprocmask)
    __NR_sigprocmask,
#endif
#if defined(__NR_sigreturn)
    __NR_sigreturn,
#endif
};

bool HasExactlyOneBit(uint64_t x) {
  
  return x != 0 && (x & (x - 1)) == 0;
}

bool IsDenied(const ErrorCode& code) {
  return (code.err() & SECCOMP_RET_ACTION) == SECCOMP_RET_TRAP ||
         (code.err() >= (SECCOMP_RET_ERRNO + ErrorCode::ERR_MIN_ERRNO) &&
          code.err() <= (SECCOMP_RET_ERRNO + ErrorCode::ERR_MAX_ERRNO));
}



intptr_t ReturnErrno(const struct arch_seccomp_data&, void* aux) {
  
  
  
  
  int err = reinterpret_cast<intptr_t>(aux) & SECCOMP_RET_DATA;
  return -err;
}

intptr_t BPFFailure(const struct arch_seccomp_data&, void* aux) {
  SANDBOX_DIE(static_cast<char*>(aux));
}

bool HasUnsafeTraps(const Policy* policy) {
  for (uint32_t sysnum : SyscallSet::ValidOnly()) {
    if (policy->EvaluateSyscall(sysnum)->HasUnsafeTraps()) {
      return true;
    }
  }
  return policy->InvalidSyscall()->HasUnsafeTraps();
}

}  

struct PolicyCompiler::Range {
  Range(uint32_t f, const ErrorCode& e) : from(f), err(e) {}
  uint32_t from;
  ErrorCode err;
};

PolicyCompiler::PolicyCompiler(const Policy* policy, TrapRegistry* registry)
    : policy_(policy),
      registry_(registry),
      conds_(),
      gen_(),
      has_unsafe_traps_(HasUnsafeTraps(policy_)) {
}

PolicyCompiler::~PolicyCompiler() {
}

scoped_ptr<CodeGen::Program> PolicyCompiler::Compile() {
  if (!IsDenied(policy_->InvalidSyscall()->Compile(this))) {
    SANDBOX_DIE("Policies should deny invalid system calls.");
  }

  
  if (has_unsafe_traps_) {
    
    
    
    
    if (Syscall::Call(-1) == -1 && errno == ENOSYS) {
      SANDBOX_DIE(
          "Support for UnsafeTrap() has not yet been ported to this "
          "architecture");
    }

    for (int sysnum : kSyscallsRequiredForUnsafeTraps) {
      if (!policy_->EvaluateSyscall(sysnum)->Compile(this)
               .Equals(ErrorCode(ErrorCode::ERR_ALLOWED))) {
        SANDBOX_DIE(
            "Policies that use UnsafeTrap() must unconditionally allow all "
            "required system calls");
      }
    }

    if (!registry_->EnableUnsafeTraps()) {
      
      
      
      
      
      SANDBOX_DIE("We'd rather die than enable unsafe traps");
    }
  }

  
  scoped_ptr<CodeGen::Program> program(new CodeGen::Program());
  gen_.Compile(AssemblePolicy(), program.get());
  return program.Pass();
}

Instruction* PolicyCompiler::AssemblePolicy() {
  
  
  
  
  
  
  return CheckArch(MaybeAddEscapeHatch(DispatchSyscall()));
}

Instruction* PolicyCompiler::CheckArch(Instruction* passed) {
  
  
  return gen_.MakeInstruction(
      BPF_LD + BPF_W + BPF_ABS,
      SECCOMP_ARCH_IDX,
      gen_.MakeInstruction(
          BPF_JMP + BPF_JEQ + BPF_K,
          SECCOMP_ARCH,
          passed,
          RetExpression(Kill("Invalid audit architecture in BPF filter"))));
}

Instruction* PolicyCompiler::MaybeAddEscapeHatch(Instruction* rest) {
  
  if (!has_unsafe_traps_) {
    return rest;
  }

  
  
  uint64_t syscall_entry_point =
      static_cast<uint64_t>(static_cast<uintptr_t>(Syscall::Call(-1)));
  uint32_t low = static_cast<uint32_t>(syscall_entry_point);
  uint32_t hi = static_cast<uint32_t>(syscall_entry_point >> 32);

  
  
  
  
  
  
  
  return gen_.MakeInstruction(
      BPF_LD + BPF_W + BPF_ABS,
      SECCOMP_IP_LSB_IDX,
      gen_.MakeInstruction(
          BPF_JMP + BPF_JEQ + BPF_K,
          low,
          gen_.MakeInstruction(
              BPF_LD + BPF_W + BPF_ABS,
              SECCOMP_IP_MSB_IDX,
              gen_.MakeInstruction(
                  BPF_JMP + BPF_JEQ + BPF_K,
                  hi,
                  RetExpression(ErrorCode(ErrorCode::ERR_ALLOWED)),
                  rest)),
          rest));
}

Instruction* PolicyCompiler::DispatchSyscall() {
  
  
  Ranges ranges;
  FindRanges(&ranges);

  
  Instruction* jumptable = AssembleJumpTable(ranges.begin(), ranges.end());

  
  
  return gen_.MakeInstruction(
      BPF_LD + BPF_W + BPF_ABS, SECCOMP_NR_IDX, CheckSyscallNumber(jumptable));
}

Instruction* PolicyCompiler::CheckSyscallNumber(Instruction* passed) {
  if (kIsIntel) {
    
    
    Instruction* invalidX32 =
        RetExpression(Kill("Illegal mixing of system call ABIs"));
    if (kIsX32) {
      
      return gen_.MakeInstruction(
          BPF_JMP + BPF_JSET + BPF_K, 0x40000000, passed, invalidX32);
    } else {
      
      return gen_.MakeInstruction(
          BPF_JMP + BPF_JSET + BPF_K, 0x40000000, invalidX32, passed);
    }
  }

  
  return passed;
}

void PolicyCompiler::FindRanges(Ranges* ranges) {
  
  
  
  
  
  const ErrorCode invalid_err = policy_->InvalidSyscall()->Compile(this);
  uint32_t old_sysnum = 0;
  ErrorCode old_err = SyscallSet::IsValid(old_sysnum)
                          ? policy_->EvaluateSyscall(old_sysnum)->Compile(this)
                          : invalid_err;

  for (uint32_t sysnum : SyscallSet::All()) {
    ErrorCode err =
        SyscallSet::IsValid(sysnum)
            ? policy_->EvaluateSyscall(static_cast<int>(sysnum))->Compile(this)
            : invalid_err;
    if (!err.Equals(old_err)) {
      ranges->push_back(Range(old_sysnum, old_err));
      old_sysnum = sysnum;
      old_err = err;
    }
  }
  ranges->push_back(Range(old_sysnum, old_err));
}

Instruction* PolicyCompiler::AssembleJumpTable(Ranges::const_iterator start,
                                               Ranges::const_iterator stop) {
  
  
  
  
  if (stop - start <= 0) {
    SANDBOX_DIE("Invalid set of system call ranges");
  } else if (stop - start == 1) {
    
    
    return RetExpression(start->err);
  }

  
  
  
  
  Ranges::const_iterator mid = start + (stop - start) / 2;

  
  Instruction* jf = AssembleJumpTable(start, mid);
  Instruction* jt = AssembleJumpTable(mid, stop);
  return gen_.MakeInstruction(BPF_JMP + BPF_JGE + BPF_K, mid->from, jt, jf);
}

Instruction* PolicyCompiler::RetExpression(const ErrorCode& err) {
  switch (err.error_type()) {
    case ErrorCode::ET_COND:
      return CondExpression(err);
    case ErrorCode::ET_SIMPLE:
    case ErrorCode::ET_TRAP:
      return gen_.MakeInstruction(BPF_RET + BPF_K, err.err());
    default:
      SANDBOX_DIE("ErrorCode is not suitable for returning from a BPF program");
  }
}

Instruction* PolicyCompiler::CondExpression(const ErrorCode& cond) {
  
  if (cond.argno_ < 0 || cond.argno_ >= 6) {
    SANDBOX_DIE("sandbox_bpf: invalid argument number");
  }
  if (cond.width_ != ErrorCode::TP_32BIT &&
      cond.width_ != ErrorCode::TP_64BIT) {
    SANDBOX_DIE("sandbox_bpf: invalid argument width");
  }
  if (cond.mask_ == 0) {
    SANDBOX_DIE("sandbox_bpf: zero mask is invalid");
  }
  if ((cond.value_ & cond.mask_) != cond.value_) {
    SANDBOX_DIE("sandbox_bpf: value contains masked out bits");
  }
  if (cond.width_ == ErrorCode::TP_32BIT &&
      ((cond.mask_ >> 32) != 0 || (cond.value_ >> 32) != 0)) {
    SANDBOX_DIE("sandbox_bpf: test exceeds argument size");
  }
  
  

  Instruction* passed = RetExpression(*cond.passed_);
  Instruction* failed = RetExpression(*cond.failed_);

  
  
  
  
  return CondExpressionHalf(cond,
                            UpperHalf,
                            CondExpressionHalf(cond, LowerHalf, passed, failed),
                            failed);
}

Instruction* PolicyCompiler::CondExpressionHalf(const ErrorCode& cond,
                                                ArgHalf half,
                                                Instruction* passed,
                                                Instruction* failed) {
  if (cond.width_ == ErrorCode::TP_32BIT && half == UpperHalf) {
    
    

    
    Instruction* invalid_64bit = RetExpression(Unexpected64bitArgument());

    const uint32_t upper = SECCOMP_ARG_MSB_IDX(cond.argno_);
    const uint32_t lower = SECCOMP_ARG_LSB_IDX(cond.argno_);

    if (sizeof(void*) == 4) {
      
      
      
      return gen_.MakeInstruction(
          BPF_LD + BPF_W + BPF_ABS,
          upper,
          gen_.MakeInstruction(
              BPF_JMP + BPF_JEQ + BPF_K, 0, passed, invalid_64bit));
    }

    
    
    
    
    
    
    
    
    
    
    return gen_.MakeInstruction(
        BPF_LD + BPF_W + BPF_ABS,
        upper,
        gen_.MakeInstruction(
            BPF_JMP + BPF_JEQ + BPF_K,
            0,
            passed,
            gen_.MakeInstruction(
                BPF_JMP + BPF_JEQ + BPF_K,
                std::numeric_limits<uint32_t>::max(),
                gen_.MakeInstruction(
                    BPF_LD + BPF_W + BPF_ABS,
                    lower,
                    gen_.MakeInstruction(BPF_JMP + BPF_JSET + BPF_K,
                                         1U << 31,
                                         passed,
                                         invalid_64bit)),
                invalid_64bit)));
  }

  const uint32_t idx = (half == UpperHalf) ? SECCOMP_ARG_MSB_IDX(cond.argno_)
                                           : SECCOMP_ARG_LSB_IDX(cond.argno_);
  const uint32_t mask = (half == UpperHalf) ? cond.mask_ >> 32 : cond.mask_;
  const uint32_t value = (half == UpperHalf) ? cond.value_ >> 32 : cond.value_;

  

  
  if (mask == 0) {
    CHECK_EQ(0U, value);
    return passed;
  }

  
  
  
  if (mask == std::numeric_limits<uint32_t>::max()) {
    return gen_.MakeInstruction(
        BPF_LD + BPF_W + BPF_ABS,
        idx,
        gen_.MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, value, passed, failed));
  }

  
  
  
  
  if (value == 0) {
    return gen_.MakeInstruction(
        BPF_LD + BPF_W + BPF_ABS,
        idx,
        gen_.MakeInstruction(BPF_JMP + BPF_JSET + BPF_K, mask, failed, passed));
  }

  
  
  
  if (mask == value && HasExactlyOneBit(mask)) {
    return gen_.MakeInstruction(
        BPF_LD + BPF_W + BPF_ABS,
        idx,
        gen_.MakeInstruction(BPF_JMP + BPF_JSET + BPF_K, mask, passed, failed));
  }

  
  
  
  
  return gen_.MakeInstruction(
      BPF_LD + BPF_W + BPF_ABS,
      idx,
      gen_.MakeInstruction(
          BPF_ALU + BPF_AND + BPF_K,
          mask,
          gen_.MakeInstruction(
              BPF_JMP + BPF_JEQ + BPF_K, value, passed, failed)));
}

ErrorCode PolicyCompiler::Unexpected64bitArgument() {
  return Kill("Unexpected 64bit argument detected");
}

ErrorCode PolicyCompiler::Error(int err) {
  if (has_unsafe_traps_) {
    
    
    
    
    
    
    
    
    
    return Trap(ReturnErrno, reinterpret_cast<void*>(err));
  }

  return ErrorCode(err);
}

ErrorCode PolicyCompiler::MakeTrap(TrapRegistry::TrapFnc fnc,
                                   const void* aux,
                                   bool safe) {
  uint16_t trap_id = registry_->Add(fnc, aux, safe);
  return ErrorCode(trap_id, fnc, aux, safe);
}

ErrorCode PolicyCompiler::Trap(TrapRegistry::TrapFnc fnc, const void* aux) {
  return MakeTrap(fnc, aux, true );
}

ErrorCode PolicyCompiler::UnsafeTrap(TrapRegistry::TrapFnc fnc,
                                     const void* aux) {
  return MakeTrap(fnc, aux, false );
}

bool PolicyCompiler::IsRequiredForUnsafeTrap(int sysno) {
  for (size_t i = 0; i < arraysize(kSyscallsRequiredForUnsafeTraps); ++i) {
    if (sysno == kSyscallsRequiredForUnsafeTraps[i]) {
      return true;
    }
  }
  return false;
}

ErrorCode PolicyCompiler::CondMaskedEqual(int argno,
                                          ErrorCode::ArgType width,
                                          uint64_t mask,
                                          uint64_t value,
                                          const ErrorCode& passed,
                                          const ErrorCode& failed) {
  return ErrorCode(argno,
                   width,
                   mask,
                   value,
                   &*conds_.insert(passed).first,
                   &*conds_.insert(failed).first);
}

ErrorCode PolicyCompiler::Kill(const char* msg) {
  return Trap(BPFFailure, const_cast<char*>(msg));
}

}  
}  
