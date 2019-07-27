



#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#if defined(ANDROID)

#define __user
#endif
#include <linux/futex.h>

#include <ostream>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/posix/eintr_wrapper.h"
#include "build/build_config.h"
#include "sandbox/linux/seccomp-bpf/bpf_tests.h"
#include "sandbox/linux/seccomp-bpf/syscall.h"
#include "sandbox/linux/seccomp-bpf/trap.h"
#include "sandbox/linux/seccomp-bpf/verifier.h"
#include "sandbox/linux/services/broker_process.h"
#include "sandbox/linux/services/linux_syscalls.h"
#include "sandbox/linux/tests/scoped_temporary_file.h"
#include "sandbox/linux/tests/unit_tests.h"
#include "testing/gtest/include/gtest/gtest.h"


#ifndef PR_GET_ENDIAN
#define PR_GET_ENDIAN 19
#endif
#ifndef PR_CAPBSET_READ
#define PR_CAPBSET_READ 23
#define PR_CAPBSET_DROP 24
#endif

namespace sandbox {

namespace {

const int kExpectedReturnValue = 42;
const char kSandboxDebuggingEnv[] = "CHROME_SANDBOX_DEBUGGING";


void EnableUnsafeTraps() {
  
  
  
  
  
  setenv(kSandboxDebuggingEnv, "t", 0);
  Die::SuppressInfoMessages(true);
}



TEST(SandboxBPF, DISABLE_ON_TSAN(CallSupports)) {
  
  
  bool seccomp_bpf_supported =
      SandboxBPF::SupportsSeccompSandbox(-1) == SandboxBPF::STATUS_AVAILABLE;
  
  
  RecordProperty("SeccompBPFSupported",
                 seccomp_bpf_supported ? "true." : "false.");
  std::cout << "Seccomp BPF supported: "
            << (seccomp_bpf_supported ? "true." : "false.") << "\n";
  RecordProperty("PointerSize", sizeof(void*));
  std::cout << "Pointer size: " << sizeof(void*) << "\n";
}

SANDBOX_TEST(SandboxBPF, DISABLE_ON_TSAN(CallSupportsTwice)) {
  SandboxBPF::SupportsSeccompSandbox(-1);
  SandboxBPF::SupportsSeccompSandbox(-1);
}








intptr_t IncreaseCounter(const struct arch_seccomp_data& args, void* aux) {
  BPF_ASSERT(aux);
  int* counter = static_cast<int*>(aux);
  return (*counter)++;
}

class VerboseAPITestingPolicy : public SandboxBPFPolicy {
 public:
  VerboseAPITestingPolicy(int* counter_ptr) : counter_ptr_(counter_ptr) {}

  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE {
    DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
    if (sysno == __NR_uname) {
      return sandbox->Trap(IncreaseCounter, counter_ptr_);
    }
    return ErrorCode(ErrorCode::ERR_ALLOWED);
  }

 private:
  int* counter_ptr_;
  DISALLOW_COPY_AND_ASSIGN(VerboseAPITestingPolicy);
};

SANDBOX_TEST(SandboxBPF, DISABLE_ON_TSAN(VerboseAPITesting)) {
  if (SandboxBPF::SupportsSeccompSandbox(-1) ==
      sandbox::SandboxBPF::STATUS_AVAILABLE) {
    static int counter = 0;

    SandboxBPF sandbox;
    sandbox.SetSandboxPolicy(new VerboseAPITestingPolicy(&counter));
    BPF_ASSERT(sandbox.StartSandbox(SandboxBPF::PROCESS_SINGLE_THREADED));

    BPF_ASSERT_EQ(0, counter);
    BPF_ASSERT_EQ(0, syscall(__NR_uname, 0));
    BPF_ASSERT_EQ(1, counter);
    BPF_ASSERT_EQ(1, syscall(__NR_uname, 0));
    BPF_ASSERT_EQ(2, counter);
  }
}



class BlacklistNanosleepPolicy : public SandboxBPFPolicy {
 public:
  BlacklistNanosleepPolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF*, int sysno) const OVERRIDE {
    DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
    switch (sysno) {
      case __NR_nanosleep:
        return ErrorCode(EACCES);
      default:
        return ErrorCode(ErrorCode::ERR_ALLOWED);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BlacklistNanosleepPolicy);
};

BPF_TEST_C(SandboxBPF, ApplyBasicBlacklistPolicy, BlacklistNanosleepPolicy) {
  
  const struct timespec ts = {0, 0};
  errno = 0;
  BPF_ASSERT(syscall(__NR_nanosleep, &ts, NULL) == -1);
  BPF_ASSERT(errno == EACCES);
}



class WhitelistGetpidPolicy : public SandboxBPFPolicy {
 public:
  WhitelistGetpidPolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF*, int sysno) const OVERRIDE {
    DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
    switch (sysno) {
      case __NR_getpid:
      case __NR_exit_group:
        return ErrorCode(ErrorCode::ERR_ALLOWED);
      default:
        return ErrorCode(ENOMEM);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(WhitelistGetpidPolicy);
};

BPF_TEST_C(SandboxBPF, ApplyBasicWhitelistPolicy, WhitelistGetpidPolicy) {
  
  errno = 0;
  BPF_ASSERT(syscall(__NR_getpid) > 0);
  BPF_ASSERT(errno == 0);

  
  BPF_ASSERT(getpgid(0) == -1);
  BPF_ASSERT(errno == ENOMEM);
}


intptr_t EnomemHandler(const struct arch_seccomp_data& args, void* aux) {
  
  SANDBOX_ASSERT(aux);
  *(static_cast<int*>(aux)) = kExpectedReturnValue;
  return -ENOMEM;
}

ErrorCode BlacklistNanosleepPolicySigsys(SandboxBPF* sandbox,
                                         int sysno,
                                         int* aux) {
  DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
  switch (sysno) {
    case __NR_nanosleep:
      return sandbox->Trap(EnomemHandler, aux);
    default:
      return ErrorCode(ErrorCode::ERR_ALLOWED);
  }
}

BPF_TEST(SandboxBPF,
         BasicBlacklistWithSigsys,
         BlacklistNanosleepPolicySigsys,
         int ) {
  
  errno = 0;
  BPF_ASSERT(syscall(__NR_getpid) > 0);
  BPF_ASSERT(errno == 0);

  
  *BPF_AUX = -1;
  const struct timespec ts = {0, 0};
  BPF_ASSERT(syscall(__NR_nanosleep, &ts, NULL) == -1);
  BPF_ASSERT(errno == ENOMEM);

  
  BPF_ASSERT(*BPF_AUX == kExpectedReturnValue);
}



class ErrnoTestPolicy : public SandboxBPFPolicy {
 public:
  ErrnoTestPolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF*, int sysno) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(ErrnoTestPolicy);
};

ErrorCode ErrnoTestPolicy::EvaluateSyscall(SandboxBPF*, int sysno) const {
  DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
  switch (sysno) {
#if defined(ANDROID)
    case __NR_dup3:    
#else
    case __NR_dup2:
#endif
      
      return ErrorCode(0);
    case __NR_setuid:
#if defined(__NR_setuid32)
    case __NR_setuid32:
#endif
      
      return ErrorCode(1);
    case __NR_setgid:
#if defined(__NR_setgid32)
    case __NR_setgid32:
#endif
      
      return ErrorCode(ErrorCode::ERR_MAX_ERRNO);
    case __NR_uname:
      
      return ErrorCode(42);
    default:
      return ErrorCode(ErrorCode::ERR_ALLOWED);
  }
}

BPF_TEST_C(SandboxBPF, ErrnoTest, ErrnoTestPolicy) {
  
  int fds[4];
  BPF_ASSERT(pipe(fds) == 0);
  BPF_ASSERT(pipe(fds + 2) == 0);
  BPF_ASSERT(dup2(fds[2], fds[0]) == 0);
  char buf[1] = {};
  BPF_ASSERT(write(fds[1], "\x55", 1) == 1);
  BPF_ASSERT(write(fds[3], "\xAA", 1) == 1);
  BPF_ASSERT(read(fds[0], buf, 1) == 1);

  
  
  BPF_ASSERT(buf[0] == '\x55');

  
  errno = 0;
  BPF_ASSERT(setuid(0) == -1);
  BPF_ASSERT(errno == 1);

  
  
  
  if (sandbox::IsAndroid() && setgid(0) != -1) {
    errno = 0;
    BPF_ASSERT(setgid(0) == -ErrorCode::ERR_MAX_ERRNO);
    BPF_ASSERT(errno == 0);
  } else {
    errno = 0;
    BPF_ASSERT(setgid(0) == -1);
    BPF_ASSERT(errno == ErrorCode::ERR_MAX_ERRNO);
  }

  
  errno = 0;
  struct utsname uts_buf;
  BPF_ASSERT(uname(&uts_buf) == -1);
  BPF_ASSERT(errno == 42);
}



class StackingPolicyPartOne : public SandboxBPFPolicy {
 public:
  StackingPolicyPartOne() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE {
    DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
    switch (sysno) {
      case __NR_getppid:
        return sandbox->Cond(0,
                             ErrorCode::TP_32BIT,
                             ErrorCode::OP_EQUAL,
                             0,
                             ErrorCode(ErrorCode::ERR_ALLOWED),
                             ErrorCode(EPERM));
      default:
        return ErrorCode(ErrorCode::ERR_ALLOWED);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(StackingPolicyPartOne);
};

class StackingPolicyPartTwo : public SandboxBPFPolicy {
 public:
  StackingPolicyPartTwo() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE {
    DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
    switch (sysno) {
      case __NR_getppid:
        return sandbox->Cond(0,
                             ErrorCode::TP_32BIT,
                             ErrorCode::OP_EQUAL,
                             0,
                             ErrorCode(EINVAL),
                             ErrorCode(ErrorCode::ERR_ALLOWED));
      default:
        return ErrorCode(ErrorCode::ERR_ALLOWED);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(StackingPolicyPartTwo);
};

BPF_TEST_C(SandboxBPF, StackingPolicy, StackingPolicyPartOne) {
  errno = 0;
  BPF_ASSERT(syscall(__NR_getppid, 0) > 0);
  BPF_ASSERT(errno == 0);

  BPF_ASSERT(syscall(__NR_getppid, 1) == -1);
  BPF_ASSERT(errno == EPERM);

  
  
  SandboxBPF sandbox;
  sandbox.SetSandboxPolicy(new StackingPolicyPartTwo());
  BPF_ASSERT(sandbox.StartSandbox(SandboxBPF::PROCESS_SINGLE_THREADED));

  errno = 0;
  BPF_ASSERT(syscall(__NR_getppid, 0) == -1);
  BPF_ASSERT(errno == EINVAL);

  BPF_ASSERT(syscall(__NR_getppid, 1) == -1);
  BPF_ASSERT(errno == EPERM);
}










int SysnoToRandomErrno(int sysno) {
  
  
  return ((sysno & ~3) >> 2) % 29 + 1;
}

class SyntheticPolicy : public SandboxBPFPolicy {
 public:
  SyntheticPolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF*, int sysno) const OVERRIDE {
    DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
    if (sysno == __NR_exit_group || sysno == __NR_write) {
      
      
      return ErrorCode(ErrorCode::ERR_ALLOWED);
    }
    return ErrorCode(SysnoToRandomErrno(sysno));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SyntheticPolicy);
};

BPF_TEST_C(SandboxBPF, SyntheticPolicy, SyntheticPolicy) {
  
  
  BPF_ASSERT(std::numeric_limits<int>::max() - kExpectedReturnValue - 1 >=
             static_cast<int>(MAX_PUBLIC_SYSCALL));

  for (int syscall_number = static_cast<int>(MIN_SYSCALL);
       syscall_number <= static_cast<int>(MAX_PUBLIC_SYSCALL);
       ++syscall_number) {
    if (syscall_number == __NR_exit_group || syscall_number == __NR_write) {
      
      continue;
    }
    errno = 0;
    BPF_ASSERT(syscall(syscall_number) == -1);
    BPF_ASSERT(errno == SysnoToRandomErrno(syscall_number));
  }
}

#if defined(__arm__)





int ArmPrivateSysnoToErrno(int sysno) {
  if (sysno >= static_cast<int>(MIN_PRIVATE_SYSCALL) &&
      sysno <= static_cast<int>(MAX_PRIVATE_SYSCALL)) {
    return (sysno - MIN_PRIVATE_SYSCALL) + 1;
  } else {
    return ENOSYS;
  }
}

class ArmPrivatePolicy : public SandboxBPFPolicy {
 public:
  ArmPrivatePolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF*, int sysno) const OVERRIDE {
    DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
    
    
    if (sysno >= static_cast<int>(__ARM_NR_set_tls + 1) &&
        sysno <= static_cast<int>(MAX_PRIVATE_SYSCALL)) {
      return ErrorCode(ArmPrivateSysnoToErrno(sysno));
    }
    return ErrorCode(ErrorCode::ERR_ALLOWED);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArmPrivatePolicy);
};

BPF_TEST_C(SandboxBPF, ArmPrivatePolicy, ArmPrivatePolicy) {
  for (int syscall_number = static_cast<int>(__ARM_NR_set_tls + 1);
       syscall_number <= static_cast<int>(MAX_PRIVATE_SYSCALL);
       ++syscall_number) {
    errno = 0;
    BPF_ASSERT(syscall(syscall_number) == -1);
    BPF_ASSERT(errno == ArmPrivateSysnoToErrno(syscall_number));
  }
}
#endif  

intptr_t CountSyscalls(const struct arch_seccomp_data& args, void* aux) {
  
  ++*reinterpret_cast<int*>(aux);

  
  
  BPF_ASSERT(syscall(__NR_getpid) > 1);

  
  
  return SandboxBPF::ForwardSyscall(args);
}

ErrorCode GreyListedPolicy(SandboxBPF* sandbox, int sysno, int* aux) {
  
  if (sysno == MIN_SYSCALL) {
    EnableUnsafeTraps();
  }

  
  
  if (sysno == __NR_rt_sigprocmask || sysno == __NR_rt_sigreturn
#if defined(__NR_sigprocmask)
      ||
      sysno == __NR_sigprocmask
#endif
#if defined(__NR_sigreturn)
      ||
      sysno == __NR_sigreturn
#endif
      ) {
    return ErrorCode(ErrorCode::ERR_ALLOWED);
  } else if (sysno == __NR_getpid) {
    
    return ErrorCode(EPERM);
  } else if (SandboxBPF::IsValidSyscallNumber(sysno)) {
    
    return sandbox->UnsafeTrap(CountSyscalls, aux);
  } else {
    return ErrorCode(ENOSYS);
  }
}

BPF_TEST(SandboxBPF, GreyListedPolicy, GreyListedPolicy, int ) {
  BPF_ASSERT(syscall(__NR_getpid) == -1);
  BPF_ASSERT(errno == EPERM);
  BPF_ASSERT(*BPF_AUX == 0);
  BPF_ASSERT(syscall(__NR_geteuid) == syscall(__NR_getuid));
  BPF_ASSERT(*BPF_AUX == 2);
  char name[17] = {};
  BPF_ASSERT(!syscall(__NR_prctl,
                      PR_GET_NAME,
                      name,
                      (void*)NULL,
                      (void*)NULL,
                      (void*)NULL));
  BPF_ASSERT(*BPF_AUX == 3);
  BPF_ASSERT(*name);
}

SANDBOX_TEST(SandboxBPF, EnableUnsafeTrapsInSigSysHandler) {
  
  setenv(kSandboxDebuggingEnv, "t", 0);
  Die::SuppressInfoMessages(true);

  unsetenv(kSandboxDebuggingEnv);
  SANDBOX_ASSERT(Trap::EnableUnsafeTrapsInSigSysHandler() == false);
  setenv(kSandboxDebuggingEnv, "", 1);
  SANDBOX_ASSERT(Trap::EnableUnsafeTrapsInSigSysHandler() == false);
  setenv(kSandboxDebuggingEnv, "t", 1);
  SANDBOX_ASSERT(Trap::EnableUnsafeTrapsInSigSysHandler() == true);
}

intptr_t PrctlHandler(const struct arch_seccomp_data& args, void*) {
  if (args.args[0] == PR_CAPBSET_DROP && static_cast<int>(args.args[1]) == -1) {
    
    
    return 0;
  } else {
    return SandboxBPF::ForwardSyscall(args);
  }
}

class PrctlPolicy : public SandboxBPFPolicy {
 public:
  PrctlPolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE {
    DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
    setenv(kSandboxDebuggingEnv, "t", 0);
    Die::SuppressInfoMessages(true);

    if (sysno == __NR_prctl) {
      
      return sandbox->UnsafeTrap(PrctlHandler, NULL);
    }

    
    return ErrorCode(ErrorCode::ERR_ALLOWED);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PrctlPolicy);
};

BPF_TEST_C(SandboxBPF, ForwardSyscall, PrctlPolicy) {
  
  
  BPF_ASSERT(
      !prctl(PR_CAPBSET_DROP, -1, (void*)NULL, (void*)NULL, (void*)NULL));

  
  BPF_ASSERT(
      prctl(PR_CAPBSET_DROP, -2, (void*)NULL, (void*)NULL, (void*)NULL) == -1);

  
  char name[17] = {};
  BPF_ASSERT(!syscall(__NR_prctl,
                      PR_GET_NAME,
                      name,
                      (void*)NULL,
                      (void*)NULL,
                      (void*)NULL));
  BPF_ASSERT(*name);

  
  
  struct utsname uts = {};
  BPF_ASSERT(!uname(&uts));
  BPF_ASSERT(!strcmp(uts.sysname, "Linux"));
}

intptr_t AllowRedirectedSyscall(const struct arch_seccomp_data& args, void*) {
  return SandboxBPF::ForwardSyscall(args);
}

class RedirectAllSyscallsPolicy : public SandboxBPFPolicy {
 public:
  RedirectAllSyscallsPolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(RedirectAllSyscallsPolicy);
};

ErrorCode RedirectAllSyscallsPolicy::EvaluateSyscall(SandboxBPF* sandbox,
                                                     int sysno) const {
  DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
  setenv(kSandboxDebuggingEnv, "t", 0);
  Die::SuppressInfoMessages(true);

  
  
  if (sysno == __NR_rt_sigprocmask || sysno == __NR_rt_sigreturn
#if defined(__NR_sigprocmask)
      ||
      sysno == __NR_sigprocmask
#endif
#if defined(__NR_sigreturn)
      ||
      sysno == __NR_sigreturn
#endif
      ) {
    return ErrorCode(ErrorCode::ERR_ALLOWED);
  }
  return sandbox->UnsafeTrap(AllowRedirectedSyscall, NULL);
}

int bus_handler_fd_ = -1;

void SigBusHandler(int, siginfo_t* info, void* void_context) {
  BPF_ASSERT(write(bus_handler_fd_, "\x55", 1) == 1);
}

BPF_TEST_C(SandboxBPF, SigBus, RedirectAllSyscallsPolicy) {
  
  
  
  
  
  
  
  int fds[2];
  BPF_ASSERT(pipe(fds) == 0);
  bus_handler_fd_ = fds[1];
  struct sigaction sa = {};
  sa.sa_sigaction = SigBusHandler;
  sa.sa_flags = SA_SIGINFO;
  BPF_ASSERT(sigaction(SIGBUS, &sa, NULL) == 0);
  raise(SIGBUS);
  char c = '\000';
  BPF_ASSERT(read(fds[0], &c, 1) == 1);
  BPF_ASSERT(close(fds[0]) == 0);
  BPF_ASSERT(close(fds[1]) == 0);
  BPF_ASSERT(c == 0x55);
}

BPF_TEST_C(SandboxBPF, SigMask, RedirectAllSyscallsPolicy) {
  
  
  
  
  
  
  
  sigset_t mask0, mask1, mask2;

  
  
  
  
  
  sigemptyset(&mask0);
  BPF_ASSERT(!sigprocmask(SIG_BLOCK, &mask0, &mask1));
  BPF_ASSERT(!sigismember(&mask1, SIGUSR2));

  
  
  sigaddset(&mask0, SIGUSR2);
  BPF_ASSERT(!sigprocmask(SIG_BLOCK, &mask0, NULL));
  BPF_ASSERT(!sigprocmask(SIG_BLOCK, NULL, &mask2));
  BPF_ASSERT(sigismember(&mask2, SIGUSR2));
}

BPF_TEST_C(SandboxBPF, UnsafeTrapWithErrno, RedirectAllSyscallsPolicy) {
  
  
  
  
  
  

  
  
  errno = 0;
  BPF_ASSERT(close(-1) == -1);
  BPF_ASSERT(errno == EBADF);

  
  
  
  errno = 0;
  struct arch_seccomp_data args = {};
  args.nr = __NR_close;
  args.args[0] = -1;
  BPF_ASSERT(SandboxBPF::ForwardSyscall(args) == -EBADF);
  BPF_ASSERT(errno == 0);
}

bool NoOpCallback() { return true; }



class InitializedOpenBroker {
 public:
  InitializedOpenBroker() : initialized_(false) {
    std::vector<std::string> allowed_files;
    allowed_files.push_back("/proc/allowed");
    allowed_files.push_back("/proc/cpuinfo");

    broker_process_.reset(
        new BrokerProcess(EPERM, allowed_files, std::vector<std::string>()));
    BPF_ASSERT(broker_process() != NULL);
    BPF_ASSERT(broker_process_->Init(base::Bind(&NoOpCallback)));

    initialized_ = true;
  }
  bool initialized() { return initialized_; }
  class BrokerProcess* broker_process() { return broker_process_.get(); }

 private:
  bool initialized_;
  scoped_ptr<class BrokerProcess> broker_process_;
  DISALLOW_COPY_AND_ASSIGN(InitializedOpenBroker);
};

intptr_t BrokerOpenTrapHandler(const struct arch_seccomp_data& args,
                               void* aux) {
  BPF_ASSERT(aux);
  BrokerProcess* broker_process = static_cast<BrokerProcess*>(aux);
  switch (args.nr) {
#if defined(ANDROID)
    case __NR_faccessat:    
      return broker_process->Access(reinterpret_cast<const char*>(args.args[1]),
                                    static_cast<int>(args.args[2]));
#else
    case __NR_access:
      return broker_process->Access(reinterpret_cast<const char*>(args.args[0]),
                                    static_cast<int>(args.args[1]));
#endif
    case __NR_open:
      return broker_process->Open(reinterpret_cast<const char*>(args.args[0]),
                                  static_cast<int>(args.args[1]));
    case __NR_openat:
      
      
      BPF_ASSERT(static_cast<int>(args.args[0]) == AT_FDCWD);
      return broker_process->Open(reinterpret_cast<const char*>(args.args[1]),
                                  static_cast<int>(args.args[2]));
    default:
      BPF_ASSERT(false);
      return -ENOSYS;
  }
}

ErrorCode DenyOpenPolicy(SandboxBPF* sandbox,
                         int sysno,
                         InitializedOpenBroker* iob) {
  if (!SandboxBPF::IsValidSyscallNumber(sysno)) {
    return ErrorCode(ENOSYS);
  }

  switch (sysno) {
#if defined(ANDROID)
    case __NR_faccessat:
#else
    case __NR_access:
#endif
    case __NR_open:
    case __NR_openat:
      
      
      return ErrorCode(
          sandbox->Trap(BrokerOpenTrapHandler, iob->broker_process()));
    default:
      return ErrorCode(ErrorCode::ERR_ALLOWED);
  }
}



BPF_TEST(SandboxBPF,
         UseOpenBroker,
         DenyOpenPolicy,
         InitializedOpenBroker ) {
  BPF_ASSERT(BPF_AUX->initialized());
  BrokerProcess* broker_process = BPF_AUX->broker_process();
  BPF_ASSERT(broker_process != NULL);

  
  BPF_ASSERT(broker_process->Open("/proc/denied", O_RDONLY) == -EPERM);
  BPF_ASSERT(broker_process->Access("/proc/denied", R_OK) == -EPERM);
  BPF_ASSERT(broker_process->Open("/proc/allowed", O_RDONLY) == -ENOENT);
  BPF_ASSERT(broker_process->Access("/proc/allowed", R_OK) == -ENOENT);

  
  BPF_ASSERT(open("/proc/denied", O_RDONLY) == -1);
  BPF_ASSERT(errno == EPERM);

  BPF_ASSERT(open("/proc/allowed", O_RDONLY) == -1);
  BPF_ASSERT(errno == ENOENT);

  
  
  BPF_ASSERT(openat(AT_FDCWD, "/proc/denied", O_RDONLY) == -1);
  BPF_ASSERT(errno == EPERM);

  BPF_ASSERT(openat(AT_FDCWD, "/proc/allowed", O_RDONLY) == -1);
  BPF_ASSERT(errno == ENOENT);

  
  BPF_ASSERT(access("/proc/denied", R_OK) == -1);
  BPF_ASSERT(errno == EPERM);

  BPF_ASSERT(access("/proc/allowed", R_OK) == -1);
  BPF_ASSERT(errno == ENOENT);

  
  int cpu_info_access = access("/proc/cpuinfo", R_OK);
  BPF_ASSERT(cpu_info_access == 0);
  int cpu_info_fd = open("/proc/cpuinfo", O_RDONLY);
  BPF_ASSERT(cpu_info_fd >= 0);
  char buf[1024];
  BPF_ASSERT(read(cpu_info_fd, buf, sizeof(buf)) > 0);
}



class SimpleCondTestPolicy : public SandboxBPFPolicy {
 public:
  SimpleCondTestPolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(SimpleCondTestPolicy);
};

ErrorCode SimpleCondTestPolicy::EvaluateSyscall(SandboxBPF* sandbox,
                                                int sysno) const {
  DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));

  
  
  
  switch (sysno) {
#if defined(ANDROID)
    case __NR_openat:    
      
      COMPILE_ASSERT(O_RDONLY == 0, O_RDONLY_must_be_all_zero_bits);
      return sandbox->Cond(2,
                           ErrorCode::TP_32BIT,
                           ErrorCode::OP_HAS_ANY_BITS,
                           O_ACCMODE ,
                           ErrorCode(EROFS),
                           ErrorCode(ErrorCode::ERR_ALLOWED));
#else
    case __NR_open:
      
      COMPILE_ASSERT(O_RDONLY == 0, O_RDONLY_must_be_all_zero_bits);
      return sandbox->Cond(1,
                           ErrorCode::TP_32BIT,
                           ErrorCode::OP_HAS_ANY_BITS,
                           O_ACCMODE ,
                           ErrorCode(EROFS),
                           ErrorCode(ErrorCode::ERR_ALLOWED));
#endif
    case __NR_prctl:
      
      
      return sandbox->Cond(0,
                           ErrorCode::TP_32BIT,
                           ErrorCode::OP_EQUAL,
                           PR_SET_DUMPABLE,
                           ErrorCode(ErrorCode::ERR_ALLOWED),
                           sandbox->Cond(0,
                                         ErrorCode::TP_32BIT,
                                         ErrorCode::OP_EQUAL,
                                         PR_GET_DUMPABLE,
                                         ErrorCode(ErrorCode::ERR_ALLOWED),
                                         ErrorCode(ENOMEM)));
    default:
      return ErrorCode(ErrorCode::ERR_ALLOWED);
  }
}

BPF_TEST_C(SandboxBPF, SimpleCondTest, SimpleCondTestPolicy) {
  int fd;
  BPF_ASSERT((fd = open("/proc/self/comm", O_RDWR)) == -1);
  BPF_ASSERT(errno == EROFS);
  BPF_ASSERT((fd = open("/proc/self/comm", O_RDONLY)) >= 0);
  close(fd);

  int ret;
  BPF_ASSERT((ret = prctl(PR_GET_DUMPABLE)) >= 0);
  BPF_ASSERT(prctl(PR_SET_DUMPABLE, 1 - ret) == 0);
  BPF_ASSERT(prctl(PR_GET_ENDIAN, &ret) == -1);
  BPF_ASSERT(errno == ENOMEM);
}





class EqualityStressTest {
 public:
  EqualityStressTest() {
    
    srand(0);

    
    
    
    
    
    COMPILE_ASSERT(
        kNumTestCases < (int)(MAX_PUBLIC_SYSCALL - MIN_SYSCALL - 10),
        num_test_cases_must_be_significantly_smaller_than_num_system_calls);
    for (int sysno = MIN_SYSCALL, end = kNumTestCases; sysno < end; ++sysno) {
      if (IsReservedSyscall(sysno)) {
        
        
        
        ++end;
        arg_values_.push_back(NULL);
      } else {
        arg_values_.push_back(
            RandomArgValue(rand() % kMaxArgs, 0, rand() % kMaxArgs));
      }
    }
  }

  ~EqualityStressTest() {
    for (std::vector<ArgValue*>::iterator iter = arg_values_.begin();
         iter != arg_values_.end();
         ++iter) {
      DeleteArgValue(*iter);
    }
  }

  ErrorCode Policy(SandboxBPF* sandbox, int sysno) {
    if (!SandboxBPF::IsValidSyscallNumber(sysno)) {
      
      return ErrorCode(ENOSYS);
    } else if (sysno < 0 || sysno >= (int)arg_values_.size() ||
               IsReservedSyscall(sysno)) {
      
      
      
      return ErrorCode(ErrorCode::ERR_ALLOWED);
    } else {
      
      
      return ToErrorCode(sandbox, arg_values_[sysno]);
    }
  }

  void VerifyFilter() {
    
    
    for (int sysno = 0; sysno < (int)arg_values_.size(); ++sysno) {
      if (!arg_values_[sysno]) {
        
        continue;
      }
      
      
      
      
      
      
      
      intptr_t args[6] = {};
      Verify(sysno, args, *arg_values_[sysno]);
    }
  }

 private:
  struct ArgValue {
    int argno;  
    int size;   
    struct Tests {
      uint32_t k_value;            
      int err;                     
      struct ArgValue* arg_value;  
    }* tests;
    int err;                     
    struct ArgValue* arg_value;  
  };

  bool IsReservedSyscall(int sysno) {
    
    
    
    
    
    
    
    
    
    
    return sysno == __NR_read || sysno == __NR_write || sysno == __NR_exit ||
           sysno == __NR_exit_group || sysno == __NR_restart_syscall;
  }

  ArgValue* RandomArgValue(int argno, int args_mask, int remaining_args) {
    
    
    
    
    struct ArgValue* arg_value = new ArgValue();
    args_mask |= 1 << argno;
    arg_value->argno = argno;

    
    
    
    int fan_out = kMaxFanOut;
    if (remaining_args > 3) {
      fan_out = 1;
    } else if (remaining_args > 2) {
      fan_out = 2;
    }

    
    
    arg_value->size = rand() % fan_out + 1;
    arg_value->tests = new ArgValue::Tests[arg_value->size];

    uint32_t k_value = rand();
    for (int n = 0; n < arg_value->size; ++n) {
      
      k_value += rand() % (RAND_MAX / (kMaxFanOut + 1)) + 1;

      
      
      
      
      
      
      
      arg_value->tests[n].k_value = k_value;
      if (!remaining_args || (rand() & 1)) {
        arg_value->tests[n].err = (rand() % 1000) + 1;
        arg_value->tests[n].arg_value = NULL;
      } else {
        arg_value->tests[n].err = 0;
        arg_value->tests[n].arg_value =
            RandomArgValue(RandomArg(args_mask), args_mask, remaining_args - 1);
      }
    }
    
    
    
    if (!remaining_args || (rand() & 1)) {
      arg_value->err = (rand() % 1000) + 1;
      arg_value->arg_value = NULL;
    } else {
      arg_value->err = 0;
      arg_value->arg_value =
          RandomArgValue(RandomArg(args_mask), args_mask, remaining_args - 1);
    }
    
    
    
    return arg_value;
  }

  int RandomArg(int args_mask) {
    
    int argno = rand() % kMaxArgs;

    
    
    
    while (args_mask & (1 << argno)) {
      argno = (argno + 1) % kMaxArgs;
    }
    return argno;
  }

  void DeleteArgValue(ArgValue* arg_value) {
    
    
    if (arg_value) {
      if (arg_value->size) {
        for (int n = 0; n < arg_value->size; ++n) {
          if (!arg_value->tests[n].err) {
            DeleteArgValue(arg_value->tests[n].arg_value);
          }
        }
        delete[] arg_value->tests;
      }
      if (!arg_value->err) {
        DeleteArgValue(arg_value->arg_value);
      }
      delete arg_value;
    }
  }

  ErrorCode ToErrorCode(SandboxBPF* sandbox, ArgValue* arg_value) {
    
    
    
    ErrorCode err;
    if (arg_value->err) {
      
      
      err = ErrorCode(arg_value->err);
    } else {
      
      
      
      err = ToErrorCode(sandbox, arg_value->arg_value);
    }

    
    
    
    for (int n = arg_value->size; n-- > 0;) {
      ErrorCode matched;
      
      if (arg_value->tests[n].err) {
        matched = ErrorCode(arg_value->tests[n].err);
      } else {
        matched = ToErrorCode(sandbox, arg_value->tests[n].arg_value);
      }
      
      
      
      err = sandbox->Cond(arg_value->argno,
                          ErrorCode::TP_32BIT,
                          ErrorCode::OP_EQUAL,
                          arg_value->tests[n].k_value,
                          matched,
                          err);
    }
    return err;
  }

  void Verify(int sysno, intptr_t* args, const ArgValue& arg_value) {
    uint32_t mismatched = 0;
    
    
    
    for (int n = arg_value.size; n-- > 0;) {
      mismatched += arg_value.tests[n].k_value;
      args[arg_value.argno] = arg_value.tests[n].k_value;
      if (arg_value.tests[n].err) {
        VerifyErrno(sysno, args, arg_value.tests[n].err);
      } else {
        Verify(sysno, args, *arg_value.tests[n].arg_value);
      }
    }
  
  
  
  
  try_again:
    for (int n = arg_value.size; n-- > 0;) {
      if (mismatched == arg_value.tests[n].k_value) {
        ++mismatched;
        goto try_again;
      }
    }
    
    
    
    args[arg_value.argno] = mismatched;
    if (arg_value.err) {
      VerifyErrno(sysno, args, arg_value.err);
    } else {
      Verify(sysno, args, *arg_value.arg_value);
    }
    
    
    args[arg_value.argno] = 0;
  }

  void VerifyErrno(int sysno, intptr_t* args, int err) {
    
    
    
    BPF_ASSERT(
        Syscall::Call(
            sysno, args[0], args[1], args[2], args[3], args[4], args[5]) ==
        -err);
  }

  
  
  std::vector<ArgValue*> arg_values_;

  
  
  
  static const int kNumTestCases = 40;
  static const int kMaxFanOut = 3;
  static const int kMaxArgs = 6;
};

ErrorCode EqualityStressTestPolicy(SandboxBPF* sandbox,
                                   int sysno,
                                   EqualityStressTest* aux) {
  DCHECK(aux);
  return aux->Policy(sandbox, sysno);
}

BPF_TEST(SandboxBPF,
         EqualityTests,
         EqualityStressTestPolicy,
         EqualityStressTest ) {
  BPF_AUX->VerifyFilter();
}

class EqualityArgumentWidthPolicy : public SandboxBPFPolicy {
 public:
  EqualityArgumentWidthPolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(EqualityArgumentWidthPolicy);
};

ErrorCode EqualityArgumentWidthPolicy::EvaluateSyscall(SandboxBPF* sandbox,
                                                       int sysno) const {
  DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
  if (sysno == __NR_uname) {
    return sandbox->Cond(
        0,
        ErrorCode::TP_32BIT,
        ErrorCode::OP_EQUAL,
        0,
        sandbox->Cond(1,
                      ErrorCode::TP_32BIT,
                      ErrorCode::OP_EQUAL,
                      0x55555555,
                      ErrorCode(1),
                      ErrorCode(2)),
        
        
        
        
        
        
        
        
        sandbox->Cond(1,
                      ErrorCode::TP_64BIT,
                      ErrorCode::OP_EQUAL,
                      0x55555555AAAAAAAAULL,
                      ErrorCode(1),
                      ErrorCode(2)));
  }
  return ErrorCode(ErrorCode::ERR_ALLOWED);
}

BPF_TEST_C(SandboxBPF, EqualityArgumentWidth, EqualityArgumentWidthPolicy) {
  BPF_ASSERT(Syscall::Call(__NR_uname, 0, 0x55555555) == -1);
  BPF_ASSERT(Syscall::Call(__NR_uname, 0, 0xAAAAAAAA) == -2);
#if __SIZEOF_POINTER__ > 4
  
  
  
  BPF_ASSERT(Syscall::Call(__NR_uname, 1, 0x55555555AAAAAAAAULL) == -1);
  BPF_ASSERT(Syscall::Call(__NR_uname, 1, 0x5555555500000000ULL) == -2);
  BPF_ASSERT(Syscall::Call(__NR_uname, 1, 0x5555555511111111ULL) == -2);
  BPF_ASSERT(Syscall::Call(__NR_uname, 1, 0x11111111AAAAAAAAULL) == -2);
#else
  BPF_ASSERT(Syscall::Call(__NR_uname, 1, 0x55555555) == -2);
#endif
}

#if __SIZEOF_POINTER__ > 4



BPF_DEATH_TEST_C(SandboxBPF,
                 EqualityArgumentUnallowed64bit,
                 DEATH_MESSAGE("Unexpected 64bit argument detected"),
                 EqualityArgumentWidthPolicy) {
  Syscall::Call(__NR_uname, 0, 0x5555555555555555ULL);
}
#endif

class EqualityWithNegativeArgumentsPolicy : public SandboxBPFPolicy {
 public:
  EqualityWithNegativeArgumentsPolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE {
    DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
    if (sysno == __NR_uname) {
      return sandbox->Cond(0,
                           ErrorCode::TP_32BIT,
                           ErrorCode::OP_EQUAL,
                           0xFFFFFFFF,
                           ErrorCode(1),
                           ErrorCode(2));
    }
    return ErrorCode(ErrorCode::ERR_ALLOWED);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(EqualityWithNegativeArgumentsPolicy);
};

BPF_TEST_C(SandboxBPF,
           EqualityWithNegativeArguments,
           EqualityWithNegativeArgumentsPolicy) {
  BPF_ASSERT(Syscall::Call(__NR_uname, 0xFFFFFFFF) == -1);
  BPF_ASSERT(Syscall::Call(__NR_uname, -1) == -1);
  BPF_ASSERT(Syscall::Call(__NR_uname, -1LL) == -1);
}

#if __SIZEOF_POINTER__ > 4
BPF_DEATH_TEST_C(SandboxBPF,
                 EqualityWithNegative64bitArguments,
                 DEATH_MESSAGE("Unexpected 64bit argument detected"),
                 EqualityWithNegativeArgumentsPolicy) {
  
  
  
  BPF_ASSERT(Syscall::Call(__NR_uname, 0xFFFFFFFF00000000LL) == -1);
}
#endif
class AllBitTestPolicy : public SandboxBPFPolicy {
 public:
  AllBitTestPolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(AllBitTestPolicy);
};

ErrorCode AllBitTestPolicy::EvaluateSyscall(SandboxBPF* sandbox,
                                            int sysno) const {
  DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
  
  
  
  
  
  
  if (sysno == __NR_uname) {
    return sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 0,
           sandbox->Cond(1, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ALL_BITS,
                         0x0,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 1,
           sandbox->Cond(1, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ALL_BITS,
                         0x1,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 2,
           sandbox->Cond(1, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ALL_BITS,
                         0x3,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 3,
           sandbox->Cond(1, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ALL_BITS,
                         0x80000000,
                         ErrorCode(1), ErrorCode(0)),
           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 4,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ALL_BITS,
                         0x0,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 5,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ALL_BITS,
                         0x1,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 6,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ALL_BITS,
                         0x3,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 7,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ALL_BITS,
                         0x80000000,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 8,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ALL_BITS,
                         0x100000000ULL,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 9,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ALL_BITS,
                         0x300000000ULL,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 10,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ALL_BITS,
                         0x100000001ULL,
                         ErrorCode(1), ErrorCode(0)),

                         sandbox->Kill("Invalid test case number"))))))))))));
  }
  return ErrorCode(ErrorCode::ERR_ALLOWED);
}








#define BITMASK_TEST(testcase, arg, op, mask, expected_value) \
  BPF_ASSERT(Syscall::Call(__NR_uname, (testcase), (arg)) == (expected_value))




#define EXPECT_FAILURE 0
#define EXPECT_SUCCESS -1






#define EXPT64_SUCCESS (sizeof(void*) > 4 ? EXPECT_SUCCESS : EXPECT_FAILURE)
BPF_TEST_C(SandboxBPF, AllBitTests, AllBitTestPolicy) {
  
  BITMASK_TEST( 0,                   0, ALLBITS32,          0, EXPECT_SUCCESS);
  BITMASK_TEST( 0,                   1, ALLBITS32,          0, EXPECT_SUCCESS);
  BITMASK_TEST( 0,                   3, ALLBITS32,          0, EXPECT_SUCCESS);
  BITMASK_TEST( 0,         0xFFFFFFFFU, ALLBITS32,          0, EXPECT_SUCCESS);
  BITMASK_TEST( 0,                -1LL, ALLBITS32,          0, EXPECT_SUCCESS);

  
  BITMASK_TEST( 1,                   0, ALLBITS32,        0x1, EXPECT_FAILURE);
  BITMASK_TEST( 1,                   1, ALLBITS32,        0x1, EXPECT_SUCCESS);
  BITMASK_TEST( 1,                   2, ALLBITS32,        0x1, EXPECT_FAILURE);
  BITMASK_TEST( 1,                   3, ALLBITS32,        0x1, EXPECT_SUCCESS);

  
  BITMASK_TEST( 2,                   0, ALLBITS32,        0x3, EXPECT_FAILURE);
  BITMASK_TEST( 2,                   1, ALLBITS32,        0x3, EXPECT_FAILURE);
  BITMASK_TEST( 2,                   2, ALLBITS32,        0x3, EXPECT_FAILURE);
  BITMASK_TEST( 2,                   3, ALLBITS32,        0x3, EXPECT_SUCCESS);
  BITMASK_TEST( 2,                   7, ALLBITS32,        0x3, EXPECT_SUCCESS);

  
  BITMASK_TEST( 3,                   0, ALLBITS32, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 3,         0x40000000U, ALLBITS32, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 3,         0x80000000U, ALLBITS32, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 3,         0xC0000000U, ALLBITS32, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 3,       -0x80000000LL, ALLBITS32, 0x80000000, EXPECT_SUCCESS);

  
  BITMASK_TEST( 4,                   0, ALLBITS64,          0, EXPECT_SUCCESS);
  BITMASK_TEST( 4,                   1, ALLBITS64,          0, EXPECT_SUCCESS);
  BITMASK_TEST( 4,                   3, ALLBITS64,          0, EXPECT_SUCCESS);
  BITMASK_TEST( 4,         0xFFFFFFFFU, ALLBITS64,          0, EXPECT_SUCCESS);
  BITMASK_TEST( 4,       0x100000000LL, ALLBITS64,          0, EXPECT_SUCCESS);
  BITMASK_TEST( 4,       0x300000000LL, ALLBITS64,          0, EXPECT_SUCCESS);
  BITMASK_TEST( 4,0x8000000000000000LL, ALLBITS64,          0, EXPECT_SUCCESS);
  BITMASK_TEST( 4,                -1LL, ALLBITS64,          0, EXPECT_SUCCESS);

  
  BITMASK_TEST( 5,                   0, ALLBITS64,          1, EXPECT_FAILURE);
  BITMASK_TEST( 5,                   1, ALLBITS64,          1, EXPECT_SUCCESS);
  BITMASK_TEST( 5,                   2, ALLBITS64,          1, EXPECT_FAILURE);
  BITMASK_TEST( 5,                   3, ALLBITS64,          1, EXPECT_SUCCESS);
  BITMASK_TEST( 5,       0x100000000LL, ALLBITS64,          1, EXPECT_FAILURE);
  BITMASK_TEST( 5,       0x100000001LL, ALLBITS64,          1, EXPECT_SUCCESS);
  BITMASK_TEST( 5,       0x100000002LL, ALLBITS64,          1, EXPECT_FAILURE);
  BITMASK_TEST( 5,       0x100000003LL, ALLBITS64,          1, EXPECT_SUCCESS);

  
  BITMASK_TEST( 6,                   0, ALLBITS64,          3, EXPECT_FAILURE);
  BITMASK_TEST( 6,                   1, ALLBITS64,          3, EXPECT_FAILURE);
  BITMASK_TEST( 6,                   2, ALLBITS64,          3, EXPECT_FAILURE);
  BITMASK_TEST( 6,                   3, ALLBITS64,          3, EXPECT_SUCCESS);
  BITMASK_TEST( 6,                   7, ALLBITS64,          3, EXPECT_SUCCESS);
  BITMASK_TEST( 6,       0x100000000LL, ALLBITS64,          3, EXPECT_FAILURE);
  BITMASK_TEST( 6,       0x100000001LL, ALLBITS64,          3, EXPECT_FAILURE);
  BITMASK_TEST( 6,       0x100000002LL, ALLBITS64,          3, EXPECT_FAILURE);
  BITMASK_TEST( 6,       0x100000003LL, ALLBITS64,          3, EXPECT_SUCCESS);
  BITMASK_TEST( 6,       0x100000007LL, ALLBITS64,          3, EXPECT_SUCCESS);

  
  BITMASK_TEST( 7,                   0, ALLBITS64, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 7,         0x40000000U, ALLBITS64, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 7,         0x80000000U, ALLBITS64, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 7,         0xC0000000U, ALLBITS64, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 7,       -0x80000000LL, ALLBITS64, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 7,       0x100000000LL, ALLBITS64, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 7,       0x140000000LL, ALLBITS64, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 7,       0x180000000LL, ALLBITS64, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 7,       0x1C0000000LL, ALLBITS64, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 7,      -0x180000000LL, ALLBITS64, 0x80000000, EXPECT_SUCCESS);

  
  BITMASK_TEST( 8,       0x000000000LL, ALLBITS64,0x100000000, EXPECT_FAILURE);
  BITMASK_TEST( 8,       0x100000000LL, ALLBITS64,0x100000000, EXPT64_SUCCESS);
  BITMASK_TEST( 8,       0x200000000LL, ALLBITS64,0x100000000, EXPECT_FAILURE);
  BITMASK_TEST( 8,       0x300000000LL, ALLBITS64,0x100000000, EXPT64_SUCCESS);
  BITMASK_TEST( 8,       0x000000001LL, ALLBITS64,0x100000000, EXPECT_FAILURE);
  BITMASK_TEST( 8,       0x100000001LL, ALLBITS64,0x100000000, EXPT64_SUCCESS);
  BITMASK_TEST( 8,       0x200000001LL, ALLBITS64,0x100000000, EXPECT_FAILURE);
  BITMASK_TEST( 8,       0x300000001LL, ALLBITS64,0x100000000, EXPT64_SUCCESS);

  
  BITMASK_TEST( 9,       0x000000000LL, ALLBITS64,0x300000000, EXPECT_FAILURE);
  BITMASK_TEST( 9,       0x100000000LL, ALLBITS64,0x300000000, EXPECT_FAILURE);
  BITMASK_TEST( 9,       0x200000000LL, ALLBITS64,0x300000000, EXPECT_FAILURE);
  BITMASK_TEST( 9,       0x300000000LL, ALLBITS64,0x300000000, EXPT64_SUCCESS);
  BITMASK_TEST( 9,       0x700000000LL, ALLBITS64,0x300000000, EXPT64_SUCCESS);
  BITMASK_TEST( 9,       0x000000001LL, ALLBITS64,0x300000000, EXPECT_FAILURE);
  BITMASK_TEST( 9,       0x100000001LL, ALLBITS64,0x300000000, EXPECT_FAILURE);
  BITMASK_TEST( 9,       0x200000001LL, ALLBITS64,0x300000000, EXPECT_FAILURE);
  BITMASK_TEST( 9,       0x300000001LL, ALLBITS64,0x300000000, EXPT64_SUCCESS);
  BITMASK_TEST( 9,       0x700000001LL, ALLBITS64,0x300000000, EXPT64_SUCCESS);

  
  BITMASK_TEST(10,       0x000000000LL, ALLBITS64,0x100000001, EXPECT_FAILURE);
  BITMASK_TEST(10,       0x000000001LL, ALLBITS64,0x100000001, EXPECT_FAILURE);
  BITMASK_TEST(10,       0x100000000LL, ALLBITS64,0x100000001, EXPECT_FAILURE);
  BITMASK_TEST(10,       0x100000001LL, ALLBITS64,0x100000001, EXPT64_SUCCESS);
  BITMASK_TEST(10,         0xFFFFFFFFU, ALLBITS64,0x100000001, EXPECT_FAILURE);
  BITMASK_TEST(10,                 -1L, ALLBITS64,0x100000001, EXPT64_SUCCESS);
}

class AnyBitTestPolicy : public SandboxBPFPolicy {
 public:
  AnyBitTestPolicy() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(AnyBitTestPolicy);
};

ErrorCode AnyBitTestPolicy::EvaluateSyscall(SandboxBPF* sandbox,
                                            int sysno) const {
  DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
  
  
  
  
  
  
  if (sysno == __NR_uname) {
    return sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 0,
           sandbox->Cond(1, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ANY_BITS,
                         0x0,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 1,
           sandbox->Cond(1, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ANY_BITS,
                         0x1,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 2,
           sandbox->Cond(1, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ANY_BITS,
                         0x3,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 3,
           sandbox->Cond(1, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ANY_BITS,
                         0x80000000,
                         ErrorCode(1), ErrorCode(0)),

           
           
           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 4,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ANY_BITS,
                         0x0,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 5,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ANY_BITS,
                         0x1,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 6,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ANY_BITS,
                         0x3,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 7,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ANY_BITS,
                         0x80000000,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 8,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ANY_BITS,
                         0x100000000ULL,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 9,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ANY_BITS,
                         0x300000000ULL,
                         ErrorCode(1), ErrorCode(0)),

           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL, 10,
           sandbox->Cond(1, ErrorCode::TP_64BIT, ErrorCode::OP_HAS_ANY_BITS,
                         0x100000001ULL,
                         ErrorCode(1), ErrorCode(0)),

                         sandbox->Kill("Invalid test case number"))))))))))));
  }
  return ErrorCode(ErrorCode::ERR_ALLOWED);
}

BPF_TEST_C(SandboxBPF, AnyBitTests, AnyBitTestPolicy) {
  
  BITMASK_TEST( 0,                   0, ANYBITS32,        0x0, EXPECT_FAILURE);
  BITMASK_TEST( 0,                   1, ANYBITS32,        0x0, EXPECT_FAILURE);
  BITMASK_TEST( 0,                   3, ANYBITS32,        0x0, EXPECT_FAILURE);
  BITMASK_TEST( 0,         0xFFFFFFFFU, ANYBITS32,        0x0, EXPECT_FAILURE);
  BITMASK_TEST( 0,                -1LL, ANYBITS32,        0x0, EXPECT_FAILURE);

  
  BITMASK_TEST( 1,                   0, ANYBITS32,        0x1, EXPECT_FAILURE);
  BITMASK_TEST( 1,                   1, ANYBITS32,        0x1, EXPECT_SUCCESS);
  BITMASK_TEST( 1,                   2, ANYBITS32,        0x1, EXPECT_FAILURE);
  BITMASK_TEST( 1,                   3, ANYBITS32,        0x1, EXPECT_SUCCESS);

  
  BITMASK_TEST( 2,                   0, ANYBITS32,        0x3, EXPECT_FAILURE);
  BITMASK_TEST( 2,                   1, ANYBITS32,        0x3, EXPECT_SUCCESS);
  BITMASK_TEST( 2,                   2, ANYBITS32,        0x3, EXPECT_SUCCESS);
  BITMASK_TEST( 2,                   3, ANYBITS32,        0x3, EXPECT_SUCCESS);
  BITMASK_TEST( 2,                   7, ANYBITS32,        0x3, EXPECT_SUCCESS);

  
  BITMASK_TEST( 3,                   0, ANYBITS32, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 3,         0x40000000U, ANYBITS32, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 3,         0x80000000U, ANYBITS32, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 3,         0xC0000000U, ANYBITS32, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 3,       -0x80000000LL, ANYBITS32, 0x80000000, EXPECT_SUCCESS);

  
  BITMASK_TEST( 4,                   0, ANYBITS64,        0x0, EXPECT_FAILURE);
  BITMASK_TEST( 4,                   1, ANYBITS64,        0x0, EXPECT_FAILURE);
  BITMASK_TEST( 4,                   3, ANYBITS64,        0x0, EXPECT_FAILURE);
  BITMASK_TEST( 4,         0xFFFFFFFFU, ANYBITS64,        0x0, EXPECT_FAILURE);
  BITMASK_TEST( 4,       0x100000000LL, ANYBITS64,        0x0, EXPECT_FAILURE);
  BITMASK_TEST( 4,       0x300000000LL, ANYBITS64,        0x0, EXPECT_FAILURE);
  BITMASK_TEST( 4,0x8000000000000000LL, ANYBITS64,        0x0, EXPECT_FAILURE);
  BITMASK_TEST( 4,                -1LL, ANYBITS64,        0x0, EXPECT_FAILURE);

  
  BITMASK_TEST( 5,                   0, ANYBITS64,        0x1, EXPECT_FAILURE);
  BITMASK_TEST( 5,                   1, ANYBITS64,        0x1, EXPECT_SUCCESS);
  BITMASK_TEST( 5,                   2, ANYBITS64,        0x1, EXPECT_FAILURE);
  BITMASK_TEST( 5,                   3, ANYBITS64,        0x1, EXPECT_SUCCESS);
  BITMASK_TEST( 5,       0x100000001LL, ANYBITS64,        0x1, EXPECT_SUCCESS);
  BITMASK_TEST( 5,       0x100000000LL, ANYBITS64,        0x1, EXPECT_FAILURE);
  BITMASK_TEST( 5,       0x100000002LL, ANYBITS64,        0x1, EXPECT_FAILURE);
  BITMASK_TEST( 5,       0x100000003LL, ANYBITS64,        0x1, EXPECT_SUCCESS);

  
  BITMASK_TEST( 6,                   0, ANYBITS64,        0x3, EXPECT_FAILURE);
  BITMASK_TEST( 6,                   1, ANYBITS64,        0x3, EXPECT_SUCCESS);
  BITMASK_TEST( 6,                   2, ANYBITS64,        0x3, EXPECT_SUCCESS);
  BITMASK_TEST( 6,                   3, ANYBITS64,        0x3, EXPECT_SUCCESS);
  BITMASK_TEST( 6,                   7, ANYBITS64,        0x3, EXPECT_SUCCESS);
  BITMASK_TEST( 6,       0x100000000LL, ANYBITS64,        0x3, EXPECT_FAILURE);
  BITMASK_TEST( 6,       0x100000001LL, ANYBITS64,        0x3, EXPECT_SUCCESS);
  BITMASK_TEST( 6,       0x100000002LL, ANYBITS64,        0x3, EXPECT_SUCCESS);
  BITMASK_TEST( 6,       0x100000003LL, ANYBITS64,        0x3, EXPECT_SUCCESS);
  BITMASK_TEST( 6,       0x100000007LL, ANYBITS64,        0x3, EXPECT_SUCCESS);

  
  BITMASK_TEST( 7,                   0, ANYBITS64, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 7,         0x40000000U, ANYBITS64, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 7,         0x80000000U, ANYBITS64, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 7,         0xC0000000U, ANYBITS64, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 7,       -0x80000000LL, ANYBITS64, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 7,       0x100000000LL, ANYBITS64, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 7,       0x140000000LL, ANYBITS64, 0x80000000, EXPECT_FAILURE);
  BITMASK_TEST( 7,       0x180000000LL, ANYBITS64, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 7,       0x1C0000000LL, ANYBITS64, 0x80000000, EXPECT_SUCCESS);
  BITMASK_TEST( 7,      -0x180000000LL, ANYBITS64, 0x80000000, EXPECT_SUCCESS);

  
  BITMASK_TEST( 8,       0x000000000LL, ANYBITS64,0x100000000, EXPECT_FAILURE);
  BITMASK_TEST( 8,       0x100000000LL, ANYBITS64,0x100000000, EXPT64_SUCCESS);
  BITMASK_TEST( 8,       0x200000000LL, ANYBITS64,0x100000000, EXPECT_FAILURE);
  BITMASK_TEST( 8,       0x300000000LL, ANYBITS64,0x100000000, EXPT64_SUCCESS);
  BITMASK_TEST( 8,       0x000000001LL, ANYBITS64,0x100000000, EXPECT_FAILURE);
  BITMASK_TEST( 8,       0x100000001LL, ANYBITS64,0x100000000, EXPT64_SUCCESS);
  BITMASK_TEST( 8,       0x200000001LL, ANYBITS64,0x100000000, EXPECT_FAILURE);
  BITMASK_TEST( 8,       0x300000001LL, ANYBITS64,0x100000000, EXPT64_SUCCESS);

  
  BITMASK_TEST( 9,       0x000000000LL, ANYBITS64,0x300000000, EXPECT_FAILURE);
  BITMASK_TEST( 9,       0x100000000LL, ANYBITS64,0x300000000, EXPT64_SUCCESS);
  BITMASK_TEST( 9,       0x200000000LL, ANYBITS64,0x300000000, EXPT64_SUCCESS);
  BITMASK_TEST( 9,       0x300000000LL, ANYBITS64,0x300000000, EXPT64_SUCCESS);
  BITMASK_TEST( 9,       0x700000000LL, ANYBITS64,0x300000000, EXPT64_SUCCESS);
  BITMASK_TEST( 9,       0x000000001LL, ANYBITS64,0x300000000, EXPECT_FAILURE);
  BITMASK_TEST( 9,       0x100000001LL, ANYBITS64,0x300000000, EXPT64_SUCCESS);
  BITMASK_TEST( 9,       0x200000001LL, ANYBITS64,0x300000000, EXPT64_SUCCESS);
  BITMASK_TEST( 9,       0x300000001LL, ANYBITS64,0x300000000, EXPT64_SUCCESS);
  BITMASK_TEST( 9,       0x700000001LL, ANYBITS64,0x300000000, EXPT64_SUCCESS);

  
  BITMASK_TEST( 10,      0x000000000LL, ANYBITS64,0x100000001, EXPECT_FAILURE);
  BITMASK_TEST( 10,      0x000000001LL, ANYBITS64,0x100000001, EXPECT_SUCCESS);
  BITMASK_TEST( 10,      0x100000000LL, ANYBITS64,0x100000001, EXPT64_SUCCESS);
  BITMASK_TEST( 10,      0x100000001LL, ANYBITS64,0x100000001, EXPECT_SUCCESS);
  BITMASK_TEST( 10,        0xFFFFFFFFU, ANYBITS64,0x100000001, EXPECT_SUCCESS);
  BITMASK_TEST( 10,                -1L, ANYBITS64,0x100000001, EXPECT_SUCCESS);
}

intptr_t PthreadTrapHandler(const struct arch_seccomp_data& args, void* aux) {
  if (args.args[0] != (CLONE_CHILD_CLEARTID | CLONE_CHILD_SETTID | SIGCHLD)) {
    
    
    
    const char* msg = (const char*)aux;
    printf(
        "Clone() was called with unexpected arguments\n"
        "  nr: %d\n"
        "  1: 0x%llX\n"
        "  2: 0x%llX\n"
        "  3: 0x%llX\n"
        "  4: 0x%llX\n"
        "  5: 0x%llX\n"
        "  6: 0x%llX\n"
        "%s\n",
        args.nr,
        (long long)args.args[0],
        (long long)args.args[1],
        (long long)args.args[2],
        (long long)args.args[3],
        (long long)args.args[4],
        (long long)args.args[5],
        msg);
  }
  return -EPERM;
}

class PthreadPolicyEquality : public SandboxBPFPolicy {
 public:
  PthreadPolicyEquality() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(PthreadPolicyEquality);
};

ErrorCode PthreadPolicyEquality::EvaluateSyscall(SandboxBPF* sandbox,
                                                 int sysno) const {
  DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
  
  
  
  
  if (sysno == __NR_clone) {
    
    
    
    
    
    
    
    
    
    const uint64_t kGlibcCloneMask =
        CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
        CLONE_THREAD | CLONE_SYSVSEM | CLONE_SETTLS |
        CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID;
    const uint64_t kBaseAndroidCloneMask =
        CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
        CLONE_THREAD | CLONE_SYSVSEM;
    return sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL,
                         kGlibcCloneMask,
                         ErrorCode(ErrorCode::ERR_ALLOWED),
           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL,
                         kBaseAndroidCloneMask | CLONE_DETACHED,
                         ErrorCode(ErrorCode::ERR_ALLOWED),
           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL,
                         kBaseAndroidCloneMask,
                         ErrorCode(ErrorCode::ERR_ALLOWED),
                         sandbox->Trap(PthreadTrapHandler, "Unknown mask"))));
  }
  return ErrorCode(ErrorCode::ERR_ALLOWED);
}

class PthreadPolicyBitMask : public SandboxBPFPolicy {
 public:
  PthreadPolicyBitMask() {}
  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox,
                                    int sysno) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(PthreadPolicyBitMask);
};

ErrorCode PthreadPolicyBitMask::EvaluateSyscall(SandboxBPF* sandbox,
                                                int sysno) const {
  DCHECK(SandboxBPF::IsValidSyscallNumber(sysno));
  
  
  
  
  if (sysno == __NR_clone) {
    
    
    
    
    
    
    
    
    
    return sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ANY_BITS,
                         ~uint32(CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|
                                 CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS|
                                 CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID|
                                 CLONE_DETACHED),
                         sandbox->Trap(PthreadTrapHandler,
                                       "Unexpected CLONE_XXX flag found"),
           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ALL_BITS,
                         CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|
                         CLONE_THREAD|CLONE_SYSVSEM,
           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ALL_BITS,
                         CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID,
                         ErrorCode(ErrorCode::ERR_ALLOWED),
           sandbox->Cond(0, ErrorCode::TP_32BIT, ErrorCode::OP_HAS_ANY_BITS,
                         CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID,
                         sandbox->Trap(PthreadTrapHandler,
                                       "Must set either all or none of the TLS"
                                       " and futex bits in call to clone()"),
                         ErrorCode(ErrorCode::ERR_ALLOWED))),
                         sandbox->Trap(PthreadTrapHandler,
                                       "Missing mandatory CLONE_XXX flags "
                                       "when creating new thread")));
  }
  return ErrorCode(ErrorCode::ERR_ALLOWED);
}

static void* ThreadFnc(void* arg) {
  ++*reinterpret_cast<int*>(arg);
  Syscall::Call(__NR_futex, arg, FUTEX_WAKE, 1, 0, 0, 0);
  return NULL;
}

static void PthreadTest() {
  
  pthread_t thread;
  int thread_ran = 0;
  BPF_ASSERT(!pthread_create(&thread, NULL, ThreadFnc, &thread_ran));
  BPF_ASSERT(!pthread_join(thread, NULL));
  BPF_ASSERT(thread_ran);

  
  thread_ran = 0;
  pthread_attr_t attr;
  BPF_ASSERT(!pthread_attr_init(&attr));
  BPF_ASSERT(!pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED));
  BPF_ASSERT(!pthread_create(&thread, &attr, ThreadFnc, &thread_ran));
  BPF_ASSERT(!pthread_attr_destroy(&attr));
  while (Syscall::Call(__NR_futex, &thread_ran, FUTEX_WAIT, 0, 0, 0, 0) ==
         -EINTR) {
  }
  BPF_ASSERT(thread_ran);

  
  
  
  
  
  int pid;
  BPF_ASSERT(Syscall::Call(__NR_clone,
                           CLONE_CHILD_CLEARTID | CLONE_CHILD_SETTID | SIGCHLD,
                           0,
                           0,
                           &pid) == -EPERM);
}

BPF_TEST_C(SandboxBPF, PthreadEquality, PthreadPolicyEquality) {
  PthreadTest();
}

BPF_TEST_C(SandboxBPF, PthreadBitMask, PthreadPolicyBitMask) {
  PthreadTest();
}


#ifndef PTRACE_O_TRACESECCOMP
#define PTRACE_O_TRACESECCOMP 0x00000080
#endif

#ifdef PTRACE_EVENT_SECCOMP
#define IS_SECCOMP_EVENT(status) ((status >> 16) == PTRACE_EVENT_SECCOMP)
#else





#define IS_SECCOMP_EVENT(status) ((status >> 16) == 7 || (status >> 16) == 8)
#endif

#if defined(__arm__)
#ifndef PTRACE_SET_SYSCALL
#define PTRACE_SET_SYSCALL 23
#endif
#endif










long SetSyscall(pid_t pid, regs_struct* regs, int syscall_number) {
#if defined(__arm__)
  
  
  
  return syscall(__NR_ptrace, PTRACE_SET_SYSCALL, pid, NULL, syscall_number);
#endif

  SECCOMP_PT_SYSCALL(*regs) = syscall_number;
  return 0;
}

const uint16_t kTraceData = 0xcc;

class TraceAllPolicy : public SandboxBPFPolicy {
 public:
  TraceAllPolicy() {}
  virtual ~TraceAllPolicy() {}

  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox_compiler,
                                    int system_call_number) const OVERRIDE {
    return ErrorCode(ErrorCode::ERR_TRACE + kTraceData);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TraceAllPolicy);
};

SANDBOX_TEST(SandboxBPF, DISABLE_ON_TSAN(SeccompRetTrace)) {
  if (SandboxBPF::SupportsSeccompSandbox(-1) !=
      sandbox::SandboxBPF::STATUS_AVAILABLE) {
    return;
  }

#if defined(__arm__)
  printf("This test is currently disabled on ARM due to a kernel bug.");
  return;
#endif

  pid_t pid = fork();
  BPF_ASSERT_NE(-1, pid);
  if (pid == 0) {
    pid_t my_pid = getpid();
    BPF_ASSERT_NE(-1, ptrace(PTRACE_TRACEME, -1, NULL, NULL));
    BPF_ASSERT_EQ(0, raise(SIGSTOP));
    SandboxBPF sandbox;
    sandbox.SetSandboxPolicy(new TraceAllPolicy);
    BPF_ASSERT(sandbox.StartSandbox(SandboxBPF::PROCESS_SINGLE_THREADED));

    
    BPF_ASSERT_EQ(my_pid, syscall(__NR_getpid));

    
    BPF_ASSERT_EQ(kExpectedReturnValue,
                  syscall(__NR_write, STDOUT_FILENO, "A", 1));

    
    syscall(__NR_kill, my_pid, SIGKILL);

    
    BPF_ASSERT(false);
  }

  int status;
  BPF_ASSERT(HANDLE_EINTR(waitpid(pid, &status, WUNTRACED)) != -1);
  BPF_ASSERT(WIFSTOPPED(status));

  BPF_ASSERT_NE(-1, ptrace(PTRACE_SETOPTIONS, pid, NULL,
                           reinterpret_cast<void*>(PTRACE_O_TRACESECCOMP)));
  BPF_ASSERT_NE(-1, ptrace(PTRACE_CONT, pid, NULL, NULL));
  while (true) {
    BPF_ASSERT(HANDLE_EINTR(waitpid(pid, &status, 0)) != -1);
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
      BPF_ASSERT(WIFEXITED(status));
      BPF_ASSERT_EQ(kExpectedReturnValue, WEXITSTATUS(status));
      break;
    }

    if (!WIFSTOPPED(status) || WSTOPSIG(status) != SIGTRAP ||
        !IS_SECCOMP_EVENT(status)) {
      BPF_ASSERT_NE(-1, ptrace(PTRACE_CONT, pid, NULL, NULL));
      continue;
    }

    unsigned long data;
    BPF_ASSERT_NE(-1, ptrace(PTRACE_GETEVENTMSG, pid, NULL, &data));
    BPF_ASSERT_EQ(kTraceData, data);

    regs_struct regs;
    BPF_ASSERT_NE(-1, ptrace(PTRACE_GETREGS, pid, NULL, &regs));
    switch (SECCOMP_PT_SYSCALL(regs)) {
      case __NR_write:
        
        
        if (SECCOMP_PT_PARM1(regs) == STDOUT_FILENO) {
          BPF_ASSERT_NE(-1, SetSyscall(pid, &regs, -1));
          SECCOMP_PT_RESULT(regs) = kExpectedReturnValue;
          BPF_ASSERT_NE(-1, ptrace(PTRACE_SETREGS, pid, NULL, &regs));
        }
        break;

      case __NR_kill:
        
        BPF_ASSERT_NE(-1, SetSyscall(pid, &regs, __NR_exit));
        SECCOMP_PT_PARM1(regs) = kExpectedReturnValue;
        BPF_ASSERT_NE(-1, ptrace(PTRACE_SETREGS, pid, NULL, &regs));
        break;

      default:
        
        break;
    }

    BPF_ASSERT_NE(-1, ptrace(PTRACE_CONT, pid, NULL, NULL));
  }
}


#if !defined(OS_ANDROID)

bool FullPwrite64(int fd, const char* buffer, size_t count, off64_t offset) {
  while (count > 0) {
    const ssize_t transfered =
        HANDLE_EINTR(pwrite64(fd, buffer, count, offset));
    if (transfered <= 0 || static_cast<size_t>(transfered) > count) {
      return false;
    }
    count -= transfered;
    buffer += transfered;
    offset += transfered;
  }
  return true;
}

bool FullPread64(int fd, char* buffer, size_t count, off64_t offset) {
  while (count > 0) {
    const ssize_t transfered = HANDLE_EINTR(pread64(fd, buffer, count, offset));
    if (transfered <= 0 || static_cast<size_t>(transfered) > count) {
      return false;
    }
    count -= transfered;
    buffer += transfered;
    offset += transfered;
  }
  return true;
}

bool pread_64_was_forwarded = false;

class TrapPread64Policy : public SandboxBPFPolicy {
 public:
  TrapPread64Policy() {}
  virtual ~TrapPread64Policy() {}

  virtual ErrorCode EvaluateSyscall(SandboxBPF* sandbox_compiler,
                                    int system_call_number) const OVERRIDE {
    
    if (system_call_number == MIN_SYSCALL) {
      EnableUnsafeTraps();
    }

    if (system_call_number == __NR_pread64) {
      return sandbox_compiler->UnsafeTrap(ForwardPreadHandler, NULL);
    }
    return ErrorCode(ErrorCode::ERR_ALLOWED);
  }

 private:
  static intptr_t ForwardPreadHandler(const struct arch_seccomp_data& args,
                                      void* aux) {
    BPF_ASSERT(args.nr == __NR_pread64);
    pread_64_was_forwarded = true;

    return SandboxBPF::ForwardSyscall(args);
  }
  DISALLOW_COPY_AND_ASSIGN(TrapPread64Policy);
};




BPF_TEST_C(SandboxBPF, Pread64, TrapPread64Policy) {
  ScopedTemporaryFile temp_file;
  const uint64_t kLargeOffset = (static_cast<uint64_t>(1) << 32) | 0xBEEF;
  const char kTestString[] = "This is a test!";
  BPF_ASSERT(FullPwrite64(
      temp_file.fd(), kTestString, sizeof(kTestString), kLargeOffset));

  char read_test_string[sizeof(kTestString)] = {0};
  BPF_ASSERT(FullPread64(temp_file.fd(),
                         read_test_string,
                         sizeof(read_test_string),
                         kLargeOffset));
  BPF_ASSERT_EQ(0, memcmp(kTestString, read_test_string, sizeof(kTestString)));
  BPF_ASSERT(pread_64_was_forwarded);
}

#endif  

}  

}  
