





#include "SandboxFilter.h"
#include "SandboxAssembler.h"

#include "linux_seccomp.h"
#include "linux_syscalls.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/NullPtr.h"

#include <errno.h>
#include <unistd.h>

namespace mozilla {

class SandboxFilterImpl : public SandboxAssembler
{
  void Build();
public:
  SandboxFilterImpl() {
    Build();
    Finish();
  }
};

SandboxFilter::SandboxFilter(const sock_fprog** aStored, bool aVerbose)
  : mStored(aStored)
{
  MOZ_ASSERT(*mStored == nullptr);
  std::vector<struct sock_filter> filterVec;
  {
    SandboxFilterImpl impl;
    impl.Compile(&filterVec, aVerbose);
  }
  mProg = new sock_fprog;
  mProg->len = filterVec.size();
  mProg->filter = mFilter = new sock_filter[mProg->len];
  for (size_t i = 0; i < mProg->len; ++i) {
    mFilter[i] = filterVec[i];
  }
  *mStored = mProg;
}

SandboxFilter::~SandboxFilter()
{
  *mStored = nullptr;
  delete[] mFilter;
  delete mProg;
}

void
SandboxFilterImpl::Build() {
#define SYSCALL_EXISTS(name) (defined(__NR_##name))

#define SYSCALL(name) (Condition(__NR_##name))
#if defined(__arm__) && (defined(__thumb__) || defined(__ARM_EABI__))
#define ARM_SYSCALL(name) (Condition(__ARM_NR_##name))
#endif

#define SYSCALL_WITH_ARG(name, arg, values...) ({ \
  uint32_t argValues[] = { values };              \
  Condition(__NR_##name, arg, argValues);         \
})

  
  
  
  
#if SYSCALL_EXISTS(stat64)
#define SYSCALL_LARGEFILE(plain, versioned) SYSCALL(versioned)
#else
#define SYSCALL_LARGEFILE(plain, versioned) SYSCALL(plain)
#endif

  
















  Allow(SYSCALL(futex));
  
  
#if SYSCALL_EXISTS(socketcall)
  Allow(SYSCALL(socketcall));
#else
  Allow(SYSCALL(recvmsg));
  Allow(SYSCALL(sendmsg));
#endif

  
  
  
#if SYSCALL_EXISTS(mmap2)
  Allow(SYSCALL(mmap2));
#else
  Allow(SYSCALL(mmap));
#endif

  
#ifdef MOZ_WIDGET_GONK
  Allow(SYSCALL(clock_gettime));
  Allow(SYSCALL(epoll_wait));
  Allow(SYSCALL(gettimeofday));
#endif
  Allow(SYSCALL(read));
  Allow(SYSCALL(write));
  
#if SYSCALL_EXISTS(_llseek)
  Allow(SYSCALL(_llseek));
#endif
  Allow(SYSCALL(lseek));
  
  Allow(SYSCALL(ftruncate));
#if SYSCALL_EXISTS(ftruncate64)
  Allow(SYSCALL(ftruncate64));
#endif

  


  Allow(SYSCALL(ioctl));
  Allow(SYSCALL(close));
  Allow(SYSCALL(munmap));
  Allow(SYSCALL(mprotect));
  Allow(SYSCALL(writev));
  Allow(SYSCALL(clone));
  Allow(SYSCALL(brk));
#if SYSCALL_EXISTS(set_thread_area)
  Allow(SYSCALL(set_thread_area));
#endif

  Allow(SYSCALL(getpid));
  Allow(SYSCALL(gettid));
  Allow(SYSCALL(getrusage));
  Allow(SYSCALL(madvise));
  Allow(SYSCALL(dup));
  Allow(SYSCALL(nanosleep));
  Allow(SYSCALL(poll));
  
#if SYSCALL_EXISTS(_newselect)
  Allow(SYSCALL(_newselect));
#else
  Allow(SYSCALL(select));
#endif
  
#if SYSCALL_EXISTS(getuid32)
  Allow(SYSCALL(getuid32));
  Allow(SYSCALL(geteuid32));
#else
  Allow(SYSCALL(getuid));
  Allow(SYSCALL(geteuid));
#endif
  
  
  
#if SYSCALL_EXISTS(sigreturn)
  Allow(SYSCALL(sigreturn));
#endif
  Allow(SYSCALL(rt_sigreturn));
  Allow(SYSCALL_LARGEFILE(fcntl, fcntl64));

  
  
  
  Allow(SYSCALL_LARGEFILE(fstat, fstat64));
  Allow(SYSCALL_LARGEFILE(stat, stat64));
  Allow(SYSCALL_LARGEFILE(lstat, lstat64));
  
#if !SYSCALL_EXISTS(socketcall)
  Allow(SYSCALL(socketpair));
  Deny(EACCES, SYSCALL(socket));
#endif
  Allow(SYSCALL(open));
  Allow(SYSCALL(readlink)); 
  Allow(SYSCALL(prctl));
  Allow(SYSCALL(access));
  Allow(SYSCALL(unlink));
  Allow(SYSCALL(fsync));
  Allow(SYSCALL(msync));

  
  Allow(SYSCALL(getpriority));
  Allow(SYSCALL(sched_get_priority_min));
  Allow(SYSCALL(sched_get_priority_max));
  Allow(SYSCALL(setpriority));
  
  
#if SYSCALL_EXISTS(sigprocmask)
  Allow(SYSCALL(sigprocmask));
#endif
  Allow(SYSCALL(rt_sigprocmask));

  
  
  Allow(SYSCALL_WITH_ARG(tgkill, 0, getpid()));

  
#ifdef MOZ_WIDGET_GONK
#if !SYSCALL_EXISTS(socketcall)
  Allow(SYSCALL(sendto));
  Allow(SYSCALL(recvfrom));
#endif
  Allow(SYSCALL_LARGEFILE(getdents, getdents64));
  Allow(SYSCALL(epoll_ctl));
  Allow(SYSCALL(sched_yield));
  Allow(SYSCALL(sched_getscheduler));
  Allow(SYSCALL(sched_setscheduler));
  Allow(SYSCALL(sigaltstack));
#endif

  
  
#if SYSCALL_EXISTS(sigaction)
  Allow(SYSCALL(sigaction));
#endif
  Allow(SYSCALL(rt_sigaction));
#ifdef ARM_SYSCALL
  Allow(ARM_SYSCALL(breakpoint));
  Allow(ARM_SYSCALL(cacheflush));
  Allow(ARM_SYSCALL(usr26));
  Allow(ARM_SYSCALL(usr32));
  Allow(ARM_SYSCALL(set_tls));
#endif

  
  Allow(SYSCALL(restart_syscall));

  
  
#ifndef MOZ_WIDGET_GONK
  Allow(SYSCALL(stat));
  Allow(SYSCALL(getdents));
  Allow(SYSCALL(lstat));
  Allow(SYSCALL(mmap));
  Allow(SYSCALL(openat));
  Allow(SYSCALL(fcntl));
  Allow(SYSCALL(fstat));
  Allow(SYSCALL(readlink));
  Allow(SYSCALL(getsockname));
  Allow(SYSCALL(getuid));
  Allow(SYSCALL(geteuid));
  Allow(SYSCALL(mkdir));
  Allow(SYSCALL(getcwd));
  Allow(SYSCALL(readahead));
  Allow(SYSCALL(pread64));
  Allow(SYSCALL(statfs));
  Allow(SYSCALL(pipe));
  Allow(SYSCALL(getrlimit));
  Allow(SYSCALL(shutdown));
  Allow(SYSCALL(getpeername));
  Allow(SYSCALL(eventfd2));
  Allow(SYSCALL(clock_getres));
  Allow(SYSCALL(sysinfo));
  Allow(SYSCALL(getresuid));
  Allow(SYSCALL(umask));
  Allow(SYSCALL(getresgid));
  Allow(SYSCALL(poll));
  Allow(SYSCALL(getegid));
  Allow(SYSCALL(inotify_init1));
  Allow(SYSCALL(wait4));
  Allow(SYSCALL(shmctl));
  Allow(SYSCALL(set_robust_list));
  Allow(SYSCALL(rmdir));
  Allow(SYSCALL(recvfrom));
  Allow(SYSCALL(shmdt));
  Allow(SYSCALL(pipe2));
  Allow(SYSCALL(setsockopt));
  Allow(SYSCALL(shmat));
  Allow(SYSCALL(set_tid_address));
  Allow(SYSCALL(inotify_add_watch));
  Allow(SYSCALL(rt_sigprocmask));
  Allow(SYSCALL(shmget));
  Allow(SYSCALL(getgid));
  Allow(SYSCALL(utime));
  Allow(SYSCALL(arch_prctl));
  Allow(SYSCALL(sched_getaffinity));
  
  Allow(SYSCALL(socket));
  Allow(SYSCALL(chmod));
  Allow(SYSCALL(execve));
  Allow(SYSCALL(rename));
  Allow(SYSCALL(symlink));
  Allow(SYSCALL(connect));
  Allow(SYSCALL(quotactl));
  Allow(SYSCALL(kill));
  Allow(SYSCALL(sendto));
#endif

  
  
  Allow(SYSCALL(uname));
  Allow(SYSCALL(exit_group));
  Allow(SYSCALL(exit));
}

}
