




#include "linux_seccomp.h"
#include "linux_syscalls.h"






#ifdef ALLOW_ARM_SYSCALL
#define SECCOMP_WHITELIST_ADD \
  ALLOW_ARM_SYSCALL(breakpoint), \
  ALLOW_ARM_SYSCALL(cacheflush), \
  ALLOW_ARM_SYSCALL(usr26), \
  ALLOW_ARM_SYSCALL(usr32), \
  ALLOW_ARM_SYSCALL(set_tls),
#else
#define SECCOMP_WHITELIST_ADD
#endif

















#define SECCOMP_WHITELIST \
  /* These are calls we're ok to allow */ \
  ALLOW_SYSCALL(recv), \
  ALLOW_SYSCALL(msgget), \
  ALLOW_SYSCALL(semget), \
  ALLOW_SYSCALL(read), \
  ALLOW_SYSCALL(write), \
  ALLOW_SYSCALL(brk), \
  /* ioctl() is for GL. Remove when GL proxy is implemented.
   * Additionally ioctl() might be a place where we want to have
   * argument filtering */ \
  ALLOW_SYSCALL(ioctl), \
  ALLOW_SYSCALL(writev), \
  ALLOW_SYSCALL(close), \
  ALLOW_SYSCALL(clone), \
  ALLOW_SYSCALL(clock_gettime), \
  ALLOW_SYSCALL(lseek), \
  ALLOW_SYSCALL(_llseek), \
  ALLOW_SYSCALL(gettimeofday), \
  ALLOW_SYSCALL(getpid), \
  ALLOW_SYSCALL(gettid), \
  ALLOW_SYSCALL(getrusage), \
  ALLOW_SYSCALL(madvise), \
  ALLOW_SYSCALL(rt_sigreturn), \
  ALLOW_SYSCALL(sigreturn), \
  ALLOW_SYSCALL(epoll_wait), \
  ALLOW_SYSCALL(futex), \
  ALLOW_SYSCALL(fcntl64), \
  ALLOW_SYSCALL(munmap), \
  ALLOW_SYSCALL(mmap2), \
  ALLOW_SYSCALL(mprotect), \
  ALLOW_SYSCALL(dup), \
  ALLOW_SYSCALL(getuid32), \
  ALLOW_SYSCALL(nanosleep), \
   \
   \
   \
  ALLOW_SYSCALL(open), \
  ALLOW_SYSCALL(fstat64), \
  ALLOW_SYSCALL(stat64), \
  ALLOW_SYSCALL(prctl), \
  ALLOW_SYSCALL(access), \
  ALLOW_SYSCALL(getdents64), \
  ALLOW_SYSCALL(unlink), \
   \
  ALLOW_SYSCALL(getpriority), \
  ALLOW_SYSCALL(setpriority), \
  ALLOW_SYSCALL(sigprocmask), \
  ALLOW_SYSCALL(sched_setscheduler), \
   \
  SECCOMP_WHITELIST_ADD \
   \
  ALLOW_SYSCALL(restart_syscall), \
  ALLOW_SYSCALL(exit_group), \
  ALLOW_SYSCALL(exit)

