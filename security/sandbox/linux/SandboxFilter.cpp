





#include "SandboxFilter.h"
#include "SandboxFilterUtil.h"
#include "SandboxInternal.h"
#include "SandboxLogging.h"

#include "mozilla/UniquePtr.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/ipc.h>
#include <linux/net.h>
#include <linux/prctl.h>
#include <linux/sched.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include "sandbox/linux/bpf_dsl/bpf_dsl.h"
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"
#include "sandbox/linux/services/linux_syscalls.h"

using namespace sandbox::bpf_dsl;
#define CASES SANDBOX_BPF_DSL_CASES


#ifndef ANDROID
#define DESKTOP
#endif












namespace mozilla {



class SandboxPolicyCommon : public SandboxPolicyBase
{
  static intptr_t BlockedSyscallTrap(const sandbox::arch_seccomp_data& aArgs,
                                     void *aux)
  {
    MOZ_ASSERT(!aux);
    return -ENOSYS;
  }

#if defined(ANDROID) && ANDROID_VERSION < 16
  
  
  static intptr_t TKillCompatTrap(const sandbox::arch_seccomp_data& aArgs,
                                  void *aux)
  {
    return syscall(__NR_tgkill, getpid(), aArgs.args[0], aArgs.args[1]);
  }
#endif

public:
  virtual ResultExpr InvalidSyscall() const override {
    return Trap(BlockedSyscallTrap, nullptr);
  }

  virtual ResultExpr ClonePolicy() const {
    

    
    
    
    Arg<int> flags(0);

    
    
    
    
    static const int flags_common = CLONE_VM | CLONE_FS | CLONE_FILES |
      CLONE_SIGHAND | CLONE_THREAD | CLONE_SYSVSEM;
    static const int flags_modern = flags_common | CLONE_SETTLS |
      CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID;

    
    
    
    return Switch(flags)
#ifdef ANDROID
      .Case(flags_common | CLONE_DETACHED, Allow()) 
      .Case(flags_common, Allow()) 
#endif
      .Case(flags_modern, Allow()) 
      .Default(InvalidSyscall());
  }

  virtual ResultExpr PrctlPolicy() const {
    
    
    
    Arg<int> op(0);
    return Switch(op)
      .CASES((PR_GET_SECCOMP, 
              PR_SET_NAME,    
              PR_SET_DUMPABLE), 
             Allow())
      .Default(InvalidSyscall());
  }

  virtual Maybe<ResultExpr> EvaluateSocketCall(int aCall) const override {
    switch (aCall) {
    case SYS_RECVMSG:
    case SYS_SENDMSG:
      return Some(Allow());
    default:
      return Nothing();
    }
  }

  virtual ResultExpr EvaluateSyscall(int sysno) const override {
    switch (sysno) {
      
    case __NR_clock_gettime: {
      Arg<clockid_t> clk_id(0);
      return If(clk_id == CLOCK_MONOTONIC, Allow())
        .ElseIf(clk_id == CLOCK_REALTIME, Allow())
        .Else(InvalidSyscall());
    }
    case __NR_gettimeofday:
#ifdef __NR_time
    case __NR_time:
#endif
    case __NR_nanosleep:
      return Allow();

      
    case __NR_futex:
      
      return Allow();

      
    case __NR_epoll_wait:
    case __NR_epoll_pwait:
    case __NR_epoll_ctl:
    case __NR_ppoll:
    case __NR_poll:
      return Allow();

      
    case __NR_pipe:
      return Allow();

      
    CASES_FOR_fstat:
      return Allow();

      
    case __NR_write:
    case __NR_read:
    case __NR_writev: 
      return Allow();

      
    CASES_FOR_mmap:
    case __NR_munmap:
      return Allow();

      
#if defined(ANDROID) || defined(MOZ_ASAN)
    case __NR_sigaltstack:
#endif
    CASES_FOR_sigreturn:
    CASES_FOR_sigprocmask:
    CASES_FOR_sigaction:
      return Allow();

      
    case __NR_tgkill: {
      Arg<pid_t> tgid(0);
      return If(tgid == getpid(), Allow())
        .Else(InvalidSyscall());
    }

#if defined(ANDROID) && ANDROID_VERSION < 16
      
    case __NR_tkill:
      return Trap(TKillCompatTrap, nullptr);
#endif

      
    case __NR_sched_yield:
      return Allow();

      
    case __NR_clone:
      return ClonePolicy();

      
#ifdef __NR_set_robust_list
    case __NR_set_robust_list:
      return Allow();
#endif
#ifdef ANDROID
    case __NR_set_tid_address:
      return Allow();
#endif

      
    case __NR_prctl:
      return PrctlPolicy();

      
      
    case __NR_getpriority:
      
      
    case __NR_setpriority:
      return Error(EACCES);

      
      
    case __NR_sched_getaffinity:
      return Error(ENOSYS);

      
    case __NR_getpid:
    case __NR_gettid:
      return Allow();

      
    case __NR_close:
      return Allow();

      
#ifdef __arm__
    case __ARM_NR_breakpoint:
    case __ARM_NR_cacheflush:
    case __ARM_NR_usr26: 
    case __ARM_NR_usr32:
    case __ARM_NR_set_tls:
      return Allow();
#endif

      
    case __NR_restart_syscall:
      return Allow();

      
    case __NR_exit:
    case __NR_exit_group:
      return Allow();

#ifdef MOZ_ASAN
      
    case __NR_ioctl: {
      Arg<int> fd(0);
      return If(fd == STDERR_FILENO, Allow())
        .Else(InvalidSyscall());
    }

      
      
    case __NR_readlink:
    case __NR_readlinkat:
      return Error(ENOENT);

      
      
    CASES_FOR_stat:
      return Error(ENOENT);
#endif

    default:
      return SandboxPolicyBase::EvaluateSyscall(sysno);
    }
  }
};



#ifdef MOZ_CONTENT_SANDBOX





class ContentSandboxPolicy : public SandboxPolicyCommon {
public:
  ContentSandboxPolicy() { }
  virtual ~ContentSandboxPolicy() { }
  virtual ResultExpr PrctlPolicy() const override {
    
    
    return Allow();
  }
  virtual Maybe<ResultExpr> EvaluateSocketCall(int aCall) const override {
    switch(aCall) {
    case SYS_RECVFROM:
    case SYS_SENDTO:
      return Some(Allow());

    case SYS_SOCKETPAIR: {
      
      if (!kSocketCallHasArgs) {
        
        return Some(Allow());
      }
      Arg<int> domain(0), type(1);
      return Some(If(domain == AF_UNIX &&
                     (type == SOCK_STREAM || type == SOCK_SEQPACKET), Allow())
                  .Else(InvalidSyscall()));
    }

#ifdef ANDROID
    case SYS_SOCKET:
      return Some(Error(EACCES));
#else 
    case SYS_RECV:
    case SYS_SEND:
    case SYS_SOCKET: 
    case SYS_CONNECT: 
    case SYS_SETSOCKOPT:
    case SYS_GETSOCKNAME:
    case SYS_GETPEERNAME:
    case SYS_SHUTDOWN:
      return Some(Allow());
#endif
    default:
      return SandboxPolicyCommon::EvaluateSocketCall(aCall);
    }
  }

#ifdef DESKTOP
  virtual Maybe<ResultExpr> EvaluateIpcCall(int aCall) const override {
    switch(aCall) {
      
      
      
      
    case SHMGET:
    case SHMCTL:
    case SHMAT:
    case SHMDT:
      return Some(Allow());
    default:
      return SandboxPolicyCommon::EvaluateIpcCall(aCall);
    }
  }
#endif

  virtual ResultExpr EvaluateSyscall(int sysno) const override {
    switch(sysno) {
      
    case __NR_open:
    case __NR_openat:
    case __NR_access:
    case __NR_faccessat:
    CASES_FOR_stat:
    CASES_FOR_lstat:
    CASES_FOR_fstatat:
#ifdef DESKTOP
      
      
    case __NR_mkdir:
    case __NR_rmdir:
    case __NR_getcwd:
    CASES_FOR_statfs:
    case __NR_chmod:
    case __NR_rename:
    case __NR_symlink:
    case __NR_quotactl:
    case __NR_utimes:
#endif
      return Allow();

    case __NR_readlink:
    case __NR_readlinkat:
      
      return Error(EINVAL);

    CASES_FOR_select:
    case __NR_pselect6:
      return Allow();

    CASES_FOR_getdents:
    CASES_FOR_lseek:
    CASES_FOR_ftruncate:
    case __NR_writev:
    case __NR_pread64:
#ifdef DESKTOP
    case __NR_readahead:
#endif
      return Allow();

    case __NR_ioctl:
      
      
      
      return Allow();

    CASES_FOR_fcntl:
      
      
      
      return Allow();

    case __NR_mprotect:
    case __NR_brk:
    case __NR_madvise:
#if defined(ANDROID) && !defined(MOZ_MEMORY)
      
    case __NR_mremap:
#endif
      return Allow();

    case __NR_sigaltstack:
      return Allow();

#ifdef __NR_set_thread_area
    case __NR_set_thread_area:
      return Allow();
#endif

    case __NR_getrusage:
    case __NR_times:
      return Allow();

    case __NR_dup:
      return Allow();

    CASES_FOR_getuid:
    CASES_FOR_getgid:
    CASES_FOR_geteuid:
    CASES_FOR_getegid:
      return Allow();

    case __NR_fsync:
    case __NR_msync:
      return Allow();

    case __NR_getpriority:
    case __NR_setpriority:
    case __NR_sched_get_priority_min:
    case __NR_sched_get_priority_max:
    case __NR_sched_getscheduler:
    case __NR_sched_setscheduler:
    case __NR_sched_getparam:
    case __NR_sched_setparam:
#ifdef DESKTOP
    case __NR_sched_getaffinity:
#endif
      return Allow();

#ifdef DESKTOP
    case __NR_pipe2:
      return Allow();

    CASES_FOR_getrlimit:
    case __NR_clock_getres:
    case __NR_getresuid:
    case __NR_getresgid:
      return Allow();

    case __NR_umask:
    case __NR_kill:
    case __NR_wait4:
#ifdef __NR_arch_prctl
    case __NR_arch_prctl:
#endif
      return Allow();

    case __NR_eventfd2:
    case __NR_inotify_init1:
    case __NR_inotify_add_watch:
      return Allow();
#endif



    case __NR_uname:
#ifdef DESKTOP
    case __NR_sysinfo:
#endif
      return Allow();

    default:
      return SandboxPolicyCommon::EvaluateSyscall(sysno);
    }
  }
};

UniquePtr<sandbox::bpf_dsl::Policy>
GetContentSandboxPolicy()
{
  return UniquePtr<sandbox::bpf_dsl::Policy>(new ContentSandboxPolicy());
}
#endif 


#ifdef MOZ_GMP_SANDBOX





class GMPSandboxPolicy : public SandboxPolicyCommon {
  static intptr_t OpenTrap(const sandbox::arch_seccomp_data& aArgs,
                           void* aux)
  {
    auto plugin = static_cast<SandboxOpenedFile*>(aux);
    const char* path;
    int flags;

    switch (aArgs.nr) {
#ifdef __NR_open
    case __NR_open:
      path = reinterpret_cast<const char*>(aArgs.args[0]);
      flags = static_cast<int>(aArgs.args[1]);
      break;
#endif
    case __NR_openat:
      
      
      path = reinterpret_cast<const char*>(aArgs.args[1]);
      flags = static_cast<int>(aArgs.args[2]);
      break;
    default:
      MOZ_CRASH("unexpected syscall number");
    }

    if ((flags & O_ACCMODE) != O_RDONLY) {
      SANDBOX_LOG_ERROR("non-read-only open of file %s attempted (flags=0%o)",
                        path, flags);
      return -ENOSYS;
    }
    if (strcmp(path, plugin->mPath) != 0) {
      SANDBOX_LOG_ERROR("attempt to open file %s which is not the media plugin"
                        " %s", path, plugin->mPath);
      return -ENOSYS;
    }
    int fd = plugin->mFd.exchange(-1);
    if (fd < 0) {
      SANDBOX_LOG_ERROR("multiple opens of media plugin file unimplemented");
      return -ENOSYS;
    }
    return fd;
  }

  SandboxOpenedFile* mPlugin;
public:
  explicit GMPSandboxPolicy(SandboxOpenedFile* aPlugin)
  : mPlugin(aPlugin)
  {
    MOZ_ASSERT(aPlugin->mPath[0] == '/', "plugin path should be absolute");
  }

  virtual ~GMPSandboxPolicy() { }

  virtual ResultExpr EvaluateSyscall(int sysno) const override {
    switch (sysno) {
      
#ifdef __NR_open
    case __NR_open:
#endif
    case __NR_openat:
      return Trap(OpenTrap, mPlugin);

      
    case __NR_mprotect:
      return Allow();
    case __NR_madvise: {
      Arg<int> advice(2);
      return If(advice == MADV_DONTNEED, Allow())
        .Else(InvalidSyscall());
    }

    default:
      return SandboxPolicyCommon::EvaluateSyscall(sysno);
    }
  }
};

UniquePtr<sandbox::bpf_dsl::Policy>
GetMediaSandboxPolicy(SandboxOpenedFile* aPlugin)
{
  return UniquePtr<sandbox::bpf_dsl::Policy>(new GMPSandboxPolicy(aPlugin));
}

#endif 

}
