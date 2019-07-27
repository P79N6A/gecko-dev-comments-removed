





#ifndef mozilla_SandboxFilterUtil_h
#define mozilla_SandboxFilterUtil_h






#include "mozilla/Maybe.h"
#include "sandbox/linux/services/linux_syscalls.h"
#include "sandbox/linux/bpf_dsl/policy.h"

namespace mozilla {












class SandboxPolicyBase : public sandbox::bpf_dsl::Policy
{
public:
  using ResultExpr = sandbox::bpf_dsl::ResultExpr;

  virtual ResultExpr EvaluateSyscall(int aSysno) const override;
  virtual Maybe<ResultExpr> EvaluateSocketCall(int aCall) const {
    return Nothing();
  }
#ifndef ANDROID
  
  
  virtual Maybe<ResultExpr> EvaluateIpcCall(int aCall) const {
    return Nothing();
  }
#endif

#ifdef __NR_socketcall
  
  
  static const bool kSocketCallHasArgs = false;
  static const bool kIpcCallNormalArgs = false;
#else
  
  static const bool kSocketCallHasArgs = true;
  static const bool kIpcCallNormalArgs = true;
#endif
};

} 









#ifdef __NR_mmap2
#define CASES_FOR_mmap   case __NR_mmap2
#else
#define CASES_FOR_mmap   case __NR_mmap
#endif

#ifdef __NR_getuid32
#define CASES_FOR_getuid   case __NR_getuid32
#define CASES_FOR_getgid   case __NR_getgid32
#define CASES_FOR_geteuid   case __NR_geteuid32
#define CASES_FOR_getegid   case __NR_getegid32
#define CASES_FOR_getresuid   case __NR_getresuid32
#define CASES_FOR_getresgid   case __NR_getresgid32

#else
#define CASES_FOR_getuid   case __NR_getuid
#define CASES_FOR_getgid   case __NR_getgid
#define CASES_FOR_geteuid   case __NR_geteuid
#define CASES_FOR_getegid   case __NR_getegid
#define CASES_FOR_getresuid   case __NR_getresuid
#define CASES_FOR_getresgid   case __NR_getresgid
#endif

#ifdef __NR_stat64
#define CASES_FOR_stat   case __NR_stat64
#define CASES_FOR_lstat   case __NR_lstat64
#define CASES_FOR_fstat   case __NR_fstat64
#define CASES_FOR_fstatat   case __NR_fstatat64
#define CASES_FOR_statfs   case __NR_statfs64
#define CASES_FOR_fcntl   case __NR_fcntl64

#define CASES_FOR_getdents   case __NR_getdents64: case __NR_getdents

#define CASES_FOR_lseek   case __NR_lseek: case __NR__llseek
#define CASES_FOR_ftruncate   case __NR_ftruncate: case __NR_ftruncate64
#else
#define CASES_FOR_stat   case __NR_stat
#define CASES_FOR_lstat   case __NR_lstat
#define CASES_FOR_fstatat   case __NR_newfstatat
#define CASES_FOR_fstat   case __NR_fstat
#define CASES_FOR_statfs   case __NR_statfs
#define CASES_FOR_fcntl   case __NR_fcntl
#define CASES_FOR_getdents   case __NR_getdents
#define CASES_FOR_lseek   case __NR_lseek
#define CASES_FOR_ftruncate   case __NR_ftruncate
#endif

#ifdef __NR_sigprocmask
#define CASES_FOR_sigprocmask   case __NR_sigprocmask: case __NR_rt_sigprocmask
#define CASES_FOR_sigaction   case __NR_sigaction: case __NR_rt_sigaction
#define CASES_FOR_sigreturn   case __NR_sigreturn: case __NR_rt_sigreturn
#else
#define CASES_FOR_sigprocmask   case __NR_rt_sigprocmask
#define CASES_FOR_sigaction   case __NR_rt_sigaction
#define CASES_FOR_sigreturn   case __NR_rt_sigreturn
#endif

#ifdef __NR__newselect
#define CASES_FOR_select   case __NR__newselect
#else
#define CASES_FOR_select   case __NR_select
#endif

#ifdef __NR_ugetrlimit
#define CASES_FOR_getrlimit   case __NR_ugetrlimit
#else
#define CASES_FOR_getrlimit   case __NR_getrlimit
#endif

#endif 
