







#ifndef BASE_PLATFORM_THREAD_H_
#define BASE_PLATFORM_THREAD_H_

#include "base/basictypes.h"





#if defined(OS_WIN)
#include <windows.h>
typedef DWORD PlatformThreadId;
typedef void* PlatformThreadHandle;  
#elif defined(OS_POSIX)
#include <pthread.h>
typedef pthread_t PlatformThreadHandle;
#if defined(OS_LINUX)
#include <unistd.h>
typedef pid_t PlatformThreadId;
#elif defined(OS_MACOSX)
#include <mach/mach.h>
typedef mach_port_t PlatformThreadId;
#endif
#endif


class PlatformThread {
 public:
  
  static PlatformThreadId CurrentId();

  
  static void YieldCurrentThread();

  
  static void Sleep(int duration_ms);

  
  static void SetName(const char* name);

  
  
  class Delegate {
   public:
    virtual ~Delegate() {}
    virtual void ThreadMain() = 0;
  };

  
  
  
  
  
  
  
  
  static bool Create(size_t stack_size, Delegate* delegate,
                     PlatformThreadHandle* thread_handle);

  
  
  
  static bool CreateNonJoinable(size_t stack_size, Delegate* delegate);

  
  
  
  static void Join(PlatformThreadHandle thread_handle);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PlatformThread);
};

#endif  
