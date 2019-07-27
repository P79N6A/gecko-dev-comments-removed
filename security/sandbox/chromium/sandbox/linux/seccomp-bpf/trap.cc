



#include "sandbox/linux/seccomp-bpf/trap.h"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/syscall.h>

#include <algorithm>
#include <limits>

#include "base/logging.h"
#include "build/build_config.h"
#include "sandbox/linux/seccomp-bpf/die.h"
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"
#include "sandbox/linux/seccomp-bpf/syscall.h"


#if defined(OS_ANDROID)
#include "sandbox/linux/services/android_ucontext.h"
#endif

namespace {

struct arch_sigsys {
  void* ip;
  int nr;
  unsigned int arch;
};

const int kCapacityIncrement = 20;



const char kSandboxDebuggingEnv[] = "CHROME_SANDBOX_DEBUGGING";














bool GetIsInSigHandler(const ucontext_t* ctx) {
  
  return sigismember(const_cast<sigset_t*>(&ctx->uc_sigmask), SIGBUS);
}

void SetIsInSigHandler() {
  sigset_t mask;
  if (sigemptyset(&mask) || sigaddset(&mask, SIGBUS) ||
      sigprocmask(SIG_BLOCK, &mask, NULL)) {
    SANDBOX_DIE("Failed to block SIGBUS");
  }
}

bool IsDefaultSignalAction(const struct sigaction& sa) {
  if (sa.sa_flags & SA_SIGINFO || sa.sa_handler != SIG_DFL) {
    return false;
  }
  return true;
}

}  

namespace sandbox {

Trap::Trap()
    : trap_array_(NULL),
      trap_array_size_(0),
      trap_array_capacity_(0),
      has_unsafe_traps_(false) {
  
  struct sigaction sa = {};
  sa.sa_sigaction = SigSysAction;
  sa.sa_flags = SA_SIGINFO | SA_NODEFER;
  struct sigaction old_sa;
  if (sigaction(SIGSYS, &sa, &old_sa) < 0) {
    SANDBOX_DIE("Failed to configure SIGSYS handler");
  }

  if (!IsDefaultSignalAction(old_sa)) {
    static const char kExistingSIGSYSMsg[] =
        "Existing signal handler when trying to install SIGSYS. SIGSYS needs "
        "to be reserved for seccomp-bpf.";
    DLOG(FATAL) << kExistingSIGSYSMsg;
    LOG(ERROR) << kExistingSIGSYSMsg;
  }

  
  sigset_t mask;
  if (sigemptyset(&mask) || sigaddset(&mask, SIGSYS) ||
      sigprocmask(SIG_UNBLOCK, &mask, NULL)) {
    SANDBOX_DIE("Failed to configure SIGSYS handler");
  }
}

bpf_dsl::TrapRegistry* Trap::Registry() {
  
  
  
  
  
  if (!global_trap_) {
    global_trap_ = new Trap();
    if (!global_trap_) {
      SANDBOX_DIE("Failed to allocate global trap handler");
    }
  }
  return global_trap_;
}

void Trap::SigSysAction(int nr, siginfo_t* info, void* void_context) {
  if (!global_trap_) {
    RAW_SANDBOX_DIE(
        "This can't happen. Found no global singleton instance "
        "for Trap() handling.");
  }
  global_trap_->SigSys(nr, info, void_context);
}

void Trap::SigSys(int nr, siginfo_t* info, void* void_context) {
  
  
  const int old_errno = errno;

  
  
  
  if (nr != SIGSYS || info->si_code != SYS_SECCOMP || !void_context ||
      info->si_errno <= 0 ||
      static_cast<size_t>(info->si_errno) > trap_array_size_) {
    
    
    
    RAW_LOG(ERROR, "Unexpected SIGSYS received.");
    errno = old_errno;
    return;
  }

  
  
  ucontext_t* ctx = reinterpret_cast<ucontext_t*>(void_context);

  
  
  
  struct arch_sigsys sigsys;
  memcpy(&sigsys, &info->_sifields, sizeof(sigsys));

#if defined(__mips__)
  
  
  
  bool sigsys_nr_is_bad = sigsys.nr != static_cast<int>(SECCOMP_SYSCALL(ctx)) &&
                          sigsys.nr != static_cast<int>(SECCOMP_PARM1(ctx));
#else
  bool sigsys_nr_is_bad = sigsys.nr != static_cast<int>(SECCOMP_SYSCALL(ctx));
#endif

  
  if (sigsys.ip != reinterpret_cast<void*>(SECCOMP_IP(ctx)) ||
      sigsys_nr_is_bad || sigsys.arch != SECCOMP_ARCH) {
    
    
    
    
    
    RAW_SANDBOX_DIE("Sanity checks are failing after receiving SIGSYS.");
  }

  intptr_t rc;
  if (has_unsafe_traps_ && GetIsInSigHandler(ctx)) {
    errno = old_errno;
    if (sigsys.nr == __NR_clone) {
      RAW_SANDBOX_DIE("Cannot call clone() from an UnsafeTrap() handler.");
    }
#if defined(__mips__)
    
    
    
    rc = Syscall::Call(SECCOMP_SYSCALL(ctx),
                       SECCOMP_PARM1(ctx),
                       SECCOMP_PARM2(ctx),
                       SECCOMP_PARM3(ctx),
                       SECCOMP_PARM4(ctx),
                       SECCOMP_PARM5(ctx),
                       SECCOMP_PARM6(ctx),
                       SECCOMP_PARM7(ctx),
                       SECCOMP_PARM8(ctx));
#else
    rc = Syscall::Call(SECCOMP_SYSCALL(ctx),
                       SECCOMP_PARM1(ctx),
                       SECCOMP_PARM2(ctx),
                       SECCOMP_PARM3(ctx),
                       SECCOMP_PARM4(ctx),
                       SECCOMP_PARM5(ctx),
                       SECCOMP_PARM6(ctx));
#endif  
  } else {
    const TrapKey& trap = trap_array_[info->si_errno - 1];
    if (!trap.safe) {
      SetIsInSigHandler();
    }

    
    
    
    struct arch_seccomp_data data = {
        static_cast<int>(SECCOMP_SYSCALL(ctx)),
        SECCOMP_ARCH,
        reinterpret_cast<uint64_t>(sigsys.ip),
        {static_cast<uint64_t>(SECCOMP_PARM1(ctx)),
         static_cast<uint64_t>(SECCOMP_PARM2(ctx)),
         static_cast<uint64_t>(SECCOMP_PARM3(ctx)),
         static_cast<uint64_t>(SECCOMP_PARM4(ctx)),
         static_cast<uint64_t>(SECCOMP_PARM5(ctx)),
         static_cast<uint64_t>(SECCOMP_PARM6(ctx))}};

    
    
    rc = trap.fnc(data, const_cast<void*>(trap.aux));
  }

  
  
  
  Syscall::PutValueInUcontext(rc, ctx);
  errno = old_errno;

  return;
}

bool Trap::TrapKey::operator<(const TrapKey& o) const {
  if (fnc != o.fnc) {
    return fnc < o.fnc;
  } else if (aux != o.aux) {
    return aux < o.aux;
  } else {
    return safe < o.safe;
  }
}

uint16_t Trap::MakeTrap(TrapFnc fnc, const void* aux, bool safe) {
  return Registry()->Add(fnc, aux, safe);
}

uint16_t Trap::Add(TrapFnc fnc, const void* aux, bool safe) {
  if (!safe && !SandboxDebuggingAllowedByUser()) {
    
    
    
    
    

    
    
    
    
    
    SANDBOX_DIE(
        "Cannot use unsafe traps unless CHROME_SANDBOX_DEBUGGING "
        "is enabled");

    return 0;
  }

  
  
  TrapKey key(fnc, aux, safe);

  
  
  
  
  
  
  
  

  TrapIds::const_iterator iter = trap_ids_.find(key);
  if (iter != trap_ids_.end()) {
    
    
    return iter->second;
  }

  
  if (trap_array_size_ >= SECCOMP_RET_DATA  ||
      trap_array_size_ >= std::numeric_limits<uint16_t>::max()) {
    
    
    SANDBOX_DIE("Too many SECCOMP_RET_TRAP callback instances");
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (trap_array_size_ >= trap_array_capacity_) {
    trap_array_capacity_ += kCapacityIncrement;
    TrapKey* old_trap_array = trap_array_;
    TrapKey* new_trap_array = new TrapKey[trap_array_capacity_];
    std::copy_n(old_trap_array, trap_array_size_, new_trap_array);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    asm volatile("" : "=r"(new_trap_array) : "0"(new_trap_array) : "memory");
    trap_array_ = new_trap_array;
    asm volatile("" : "=r"(trap_array_) : "0"(trap_array_) : "memory");

    delete[] old_trap_array;
  }

  uint16_t id = trap_array_size_ + 1;
  trap_ids_[key] = id;
  trap_array_[trap_array_size_] = key;
  trap_array_size_++;
  return id;
}

bool Trap::SandboxDebuggingAllowedByUser() const {
  const char* debug_flag = getenv(kSandboxDebuggingEnv);
  return debug_flag && *debug_flag;
}

bool Trap::EnableUnsafeTrapsInSigSysHandler() {
  return Registry()->EnableUnsafeTraps();
}

bool Trap::EnableUnsafeTraps() {
  if (!has_unsafe_traps_) {
    
    
    
    
    
    if (SandboxDebuggingAllowedByUser()) {
      
      
      SANDBOX_INFO("WARNING! Disabling sandbox for debugging purposes");
      has_unsafe_traps_ = true;
    } else {
      SANDBOX_INFO(
          "Cannot disable sandbox and use unsafe traps unless "
          "CHROME_SANDBOX_DEBUGGING is turned on first");
    }
  }
  
  return has_unsafe_traps_;
}

Trap* Trap::global_trap_;

}  
