



#include <errno.h>
#include <fcntl.h>
#include <linux/unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "base/posix/eintr_wrapper.h"
#include "sandbox/linux/seccomp-bpf/sandbox_bpf.h"
#include "sandbox/linux/services/linux_syscalls.h"

using sandbox::ErrorCode;
using sandbox::SandboxBPF;
using sandbox::arch_seccomp_data;

#define ERR EPERM





#define _exit(x) do { } while (0)

namespace {

bool SendFds(int transport, const void *buf, size_t len, ...) {
  int count = 0;
  va_list ap;
  va_start(ap, len);
  while (va_arg(ap, int) >= 0) {
    ++count;
  }
  va_end(ap);
  if (!count) {
    return false;
  }
  char cmsg_buf[CMSG_SPACE(count*sizeof(int))];
  memset(cmsg_buf, 0, sizeof(cmsg_buf));
  struct iovec  iov[2] = { { 0 } };
  struct msghdr msg    = { 0 };
  int dummy            = 0;
  iov[0].iov_base      = &dummy;
  iov[0].iov_len       = sizeof(dummy);
  if (buf && len > 0) {
    iov[1].iov_base    = const_cast<void *>(buf);
    iov[1].iov_len     = len;
  }
  msg.msg_iov          = iov;
  msg.msg_iovlen       = (buf && len > 0) ? 2 : 1;
  msg.msg_control      = cmsg_buf;
  msg.msg_controllen   = CMSG_LEN(count*sizeof(int));
  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level     = SOL_SOCKET;
  cmsg->cmsg_type      = SCM_RIGHTS;
  cmsg->cmsg_len       = CMSG_LEN(count*sizeof(int));
  va_start(ap, len);
  for (int i = 0, fd; (fd = va_arg(ap, int)) >= 0; ++i) {
    (reinterpret_cast<int *>(CMSG_DATA(cmsg)))[i] = fd;
  }
  return sendmsg(transport, &msg, 0) ==
      static_cast<ssize_t>(sizeof(dummy) + ((buf && len > 0) ? len : 0));
}

bool GetFds(int transport, void *buf, size_t *len, ...) {
  int count = 0;
  va_list ap;
  va_start(ap, len);
  for (int *fd; (fd = va_arg(ap, int *)) != NULL; ++count) {
    *fd = -1;
  }
  va_end(ap);
  if (!count) {
    return false;
  }
  char cmsg_buf[CMSG_SPACE(count*sizeof(int))];
  memset(cmsg_buf, 0, sizeof(cmsg_buf));
  struct iovec iov[2] = { { 0 } };
  struct msghdr msg   = { 0 };
  int err;
  iov[0].iov_base     = &err;
  iov[0].iov_len      = sizeof(int);
  if (buf && len && *len > 0) {
    iov[1].iov_base   = buf;
    iov[1].iov_len    = *len;
  }
  msg.msg_iov         = iov;
  msg.msg_iovlen      = (buf && len && *len > 0) ? 2 : 1;
  msg.msg_control     = cmsg_buf;
  msg.msg_controllen  = CMSG_LEN(count*sizeof(int));
  ssize_t bytes = recvmsg(transport, &msg, 0);
  if (len) {
    *len = bytes > static_cast<int>(sizeof(int)) ? bytes - sizeof(int) : 0;
  }
  if (bytes != static_cast<ssize_t>(sizeof(int) + iov[1].iov_len)) {
    if (bytes >= 0) {
      errno = 0;
    }
    return false;
  }
  if (err) {
    
    
    
    errno = err;
    return false;
  }
  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  if ((msg.msg_flags & (MSG_TRUNC|MSG_CTRUNC)) ||
      !cmsg                                    ||
      cmsg->cmsg_level != SOL_SOCKET           ||
      cmsg->cmsg_type  != SCM_RIGHTS           ||
      cmsg->cmsg_len   != CMSG_LEN(count*sizeof(int))) {
    errno = EBADF;
    return false;
  }
  va_start(ap, len);
  for (int *fd, i = 0; (fd = va_arg(ap, int *)) != NULL; ++i) {
    *fd = (reinterpret_cast<int *>(CMSG_DATA(cmsg)))[i];
  }
  va_end(ap);
  return true;
}








char *itoa_r(int i, char *buf, size_t sz) {
  
  size_t n = 1;
  if (n > sz) {
    return NULL;
  }

  
  char *start = buf;
  int minint = 0;
  if (i < 0) {
    
    if (++n > sz) {
      *start = '\000';
      return NULL;
    }
    *start++ = '-';

    
    if (i == -i) {
      
      minint = 1;
      i = -(i + 1);
    } else {
      
      i = -i;
    }
  }

  
  
  char *ptr = start;
  do {
    
    if (++n > sz) {
      buf = NULL;
      goto truncate;
    }

    
    
    
    
    *ptr++ = i%10 + '0' + minint;
    minint = 0;
    i /= 10;
  } while (i);
 truncate:  
  *ptr = '\000';

  
  
  
  
  while (--ptr > start) {
    char ch = *ptr;
    *ptr = *start;
    *start++ = ch;
  }
  return buf;
}







intptr_t DefaultHandler(const struct arch_seccomp_data& data, void *) {
  static const char msg0[] = "Disallowed system call #";
  static const char msg1[] = "\n";
  char buf[sizeof(msg0) - 1 + 25 + sizeof(msg1)];

  *buf = '\000';
  strncat(buf, msg0, sizeof(buf) - 1);

  char *ptr = strrchr(buf, '\000');
  itoa_r(data.nr, ptr, sizeof(buf) - (ptr - buf));

  ptr = strrchr(ptr, '\000');
  strncat(ptr, msg1, sizeof(buf) - (ptr - buf));

  ptr = strrchr(ptr, '\000');
  if (HANDLE_EINTR(write(2, buf, ptr - buf))) { }

  return -ERR;
}

ErrorCode Evaluator(SandboxBPF* sandbox, int sysno, void *) {
  switch (sysno) {
#if defined(__NR_accept)
  case __NR_accept: case __NR_accept4:
#endif
  case __NR_alarm:
  case __NR_brk:
  case __NR_clock_gettime:
  case __NR_close:
  case __NR_dup: case __NR_dup2:
  case __NR_epoll_create: case __NR_epoll_ctl: case __NR_epoll_wait:
  case __NR_exit: case __NR_exit_group:
  case __NR_fcntl:
#if defined(__NR_fcntl64)
  case __NR_fcntl64:
#endif
  case __NR_fdatasync:
  case __NR_fstat:
#if defined(__NR_fstat64)
  case __NR_fstat64:
#endif
  case __NR_ftruncate:
  case __NR_futex:
  case __NR_getdents: case __NR_getdents64:
  case __NR_getegid:
#if defined(__NR_getegid32)
  case __NR_getegid32:
#endif
  case __NR_geteuid:
#if defined(__NR_geteuid32)
  case __NR_geteuid32:
#endif
  case __NR_getgid:
#if defined(__NR_getgid32)
  case __NR_getgid32:
#endif
  case __NR_getitimer: case __NR_setitimer:
#if defined(__NR_getpeername)
  case __NR_getpeername:
#endif
  case __NR_getpid: case __NR_gettid:
#if defined(__NR_getsockname)
  case __NR_getsockname:
#endif
  case __NR_gettimeofday:
  case __NR_getuid:
#if defined(__NR_getuid32)
  case __NR_getuid32:
#endif
#if defined(__NR__llseek)
  case __NR__llseek:
#endif
  case __NR_lseek:
  case __NR_nanosleep:
  case __NR_pipe: case __NR_pipe2:
  case __NR_poll:
  case __NR_pread64: case __NR_preadv:
  case __NR_pwrite64: case __NR_pwritev:
  case __NR_read: case __NR_readv:
  case __NR_restart_syscall:
  case __NR_set_robust_list:
  case __NR_rt_sigaction:
#if defined(__NR_sigaction)
  case __NR_sigaction:
#endif
#if defined(__NR_signal)
  case __NR_signal:
#endif
  case __NR_rt_sigprocmask:
#if defined(__NR_sigprocmask)
  case __NR_sigprocmask:
#endif
#if defined(__NR_shutdown)
  case __NR_shutdown:
#endif
  case __NR_rt_sigreturn:
#if defined(__NR_sigreturn)
  case __NR_sigreturn:
#endif
#if defined(__NR_socketpair)
  case __NR_socketpair:
#endif
  case __NR_time:
  case __NR_uname:
  case __NR_write: case __NR_writev:
    return ErrorCode(ErrorCode::ERR_ALLOWED);

  case __NR_prctl:
    
    return sandbox->Cond(1, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL,
                         PR_SET_DUMPABLE,
                         ErrorCode(ErrorCode::ERR_ALLOWED),
           sandbox->Cond(1, ErrorCode::TP_32BIT, ErrorCode::OP_EQUAL,
                         PR_GET_DUMPABLE,
                         ErrorCode(ErrorCode::ERR_ALLOWED),
           sandbox->Trap(DefaultHandler, NULL)));

  
  
  
  
#if defined(__NR_sendmsg)
  case __NR_sendmsg: case __NR_sendto:
  case __NR_recvmsg: case __NR_recvfrom:
  case __NR_getsockopt: case __NR_setsockopt:
#elif defined(__NR_socketcall)
  case __NR_socketcall:
#endif
#if defined(__NR_shmat)
  case __NR_shmat: case __NR_shmctl: case __NR_shmdt: case __NR_shmget:
#elif defined(__NR_ipc)
  case __NR_ipc:
#endif
#if defined(__NR_mmap2)
  case __NR_mmap2:
#else
  case __NR_mmap:
#endif
#if defined(__NR_ugetrlimit)
  case __NR_ugetrlimit:
#endif
  case __NR_getrlimit:
  case __NR_ioctl:
  case __NR_clone:
  case __NR_munmap: case __NR_mprotect: case __NR_madvise:
  case __NR_remap_file_pages:
    return ErrorCode(ErrorCode::ERR_ALLOWED);

  
  default:
    return sandbox->Trap(DefaultHandler, NULL);
  }
}

void *ThreadFnc(void *arg) {
  return arg;
}

void *SendmsgStressThreadFnc(void *arg) {
  if (arg) { }
  static const int repetitions = 100;
  static const int kNumFds = 3;
  for (int rep = 0; rep < repetitions; ++rep) {
    int fds[2 + kNumFds];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds)) {
      perror("socketpair()");
      _exit(1);
    }
    size_t len = 4;
    char buf[4];
    if (!SendFds(fds[0], "test", 4, fds[1], fds[1], fds[1], -1) ||
        !GetFds(fds[1], buf, &len, fds+2, fds+3, fds+4, NULL) ||
        len != 4 ||
        memcmp(buf, "test", len) ||
        write(fds[2], "demo", 4) != 4 ||
        read(fds[0], buf, 4) != 4 ||
        memcmp(buf, "demo", 4)) {
      perror("sending/receiving of fds");
      _exit(1);
    }
    for (int i = 0; i < 2+kNumFds; ++i) {
      if (close(fds[i])) {
        perror("close");
        _exit(1);
      }
    }
  }
  return NULL;
}

}  

int main(int argc, char *argv[]) {
  if (argc) { }
  if (argv) { }
  int proc_fd = open("/proc", O_RDONLY|O_DIRECTORY);
  if (SandboxBPF::SupportsSeccompSandbox(proc_fd) !=
      SandboxBPF::STATUS_AVAILABLE) {
    perror("sandbox");
    _exit(1);
  }
  SandboxBPF sandbox;
  sandbox.set_proc_fd(proc_fd);
  sandbox.SetSandboxPolicyDeprecated(Evaluator, NULL);
  if (!sandbox.StartSandbox(SandboxBPF::PROCESS_SINGLE_THREADED)) {
    fprintf(stderr, "StartSandbox() failed");
    _exit(1);
  }

  
  pthread_t thr;
  if (!pthread_create(&thr, NULL, ThreadFnc,
                      reinterpret_cast<void *>(0x1234))) {
    void *ret;
    pthread_join(thr, &ret);
    if (ret != reinterpret_cast<void *>(0x1234)) {
      perror("clone() failed");
      _exit(1);
    }
  } else {
    perror("clone() failed");
    _exit(1);
  }

  
  
  
  signal(SIGALRM, SIG_IGN);
  const struct itimerval tv = { { 0, 0 }, { 0, 5*1000 } };
  const struct timespec tmo = { 0, 100*1000*1000 };
  setitimer(ITIMER_REAL, &tv, NULL);
  nanosleep(&tmo, NULL);

  
  
  if (((errno = 0), !getrlimit(RLIMIT_STACK, NULL)) || errno != EFAULT ||
      ((errno = 0), !getrlimit(RLIMIT_CORE,  NULL)) || errno != ERR) {
    perror("getrlimit()");
    _exit(1);
  }

  
  if (((errno = 0), !ioctl(2, TCGETS,     NULL)) || errno != EFAULT ||
      ((errno = 0), !ioctl(2, TIOCGWINSZ, NULL)) || errno != EFAULT ||
      ((errno = 0), !ioctl(2, TCSETS,     NULL)) || errno != ERR) {
    perror("ioctl()");
    _exit(1);
  }

  
  if (((errno = 0), !prctl(PR_GET_DUMPABLE))    || errno ||
      ((errno = 0),  prctl(PR_SET_DUMPABLE, 1)) || errno ||
      ((errno = 0), !prctl(PR_SET_SECCOMP,  0)) || errno != ERR) {
    perror("prctl()");
    _exit(1);
  }

  
  int fds[3];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds)) {
    perror("socketpair()");
    _exit(1);
  }
  size_t len = 4;
  char buf[4];
  if (!SendFds(fds[0], "test", 4, fds[1], -1) ||
      !GetFds(fds[1], buf, &len, fds+2, NULL) ||
      len != 4 ||
      memcmp(buf, "test", len) ||
      write(fds[2], "demo", 4) != 4 ||
      read(fds[0], buf, 4) != 4 ||
      memcmp(buf, "demo", 4) ||
      close(fds[0]) ||
      close(fds[1]) ||
      close(fds[2])) {
    perror("sending/receiving of fds");
    _exit(1);
  }

  
  int shmid;
  void *addr;
  if ((shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT|0600)) < 0 ||
      (addr = shmat(shmid, NULL, 0)) == reinterpret_cast<void *>(-1) ||
      shmdt(addr) ||
      shmctl(shmid, IPC_RMID, NULL)) {
    perror("sysv IPC");
    _exit(1);
  }

  
  time_t tm = time(NULL);
  printf("Sandbox has been started at %s", ctime(&tm));

  
  static const int kSendmsgStressNumThreads = 10;
  pthread_t sendmsgStressThreads[kSendmsgStressNumThreads];
  for (int i = 0; i < kSendmsgStressNumThreads; ++i) {
    if (pthread_create(sendmsgStressThreads + i, NULL,
                       SendmsgStressThreadFnc, NULL)) {
      perror("pthread_create");
      _exit(1);
    }
  }
  for (int i = 0; i < kSendmsgStressNumThreads; ++i) {
    pthread_join(sendmsgStressThreads[i], NULL);
  }

  return 0;
}
