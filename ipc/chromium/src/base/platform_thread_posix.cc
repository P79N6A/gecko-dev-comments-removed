



#include "base/platform_thread.h"

#include <errno.h>
#include <sched.h>

#if defined(OS_MACOSX)
#include <mach/mach.h>
#elif defined(OS_LINUX)
#include <sys/syscall.h>
#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__DragonFly__)
#include <sys/prctl.h>
#elif !defined(__NetBSD__)
#include <pthread_np.h>
#endif
#include <unistd.h>
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
  return mach_thread_self();
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
  
  return (intptr_t)(pthread_self());
#elif defined(OS_LINUX)
  return syscall(__NR_gettid);
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

  
  
  
  
  
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
  pthread_set_name_np(pthread_self(), name);
#elif defined(__NetBSD__)
  pthread_setname_np(pthread_self(), "%s", (void *)name);
#else
  prctl(PR_SET_NAME, reinterpret_cast<uintptr_t>(name), 0, 0, 0); 
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
