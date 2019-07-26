




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
  ALLOW_SYSCALL(geteuid32), \
  ALLOW_SYSCALL(sigreturn), \
  ALLOW_SYSCALL(fcntl64),
#elif defined(__i386__)
#define SECCOMP_WHITELIST_ARCH_LOW \
  ALLOW_SYSCALL(_llseek), \
  ALLOW_SYSCALL(getuid32), \
  ALLOW_SYSCALL(geteuid32), \
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
# ifdef __NR_sigaction
#  define SECCOMP_WHITELIST_PROFILING \
  ALLOW_SYSCALL(sigaction), \
  ALLOW_SYSCALL(rt_sigaction), \
  ALLOW_SYSCALL(tgkill),
# else
#  define SECCOMP_WHITELIST_PROFILING \
  ALLOW_SYSCALL(rt_sigaction), \
  ALLOW_SYSCALL(tgkill),
# endif
#else
# define SECCOMP_WHITELIST_PROFILING
#endif


#if defined(__arm__)
#define SECCOMP_WHITELIST_ARCH_TOREMOVE \
  ALLOW_SYSCALL(fstat64), \
  ALLOW_SYSCALL(stat64), \
  ALLOW_SYSCALL(lstat64), \
  ALLOW_SYSCALL(sigprocmask),
#elif defined(__i386__)
#define SECCOMP_WHITELIST_ARCH_TOREMOVE \
  ALLOW_SYSCALL(fstat64), \
  ALLOW_SYSCALL(stat64), \
  ALLOW_SYSCALL(lstat64), \
  ALLOW_SYSCALL(sigprocmask),
#else
#define SECCOMP_WHITELIST_ARCH_TOREMOVE
#endif


#if defined(__arm__)
#define SECCOMP_WHITELIST_ARCH_DESKTOP_LINUX
#elif defined(__i386__)
#define SECCOMP_WHITELIST_ARCH_DESKTOP_LINUX
#elif defined(__x86_64__)
#define SECCOMP_WHITELIST_ARCH_DESKTOP_LINUX
#else
#define SECCOMP_WHITELIST_ARCH_DESKTOP_LINUX
#endif


#if defined(MOZ_B2G)

#define SECCOMP_WHITELIST_B2G_HIGH \
  ALLOW_SYSCALL(gettimeofday),

#define SECCOMP_WHITELIST_B2G_MED \
  ALLOW_SYSCALL(clock_gettime), \
  ALLOW_SYSCALL(getpid), \
  ALLOW_SYSCALL(rt_sigreturn), \
  ALLOW_SYSCALL(epoll_wait),

#define SECCOMP_WHITELIST_B2G_LOW \
  ALLOW_SYSCALL(getdents64), \
  ALLOW_SYSCALL(sched_setscheduler),

#else
#define SECCOMP_WHITELIST_B2G_HIGH
#define SECCOMP_WHITELIST_B2G_MED
#define SECCOMP_WHITELIST_B2G_LOW
#endif




#if defined(MOZ_CONTENT_SANDBOX) && !defined(MOZ_B2G) && defined(XP_UNIX) && !defined(XP_MACOSX)


#define SECCOMP_WHITELIST_DESKTOP_LINUX_TO_REMOVE \
  ALLOW_SYSCALL(socket), \
  ALLOW_SYSCALL(chmod), \
  ALLOW_SYSCALL(execve), \
  ALLOW_SYSCALL(rename), \
  ALLOW_SYSCALL(symlink), \
  ALLOW_SYSCALL(connect), \
  ALLOW_SYSCALL(quotactl), \
  ALLOW_SYSCALL(kill), \
  ALLOW_SYSCALL(sendto),

#define SECCOMP_WHITELIST_DESKTOP_LINUX \
  SECCOMP_WHITELIST_ARCH_DESKTOP_LINUX \
  ALLOW_SYSCALL(stat), \
  ALLOW_SYSCALL(getdents), \
  ALLOW_SYSCALL(lstat), \
  ALLOW_SYSCALL(mmap), \
  ALLOW_SYSCALL(openat), \
  ALLOW_SYSCALL(fcntl), \
  ALLOW_SYSCALL(fstat), \
  ALLOW_SYSCALL(readlink), \
  ALLOW_SYSCALL(getsockname), \
  ALLOW_SYSCALL(recvmsg), \
  ALLOW_SYSCALL(uname), \
  /* duplicate rt_sigaction in SECCOMP_WHITELIST_PROFILING */ \
  ALLOW_SYSCALL(rt_sigaction), \
  ALLOW_SYSCALL(getuid), \
  ALLOW_SYSCALL(geteuid), \
  ALLOW_SYSCALL(mkdir), \
  ALLOW_SYSCALL(getcwd), \
  ALLOW_SYSCALL(readahead), \
  ALLOW_SYSCALL(pread64), \
  ALLOW_SYSCALL(statfs), \
  ALLOW_SYSCALL(pipe), \
  ALLOW_SYSCALL(ftruncate), \
  ALLOW_SYSCALL(getrlimit), \
  ALLOW_SYSCALL(shutdown), \
  ALLOW_SYSCALL(getpeername), \
  ALLOW_SYSCALL(eventfd2), \
  ALLOW_SYSCALL(clock_getres), \
  ALLOW_SYSCALL(sysinfo), \
  ALLOW_SYSCALL(getresuid), \
  ALLOW_SYSCALL(umask), \
  ALLOW_SYSCALL(getresgid), \
  ALLOW_SYSCALL(poll), \
  ALLOW_SYSCALL(getegid), \
  ALLOW_SYSCALL(inotify_init1), \
  ALLOW_SYSCALL(wait4), \
  ALLOW_SYSCALL(shmctl), \
  ALLOW_SYSCALL(set_robust_list), \
  ALLOW_SYSCALL(rmdir), \
  ALLOW_SYSCALL(recvfrom), \
  ALLOW_SYSCALL(shmdt), \
  ALLOW_SYSCALL(pipe2), \
  ALLOW_SYSCALL(setsockopt), \
  ALLOW_SYSCALL(shmat), \
  ALLOW_SYSCALL(set_tid_address), \
  ALLOW_SYSCALL(inotify_add_watch), \
  ALLOW_SYSCALL(rt_sigprocmask), \
  ALLOW_SYSCALL(shmget), \
  ALLOW_SYSCALL(getgid), \
  ALLOW_SYSCALL(utime), \
  ALLOW_SYSCALL(arch_prctl), \
  ALLOW_SYSCALL(sched_getaffinity), \
  SECCOMP_WHITELIST_DESKTOP_LINUX_TO_REMOVE
#else
#define SECCOMP_WHITELIST_DESKTOP_LINUX
#endif



















#define SECCOMP_WHITELIST \
  /* These are calls we're ok to allow */ \
  SECCOMP_WHITELIST_ARCH_HIGH \
  SECCOMP_WHITELIST_B2G_HIGH \
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
  SECCOMP_WHITELIST_B2G_MED \
  ALLOW_SYSCALL(gettid), \
  ALLOW_SYSCALL(getrusage), \
  ALLOW_SYSCALL(madvise), \
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
  ALLOW_SYSCALL(unlink), \
  ALLOW_SYSCALL(fsync), \
  ALLOW_SYSCALL(socketpair), \
  ALLOW_SYSCALL(sendmsg), \
   \
  ALLOW_SYSCALL(getpriority), \
  ALLOW_SYSCALL(setpriority), \
  SECCOMP_WHITELIST_PROFILING \
  SECCOMP_WHITELIST_B2G_LOW \
   \
  SECCOMP_WHITELIST_ARCH_LAST \
   \
  ALLOW_SYSCALL(restart_syscall), \
   \
   \
  SECCOMP_WHITELIST_DESKTOP_LINUX \
  ALLOW_SYSCALL(exit_group), \
  ALLOW_SYSCALL(exit)

