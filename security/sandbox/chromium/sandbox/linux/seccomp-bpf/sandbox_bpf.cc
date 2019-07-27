



#include "sandbox/linux/seccomp-bpf/sandbox_bpf.h"



#if defined(ANDROID)
#include <sys/cdefs.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/posix/eintr_wrapper.h"
#include "sandbox/linux/seccomp-bpf/codegen.h"
#include "sandbox/linux/seccomp-bpf/sandbox_bpf_policy.h"
#include "sandbox/linux/seccomp-bpf/syscall.h"
#include "sandbox/linux/seccomp-bpf/syscall_iterator.h"
#include "sandbox/linux/seccomp-bpf/verifier.h"

namespace sandbox {

namespace {

const int kExpectedExitCode = 100;

int popcount(uint32_t x) {
  return __builtin_popcount(x);
}

#if !defined(NDEBUG)
void WriteFailedStderrSetupMessage(int out_fd) {
  const char* error_string = strerror(errno);
  static const char msg[] =
      "You have reproduced a puzzling issue.\n"
      "Please, report to crbug.com/152530!\n"
      "Failed to set up stderr: ";
  if (HANDLE_EINTR(write(out_fd, msg, sizeof(msg) - 1)) > 0 && error_string &&
      HANDLE_EINTR(write(out_fd, error_string, strlen(error_string))) > 0 &&
      HANDLE_EINTR(write(out_fd, "\n", 1))) {
  }
}
#endif  



ErrorCode ProbeEvaluator(SandboxBPF*, int sysnum, void*) __attribute__((const));
ErrorCode ProbeEvaluator(SandboxBPF*, int sysnum, void*) {
  switch (sysnum) {
    case __NR_getpid:
      
      return ErrorCode(EPERM);
    case __NR_exit_group:
      
      return ErrorCode(ErrorCode::ERR_ALLOWED);
    default:
      
      return ErrorCode(EINVAL);
  }
}

void ProbeProcess(void) {
  if (syscall(__NR_getpid) < 0 && errno == EPERM) {
    syscall(__NR_exit_group, static_cast<intptr_t>(kExpectedExitCode));
  }
}

ErrorCode AllowAllEvaluator(SandboxBPF*, int sysnum, void*) {
  if (!SandboxBPF::IsValidSyscallNumber(sysnum)) {
    return ErrorCode(ENOSYS);
  }
  return ErrorCode(ErrorCode::ERR_ALLOWED);
}

void TryVsyscallProcess(void) {
  time_t current_time;
  
  
  
  if (time(&current_time) != static_cast<time_t>(-1)) {
    syscall(__NR_exit_group, static_cast<intptr_t>(kExpectedExitCode));
  }
}

bool IsSingleThreaded(int proc_fd) {
  if (proc_fd < 0) {
    
    
    return true;
  }

  struct stat sb;
  int task = -1;
  if ((task = openat(proc_fd, "self/task", O_RDONLY | O_DIRECTORY)) < 0 ||
      fstat(task, &sb) != 0 || sb.st_nlink != 3 || IGNORE_EINTR(close(task))) {
    if (task >= 0) {
      if (IGNORE_EINTR(close(task))) {
      }
    }
    return false;
  }
  return true;
}

bool IsDenied(const ErrorCode& code) {
  return (code.err() & SECCOMP_RET_ACTION) == SECCOMP_RET_TRAP ||
         (code.err() >= (SECCOMP_RET_ERRNO + ErrorCode::ERR_MIN_ERRNO) &&
          code.err() <= (SECCOMP_RET_ERRNO + ErrorCode::ERR_MAX_ERRNO));
}




void CheckForUnsafeErrorCodes(Instruction* insn, void* aux) {
  bool* is_unsafe = static_cast<bool*>(aux);
  if (!*is_unsafe) {
    if (BPF_CLASS(insn->code) == BPF_RET && insn->k > SECCOMP_RET_TRAP &&
        insn->k - SECCOMP_RET_TRAP <= SECCOMP_RET_DATA) {
      const ErrorCode& err =
          Trap::ErrorCodeFromTrapId(insn->k & SECCOMP_RET_DATA);
      if (err.error_type() != ErrorCode::ET_INVALID && !err.safe()) {
        *is_unsafe = true;
      }
    }
  }
}



intptr_t ReturnErrno(const struct arch_seccomp_data&, void* aux) {
  
  
  
  
  int err = reinterpret_cast<intptr_t>(aux) & SECCOMP_RET_DATA;
  return -err;
}





void RedirectToUserspace(Instruction* insn, void* aux) {
  
  
  
  
  
  
  
  
  
  SandboxBPF* sandbox = static_cast<SandboxBPF*>(aux);
  if (BPF_CLASS(insn->code) == BPF_RET &&
      (insn->k & SECCOMP_RET_ACTION) == SECCOMP_RET_ERRNO) {
    insn->k = sandbox->Trap(ReturnErrno,
        reinterpret_cast<void*>(insn->k & SECCOMP_RET_DATA)).err();
  }
}





class RedirectToUserSpacePolicyWrapper : public SandboxBPFPolicy {
 public:
  explicit RedirectToUserSpacePolicyWrapper(
      const SandboxBPFPolicy* wrapped_policy)
      : wrapped_policy_(wrapped_policy) {
    DCHECK(wrapped_policy_);
  }

  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox_compiler,
                                    int system_call_number) const OVERRIDE {
    ErrorCode err =
        wrapped_policy_->EvaluateSyscall(sandbox_compiler, system_call_number);
    if ((err.err() & SECCOMP_RET_ACTION) == SECCOMP_RET_ERRNO) {
      return sandbox_compiler->Trap(
          ReturnErrno, reinterpret_cast<void*>(err.err() & SECCOMP_RET_DATA));
    }
    return err;
  }

 private:
  const SandboxBPFPolicy* wrapped_policy_;
  DISALLOW_COPY_AND_ASSIGN(RedirectToUserSpacePolicyWrapper);
};

intptr_t BPFFailure(const struct arch_seccomp_data&, void* aux) {
  SANDBOX_DIE(static_cast<char*>(aux));
}


class CompatibilityPolicy : public SandboxBPFPolicy {
 public:
  CompatibilityPolicy(SandboxBPF::EvaluateSyscall syscall_evaluator, void* aux)
      : syscall_evaluator_(syscall_evaluator), aux_(aux) {
    DCHECK(syscall_evaluator_);
  }

  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox_compiler,
                                    int system_call_number) const OVERRIDE {
    return syscall_evaluator_(sandbox_compiler, system_call_number, aux_);
  }

 private:
  SandboxBPF::EvaluateSyscall syscall_evaluator_;
  void* aux_;
  DISALLOW_COPY_AND_ASSIGN(CompatibilityPolicy);
};

}  

SandboxBPF::SandboxBPF()
    : quiet_(false),
      proc_fd_(-1),
      conds_(new Conds),
      sandbox_has_started_(false) {}

SandboxBPF::~SandboxBPF() {
  
  
  
  
  
  
  
  
  
  
  
  if (conds_) {
    delete conds_;
  }
}

bool SandboxBPF::IsValidSyscallNumber(int sysnum) {
  return SyscallIterator::IsValid(sysnum);
}

bool SandboxBPF::RunFunctionInPolicy(void (*code_in_sandbox)(),
                                     EvaluateSyscall syscall_evaluator,
                                     void* aux) {
  
  
  sigset_t old_mask, new_mask;
  if (sigfillset(&new_mask) || sigprocmask(SIG_BLOCK, &new_mask, &old_mask)) {
    SANDBOX_DIE("sigprocmask() failed");
  }
  int fds[2];
  if (pipe2(fds, O_NONBLOCK | O_CLOEXEC)) {
    SANDBOX_DIE("pipe() failed");
  }

  if (fds[0] <= 2 || fds[1] <= 2) {
    SANDBOX_DIE("Process started without standard file descriptors");
  }

  
  
  
  DCHECK(IsSingleThreaded(proc_fd_));
  pid_t pid = fork();
  if (pid < 0) {
    
    
    
    
    
    
    sigprocmask(SIG_SETMASK, &old_mask, NULL);  
    SANDBOX_DIE("fork() failed unexpectedly");
  }

  
  if (!pid) {
    
    
    Die::EnableSimpleExit();

    errno = 0;
    if (IGNORE_EINTR(close(fds[0]))) {
      
      
#if !defined(NDEBUG)
      WriteFailedStderrSetupMessage(fds[1]);
      SANDBOX_DIE(NULL);
#endif
    }
    if (HANDLE_EINTR(dup2(fds[1], 2)) != 2) {
      
      
      
      
      
      
      
#if !defined(NDEBUG)
      
      WriteFailedStderrSetupMessage(fds[1]);
      SANDBOX_DIE(NULL);
#endif
    }
    if (IGNORE_EINTR(close(fds[1]))) {
      
      
#if !defined(NDEBUG)
      WriteFailedStderrSetupMessage(fds[1]);
      SANDBOX_DIE(NULL);
#endif
    }

    SetSandboxPolicyDeprecated(syscall_evaluator, aux);
    if (!StartSandbox(PROCESS_SINGLE_THREADED)) {
      SANDBOX_DIE(NULL);
    }

    
    code_in_sandbox();

    
    SANDBOX_DIE(NULL);
  }

  
  if (IGNORE_EINTR(close(fds[1]))) {
    SANDBOX_DIE("close() failed");
  }
  if (sigprocmask(SIG_SETMASK, &old_mask, NULL)) {
    SANDBOX_DIE("sigprocmask() failed");
  }
  int status;
  if (HANDLE_EINTR(waitpid(pid, &status, 0)) != pid) {
    SANDBOX_DIE("waitpid() failed unexpectedly");
  }
  bool rc = WIFEXITED(status) && WEXITSTATUS(status) == kExpectedExitCode;

  
  
  
  
  
  if (!rc) {
    char buf[4096];
    ssize_t len = HANDLE_EINTR(read(fds[0], buf, sizeof(buf) - 1));
    if (len > 0) {
      while (len > 1 && buf[len - 1] == '\n') {
        --len;
      }
      buf[len] = '\000';
      SANDBOX_DIE(buf);
    }
  }
  if (IGNORE_EINTR(close(fds[0]))) {
    SANDBOX_DIE("close() failed");
  }

  return rc;
}

bool SandboxBPF::KernelSupportSeccompBPF() {
  return RunFunctionInPolicy(ProbeProcess, ProbeEvaluator, 0) &&
         RunFunctionInPolicy(TryVsyscallProcess, AllowAllEvaluator, 0);
}

SandboxBPF::SandboxStatus SandboxBPF::SupportsSeccompSandbox(int proc_fd) {
  
  
  if (status_ == STATUS_ENABLED) {
    return status_;
  }

  
  
  if (status_ == STATUS_AVAILABLE) {
    if (!IsSingleThreaded(proc_fd)) {
      status_ = STATUS_UNAVAILABLE;
    }
    return status_;
  }

  if (status_ == STATUS_UNAVAILABLE && IsSingleThreaded(proc_fd)) {
    
    
    
    
    
    
    
    status_ = STATUS_AVAILABLE;
    return status_;
  }

  
  
  
  if (status_ == STATUS_UNKNOWN) {
    
    
    
    SandboxBPF sandbox;

    
    
    sandbox.quiet_ = true;
    sandbox.set_proc_fd(proc_fd);
    status_ = sandbox.KernelSupportSeccompBPF() ? STATUS_AVAILABLE
                                                : STATUS_UNSUPPORTED;

    
    
    
    
    if (status_ == STATUS_AVAILABLE && !IsSingleThreaded(proc_fd)) {
      status_ = STATUS_UNAVAILABLE;
    }
  }
  return status_;
}

void SandboxBPF::set_proc_fd(int proc_fd) { proc_fd_ = proc_fd; }

bool SandboxBPF::StartSandbox(SandboxThreadState thread_state) {
  CHECK(thread_state == PROCESS_SINGLE_THREADED ||
        thread_state == PROCESS_MULTI_THREADED);

  if (status_ == STATUS_UNSUPPORTED || status_ == STATUS_UNAVAILABLE) {
    SANDBOX_DIE(
        "Trying to start sandbox, even though it is known to be "
        "unavailable");
    return false;
  } else if (sandbox_has_started_ || !conds_) {
    SANDBOX_DIE(
        "Cannot repeatedly start sandbox. Create a separate Sandbox "
        "object instead.");
    return false;
  }
  if (proc_fd_ < 0) {
    proc_fd_ = open("/proc", O_RDONLY | O_DIRECTORY);
  }
  if (proc_fd_ < 0) {
    
    
  }

  if (thread_state == PROCESS_SINGLE_THREADED && !IsSingleThreaded(proc_fd_)) {
    SANDBOX_DIE("Cannot start sandbox, if process is already multi-threaded");
    return false;
  }

  
  
  
  if (proc_fd_ >= 0) {
    if (IGNORE_EINTR(close(proc_fd_))) {
      SANDBOX_DIE("Failed to close file descriptor for /proc");
      return false;
    }
    proc_fd_ = -1;
  }

  
  InstallFilter(thread_state);

  
  status_ = STATUS_ENABLED;

  return true;
}

void SandboxBPF::PolicySanityChecks(SandboxBPFPolicy* policy) {
  for (SyscallIterator iter(true); !iter.Done();) {
    uint32_t sysnum = iter.Next();
    if (!IsDenied(policy->EvaluateSyscall(this, sysnum))) {
      SANDBOX_DIE(
          "Policies should deny system calls that are outside the "
          "expected range (typically MIN_SYSCALL..MAX_SYSCALL)");
    }
  }
  return;
}


void SandboxBPF::SetSandboxPolicyDeprecated(EvaluateSyscall syscall_evaluator,
                                            void* aux) {
  if (sandbox_has_started_ || !conds_) {
    SANDBOX_DIE("Cannot change policy after sandbox has started");
  }
  SetSandboxPolicy(new CompatibilityPolicy(syscall_evaluator, aux));
}


void SandboxBPF::SetSandboxPolicy(SandboxBPFPolicy* policy) {
  DCHECK(!policy_);
  if (sandbox_has_started_ || !conds_) {
    SANDBOX_DIE("Cannot change policy after sandbox has started");
  }
  PolicySanityChecks(policy);
  policy_.reset(policy);
}

void SandboxBPF::InstallFilter(SandboxThreadState thread_state) {
  
  
  
  
  
  
  
  
  
  
  
  Program* program = AssembleFilter(false );

  struct sock_filter bpf[program->size()];
  const struct sock_fprog prog = {static_cast<unsigned short>(program->size()),
                                  bpf};
  memcpy(bpf, &(*program)[0], sizeof(bpf));
  delete program;

  
  
  
  delete conds_;
  conds_ = NULL;
  policy_.reset();

  
  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
    SANDBOX_DIE(quiet_ ? NULL : "Kernel refuses to enable no-new-privs");
  } else {
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog)) {
      SANDBOX_DIE(quiet_ ? NULL : "Kernel refuses to turn on BPF filters");
    }
  }

  
  
  

  if (thread_state == PROCESS_MULTI_THREADED) {
    
    
    #define PR_SECCOMP_EXT 41
    #define SECCOMP_EXT_ACT 1
    #define SECCOMP_EXT_ACT_TSYNC 1
    if (prctl(PR_SECCOMP_EXT, SECCOMP_EXT_ACT, SECCOMP_EXT_ACT_TSYNC, 0, 0)) {
      SANDBOX_DIE(quiet_ ? NULL : "Kernel refuses to synchronize threadgroup "
                                  "BPF filters.");
    }
  }

  sandbox_has_started_ = true;
}

SandboxBPF::Program* SandboxBPF::AssembleFilter(bool force_verification) {
#if !defined(NDEBUG)
  force_verification = true;
#endif

  
  DCHECK(policy_);

  
  CodeGen* gen = new CodeGen();
  if (!gen) {
    SANDBOX_DIE("Out of memory");
  }

  
  
  Instruction* tail;
  Instruction* head = gen->MakeInstruction(
      BPF_LD + BPF_W + BPF_ABS,
      SECCOMP_ARCH_IDX,
      tail = gen->MakeInstruction(
          BPF_JMP + BPF_JEQ + BPF_K,
          SECCOMP_ARCH,
          NULL,
          gen->MakeInstruction(
              BPF_RET + BPF_K,
              Kill("Invalid audit architecture in BPF filter"))));

  bool has_unsafe_traps = false;
  {
    
    
    Ranges ranges;
    FindRanges(&ranges);

    
    Instruction* jumptable =
        AssembleJumpTable(gen, ranges.begin(), ranges.end());

    
    
    
    
    
    gen->Traverse(jumptable, CheckForUnsafeErrorCodes, &has_unsafe_traps);

    
    Instruction* load_nr =
        gen->MakeInstruction(BPF_LD + BPF_W + BPF_ABS, SECCOMP_NR_IDX);

    
    
    
    
    
    
    
    if (has_unsafe_traps) {
      if (SandboxSyscall(-1) == -1 && errno == ENOSYS) {
        SANDBOX_DIE(
            "Support for UnsafeTrap() has not yet been ported to this "
            "architecture");
      }

      if (!policy_->EvaluateSyscall(this, __NR_rt_sigprocmask)
               .Equals(ErrorCode(ErrorCode::ERR_ALLOWED)) ||
          !policy_->EvaluateSyscall(this, __NR_rt_sigreturn)
               .Equals(ErrorCode(ErrorCode::ERR_ALLOWED))
#if defined(__NR_sigprocmask)
          ||
          !policy_->EvaluateSyscall(this, __NR_sigprocmask)
               .Equals(ErrorCode(ErrorCode::ERR_ALLOWED))
#endif
#if defined(__NR_sigreturn)
          ||
          !policy_->EvaluateSyscall(this, __NR_sigreturn)
               .Equals(ErrorCode(ErrorCode::ERR_ALLOWED))
#endif
          ) {
        SANDBOX_DIE(
            "Invalid seccomp policy; if using UnsafeTrap(), you must "
            "unconditionally allow sigreturn() and sigprocmask()");
      }

      if (!Trap::EnableUnsafeTrapsInSigSysHandler()) {
        
        
        
        
        
        SANDBOX_DIE("We'd rather die than enable unsafe traps");
      }
      gen->Traverse(jumptable, RedirectToUserspace, this);

      
      
      uintptr_t syscall_entry_point =
          static_cast<uintptr_t>(SandboxSyscall(-1));
      uint32_t low = static_cast<uint32_t>(syscall_entry_point);
#if __SIZEOF_POINTER__ > 4
      uint32_t hi = static_cast<uint32_t>(syscall_entry_point >> 32);
#endif

      
      
      
      
      Instruction* escape_hatch = gen->MakeInstruction(
          BPF_LD + BPF_W + BPF_ABS,
          SECCOMP_IP_LSB_IDX,
          gen->MakeInstruction(
              BPF_JMP + BPF_JEQ + BPF_K,
              low,
#if __SIZEOF_POINTER__ > 4
              gen->MakeInstruction(
                  BPF_LD + BPF_W + BPF_ABS,
                  SECCOMP_IP_MSB_IDX,
                  gen->MakeInstruction(
                      BPF_JMP + BPF_JEQ + BPF_K,
                      hi,
#endif
                      gen->MakeInstruction(BPF_RET + BPF_K,
                                           ErrorCode(ErrorCode::ERR_ALLOWED)),
#if __SIZEOF_POINTER__ > 4
                      load_nr)),
#endif
              load_nr));
      gen->JoinInstructions(tail, escape_hatch);
    } else {
      gen->JoinInstructions(tail, load_nr);
    }
    tail = load_nr;




#if defined(__i386__) || defined(__x86_64__)
    Instruction* invalidX32 = gen->MakeInstruction(
        BPF_RET + BPF_K, Kill("Illegal mixing of system call ABIs").err_);
    Instruction* checkX32 =
#if defined(__x86_64__) && defined(__ILP32__)
        gen->MakeInstruction(
            BPF_JMP + BPF_JSET + BPF_K, 0x40000000, 0, invalidX32);
#else
        gen->MakeInstruction(
            BPF_JMP + BPF_JSET + BPF_K, 0x40000000, invalidX32, 0);
#endif
    gen->JoinInstructions(tail, checkX32);
    tail = checkX32;
#endif

    
    gen->JoinInstructions(tail, jumptable);
  }

  
  Program* program = new Program();
  gen->Compile(head, program);
  delete gen;

  
  
  
  if (force_verification) {
    
    
    
    VerifyProgram(*program, has_unsafe_traps);
  }

  return program;
}

void SandboxBPF::VerifyProgram(const Program& program, bool has_unsafe_traps) {
  
  
  
  
  scoped_ptr<const RedirectToUserSpacePolicyWrapper> redirected_policy(
      new RedirectToUserSpacePolicyWrapper(policy_.get()));

  const char* err = NULL;
  if (!Verifier::VerifyBPF(this,
                           program,
                           has_unsafe_traps ? *redirected_policy : *policy_,
                           &err)) {
    CodeGen::PrintProgram(program);
    SANDBOX_DIE(err);
  }
}

void SandboxBPF::FindRanges(Ranges* ranges) {
  
  
  
  
  
  uint32_t old_sysnum = 0;
  ErrorCode old_err = policy_->EvaluateSyscall(this, old_sysnum);
  ErrorCode invalid_err = policy_->EvaluateSyscall(this, MIN_SYSCALL - 1);

  for (SyscallIterator iter(false); !iter.Done();) {
    uint32_t sysnum = iter.Next();
    ErrorCode err = policy_->EvaluateSyscall(this, static_cast<int>(sysnum));
    if (!iter.IsValid(sysnum) && !invalid_err.Equals(err)) {
      
      
      
      
      SANDBOX_DIE("Invalid seccomp policy");
    }
    if (!err.Equals(old_err) || iter.Done()) {
      ranges->push_back(Range(old_sysnum, sysnum - 1, old_err));
      old_sysnum = sysnum;
      old_err = err;
    }
  }
}

Instruction* SandboxBPF::AssembleJumpTable(CodeGen* gen,
                                           Ranges::const_iterator start,
                                           Ranges::const_iterator stop) {
  
  
  
  
  if (stop - start <= 0) {
    SANDBOX_DIE("Invalid set of system call ranges");
  } else if (stop - start == 1) {
    
    
    return RetExpression(gen, start->err);
  }

  
  
  
  
  Ranges::const_iterator mid = start + (stop - start) / 2;

  
  Instruction* jf = AssembleJumpTable(gen, start, mid);
  Instruction* jt = AssembleJumpTable(gen, mid, stop);
  return gen->MakeInstruction(BPF_JMP + BPF_JGE + BPF_K, mid->from, jt, jf);
}

Instruction* SandboxBPF::RetExpression(CodeGen* gen, const ErrorCode& err) {
  if (err.error_type_ == ErrorCode::ET_COND) {
    return CondExpression(gen, err);
  } else {
    return gen->MakeInstruction(BPF_RET + BPF_K, err);
  }
}

Instruction* SandboxBPF::CondExpression(CodeGen* gen, const ErrorCode& cond) {
  
  
  if (cond.argno_ < 0 || cond.argno_ >= 6) {
    SANDBOX_DIE(
        "Internal compiler error; invalid argument number "
        "encountered");
  }

  
  
  Instruction* msb_head = gen->MakeInstruction(
      BPF_LD + BPF_W + BPF_ABS, SECCOMP_ARG_MSB_IDX(cond.argno_));
  Instruction* msb_tail = msb_head;
  Instruction* lsb_head = gen->MakeInstruction(
      BPF_LD + BPF_W + BPF_ABS, SECCOMP_ARG_LSB_IDX(cond.argno_));
  Instruction* lsb_tail = lsb_head;

  
  switch (cond.op_) {
    case ErrorCode::OP_EQUAL:
      
      lsb_tail = gen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K,
                                      static_cast<uint32_t>(cond.value_),
                                      RetExpression(gen, *cond.passed_),
                                      RetExpression(gen, *cond.failed_));
      gen->JoinInstructions(lsb_head, lsb_tail);

      
      
      if (cond.width_ == ErrorCode::TP_64BIT) {
        msb_tail =
            gen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K,
                                 static_cast<uint32_t>(cond.value_ >> 32),
                                 lsb_head,
                                 RetExpression(gen, *cond.failed_));
        gen->JoinInstructions(msb_head, msb_tail);
      }
      break;
    case ErrorCode::OP_HAS_ALL_BITS:
      
      
      
      
      
      
      {
        uint32_t lsb_bits = static_cast<uint32_t>(cond.value_);
        int lsb_bit_count = popcount(lsb_bits);
        if (lsb_bit_count == 0) {
          
          lsb_head = RetExpression(gen, *cond.passed_);
          lsb_tail = NULL;
        } else if (lsb_bit_count == 1) {
          
          
          lsb_tail = gen->MakeInstruction(BPF_JMP + BPF_JSET + BPF_K,
                                          lsb_bits,
                                          RetExpression(gen, *cond.passed_),
                                          RetExpression(gen, *cond.failed_));
          gen->JoinInstructions(lsb_head, lsb_tail);
        } else {
          
          
          
          gen->JoinInstructions(
              lsb_head,
              gen->MakeInstruction(BPF_ALU + BPF_AND + BPF_K,
                                   lsb_bits,
                                   lsb_tail = gen->MakeInstruction(
                                       BPF_JMP + BPF_JEQ + BPF_K,
                                       lsb_bits,
                                       RetExpression(gen, *cond.passed_),
                                       RetExpression(gen, *cond.failed_))));
        }
      }

      
      
      if (cond.width_ == ErrorCode::TP_64BIT) {
        uint32_t msb_bits = static_cast<uint32_t>(cond.value_ >> 32);
        int msb_bit_count = popcount(msb_bits);
        if (msb_bit_count == 0) {
          
          msb_head = lsb_head;
        } else if (msb_bit_count == 1) {
          
          
          msb_tail = gen->MakeInstruction(BPF_JMP + BPF_JSET + BPF_K,
                                          msb_bits,
                                          lsb_head,
                                          RetExpression(gen, *cond.failed_));
          gen->JoinInstructions(msb_head, msb_tail);
        } else {
          
          
          
          gen->JoinInstructions(
              msb_head,
              gen->MakeInstruction(
                  BPF_ALU + BPF_AND + BPF_K,
                  msb_bits,
                  gen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K,
                                       msb_bits,
                                       lsb_head,
                                       RetExpression(gen, *cond.failed_))));
        }
      }
      break;
    case ErrorCode::OP_HAS_ANY_BITS:
      
      
      
      {
        uint32_t lsb_bits = static_cast<uint32_t>(cond.value_);
        if (!lsb_bits) {
          
          lsb_head = RetExpression(gen, *cond.failed_);
          lsb_tail = NULL;
        } else {
          lsb_tail = gen->MakeInstruction(BPF_JMP + BPF_JSET + BPF_K,
                                          lsb_bits,
                                          RetExpression(gen, *cond.passed_),
                                          RetExpression(gen, *cond.failed_));
          gen->JoinInstructions(lsb_head, lsb_tail);
        }
      }

      
      
      if (cond.width_ == ErrorCode::TP_64BIT) {
        uint32_t msb_bits = static_cast<uint32_t>(cond.value_ >> 32);
        if (!msb_bits) {
          
          msb_head = lsb_head;
        } else {
          msb_tail = gen->MakeInstruction(BPF_JMP + BPF_JSET + BPF_K,
                                          msb_bits,
                                          RetExpression(gen, *cond.passed_),
                                          lsb_head);
          gen->JoinInstructions(msb_head, msb_tail);
        }
      }
      break;
    default:
      
      SANDBOX_DIE("Not implemented");
      break;
  }

  
  
  
  
  if (cond.width_ == ErrorCode::TP_32BIT) {
    if (cond.value_ >> 32) {
      SANDBOX_DIE(
          "Invalid comparison of a 32bit system call argument "
          "against a 64bit constant; this test is always false.");
    }

    Instruction* invalid_64bit = RetExpression(gen, Unexpected64bitArgument());
#if __SIZEOF_POINTER__ > 4
    invalid_64bit = gen->MakeInstruction(
        BPF_JMP + BPF_JEQ + BPF_K,
        0xFFFFFFFF,
        gen->MakeInstruction(BPF_LD + BPF_W + BPF_ABS,
                             SECCOMP_ARG_LSB_IDX(cond.argno_),
                             gen->MakeInstruction(BPF_JMP + BPF_JGE + BPF_K,
                                                  0x80000000,
                                                  lsb_head,
                                                  invalid_64bit)),
        invalid_64bit);
#endif
    gen->JoinInstructions(
        msb_tail,
        gen->MakeInstruction(
            BPF_JMP + BPF_JEQ + BPF_K, 0, lsb_head, invalid_64bit));
  }

  return msb_head;
}

ErrorCode SandboxBPF::Unexpected64bitArgument() {
  return Kill("Unexpected 64bit argument detected");
}

ErrorCode SandboxBPF::Trap(Trap::TrapFnc fnc, const void* aux) {
  return Trap::MakeTrap(fnc, aux, true );
}

ErrorCode SandboxBPF::UnsafeTrap(Trap::TrapFnc fnc, const void* aux) {
  return Trap::MakeTrap(fnc, aux, false );
}

intptr_t SandboxBPF::ForwardSyscall(const struct arch_seccomp_data& args) {
  return SandboxSyscall(args.nr,
                        static_cast<intptr_t>(args.args[0]),
                        static_cast<intptr_t>(args.args[1]),
                        static_cast<intptr_t>(args.args[2]),
                        static_cast<intptr_t>(args.args[3]),
                        static_cast<intptr_t>(args.args[4]),
                        static_cast<intptr_t>(args.args[5]));
}

ErrorCode SandboxBPF::Cond(int argno,
                           ErrorCode::ArgType width,
                           ErrorCode::Operation op,
                           uint64_t value,
                           const ErrorCode& passed,
                           const ErrorCode& failed) {
  return ErrorCode(argno,
                   width,
                   op,
                   value,
                   &*conds_->insert(passed).first,
                   &*conds_->insert(failed).first);
}

ErrorCode SandboxBPF::Kill(const char* msg) {
  return Trap(BPFFailure, const_cast<char*>(msg));
}

SandboxBPF::SandboxStatus SandboxBPF::status_ = STATUS_UNKNOWN;

}  
