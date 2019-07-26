




#include "linux_seccomp.h"
#include "linux_syscalls.h"






#if defined(__arm__)
#define SECCOMP_WHITELIST_ARCH_HIGH \
  ALLOW_SYSCALL(msgget), \
  ALLOW_SYSCALL(recv), \
  ALLOW_SYSCALL(mmap2),
#elif defined(__i386__)
#define SECCOMP_WHITELIST_ARCH_HIGH \
  ALLOW_SYSCALL(ipc), \
  ALLOW_SYSCALL(mmap2),
#elif defined(__x86_64__)
#define SECCOMP_WHITELIST_ARCH_HIGH \
  ALLOW_SYSCALL(msgget),
#else
#define SECCOMP_WHITELIST_ARCH_HIGH
#endif


#if defined(__arm__)
#define SECCOMP_WHITELIST_ARCH_LOW \
  ALLOW_SYSCALL(_llseek), \
  ALLOW_SYSCALL(getuid32), \
  ALLOW_SYSCALL(sigreturn), \
  ALLOW_SYSCALL(fcntl64),
#elif defined(__i386__)
#define SECCOMP_WHITELIST_ARCH_LOW \
  ALLOW_SYSCALL(_llseek), \
  ALLOW_SYSCALL(getuid32), \
  ALLOW_SYSCALL(sigreturn), \
  ALLOW_SYSCALL(fcntl64),
#else
#define SECCOMP_WHITELIST_ARCH_LOW
#endif


#if defined(__arm__)
#define SECCOMP_WHITELIST_ARCH_LAST \
  ALLOW_ARM_SYSCALL(breakpoint), \
  ALLOW_ARM_SYSCALL(cacheflush), \
  ALLOW_ARM_SYSCALL(usr26), \
  ALLOW_ARM_SYSCALL(usr32), \
  ALLOW_ARM_SYSCALL(set_tls),
#else
#define SECCOMP_WHITELIST_ARCH_LAST
#endif


#ifdef MOZ_PROFILING
#define SECCOMP_WHITELIST_PROFILING \
  ALLOW_SYSCALL(sigaction), \
  ALLOW_SYSCALL(tgkill),
#else
#define SECCOMP_WHITELIST_PROFILING
#endif


#if defined(__arm__)
#define SECCOMP_WHITELIST_ARCH_TOREMOVE \
  ALLOW_SYSCALL(fstat64), \
  ALLOW_SYSCALL(stat64), \
  ALLOW_SYSCALL(sigprocmask),
#elif defined(__i386__)
#define SECCOMP_WHITELIST_ARCH_TOREMOVE \
  ALLOW_SYSCALL(fstat64), \
  ALLOW_SYSCALL(stat64), \
  ALLOW_SYSCALL(sigprocmask),
#else
#define SECCOMP_WHITELIST_ARCH_TOREMOVE
#endif

















#define SECCOMP_WHITELIST \
  /* These are calls we're ok to allow */ \
  SECCOMP_WHITELIST_ARCH_HIGH \
  ALLOW_SYSCALL(gettimeofday), \
  ALLOW_SYSCALL(read), \
  ALLOW_SYSCALL(write), \
  ALLOW_SYSCALL(lseek), \
  /* ioctl() is for GL. Remove when GL proxy is implemented.
   * Additionally ioctl() might be a place where we want to have
   * argument filtering */ \
  ALLOW_SYSCALL(ioctl), \
  ALLOW_SYSCALL(close), \
  ALLOW_SYSCALL(munmap), \
  ALLOW_SYSCALL(mprotect), \
  ALLOW_SYSCALL(writev), \
  ALLOW_SYSCALL(clone), \
  ALLOW_SYSCALL(brk), \
  ALLOW_SYSCALL(clock_gettime), \
  ALLOW_SYSCALL(getpid), \
  ALLOW_SYSCALL(gettid), \
  ALLOW_SYSCALL(getrusage), \
  ALLOW_SYSCALL(madvise), \
  ALLOW_SYSCALL(rt_sigreturn), \
  ALLOW_SYSCALL(epoll_wait), \
  ALLOW_SYSCALL(futex), \
  ALLOW_SYSCALL(dup), \
  ALLOW_SYSCALL(nanosleep), \
  SECCOMP_WHITELIST_ARCH_LOW \
   \
   \
   \
  SECCOMP_WHITELIST_ARCH_TOREMOVE \
  ALLOW_SYSCALL(open), \
  ALLOW_SYSCALL(prctl), \
  ALLOW_SYSCALL(access), \
  ALLOW_SYSCALL(getdents64), \
  ALLOW_SYSCALL(unlink), \
   \
  ALLOW_SYSCALL(getpriority), \
  ALLOW_SYSCALL(setpriority), \
  ALLOW_SYSCALL(sched_setscheduler), \
  SECCOMP_WHITELIST_PROFILING \
   \
  SECCOMP_WHITELIST_ARCH_LAST \
   \
  ALLOW_SYSCALL(restart_syscall), \
  ALLOW_SYSCALL(exit_group), \
  ALLOW_SYSCALL(exit)

