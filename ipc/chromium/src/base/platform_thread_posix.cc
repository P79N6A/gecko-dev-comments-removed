



#include "base/platform_thread.h"

#include <errno.h>
#include <sched.h>

#if defined(OS_MACOSX)
#include <mach/mach.h>
#elif defined(OS_NETBSD)
#include <lwp.h>
#elif defined(OS_LINUX)
#include <sys/syscall.h>
#include <sys/prctl.h>
#elif defined(OS_FREEBSD) && !defined(__GLIBC__)
#include <sys/param.h>
#include <sys/thr.h>
#endif

#if !defined(OS_MACOSX)
#include <unistd.h>
#endif

#if defined(OS_BSD) && !defined(OS_NETBSD) && !defined(__GLIBC__)
#include <pthread_np.h>
#endif

#if defined(OS_MACOSX)
namespace base {
void InitThreading();
}  
#endif

static void* ThreadFunc(void* closure) {
  PlatformThread::Delegate* delegate =
      static_cast<PlatformThread::Delegate*>(closure);
  delegate->ThreadMain();
  return NULL;
}


PlatformThreadId PlatformThread::CurrentId() {
  
  
#if defined(OS_MACOSX)
  mach_port_t port = mach_thread_self();
  mach_port_deallocate(mach_task_self(), port);
  return port;
#elif defined(OS_LINUX)
  return syscall(__NR_gettid);
#elif defined(OS_OPENBSD) || defined(__GLIBC__)
  return (intptr_t) (pthread_self());
#elif defined(OS_NETBSD)
  return _lwp_self();
#elif defined(OS_DRAGONFLY)
  return lwp_gettid();
#elif defined(OS_FREEBSD)
#  if __FreeBSD_version > 900030
    return pthread_getthreadid_np();
#  else
    long lwpid;
    thr_self(&lwpid);
    return lwpid;
#  endif
#endif
}


void PlatformThread::YieldCurrentThread() {
  sched_yield();
}


void PlatformThread::Sleep(int duration_ms) {
  struct timespec sleep_time, remaining;

  
  sleep_time.tv_sec = duration_ms / 1000;
  duration_ms -= sleep_time.tv_sec * 1000;

  
  sleep_time.tv_nsec = duration_ms * 1000 * 1000;  

  while (nanosleep(&sleep_time, &remaining) == -1 && errno == EINTR)
    sleep_time = remaining;
}

#ifndef OS_MACOSX



void PlatformThread::SetName(const char* name) {
  
  
  
  
  if (PlatformThread::CurrentId() == getpid())
    return;

  
  
  
  
  
#if defined(OS_LINUX)
  prctl(PR_SET_NAME, reinterpret_cast<uintptr_t>(name), 0, 0, 0); 
#elif defined(OS_NETBSD)
  pthread_setname_np(pthread_self(), "%s", (void *)name);
#elif defined(OS_BSD) && !defined(__GLIBC__)
  pthread_set_name_np(pthread_self(), name);
#else
#endif
}
#endif 

namespace {

bool CreateThread(size_t stack_size, bool joinable,
                  PlatformThread::Delegate* delegate,
                  PlatformThreadHandle* thread_handle) {
#if defined(OS_MACOSX)
  base::InitThreading();
#endif  

  bool success = false;
  pthread_attr_t attributes;
  pthread_attr_init(&attributes);

  
  
  if (!joinable) {
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
  }

  if (stack_size > 0)
    pthread_attr_setstacksize(&attributes, stack_size);

  success = !pthread_create(thread_handle, &attributes, ThreadFunc, delegate);

  pthread_attr_destroy(&attributes);
  return success;
}

}  


bool PlatformThread::Create(size_t stack_size, Delegate* delegate,
                            PlatformThreadHandle* thread_handle) {
  return CreateThread(stack_size, true ,
                      delegate, thread_handle);
}


bool PlatformThread::CreateNonJoinable(size_t stack_size, Delegate* delegate) {
  PlatformThreadHandle unused;

  bool result = CreateThread(stack_size, false ,
                             delegate, &unused);
  return result;
}


void PlatformThread::Join(PlatformThreadHandle thread_handle) {
  pthread_join(thread_handle, NULL);
}
