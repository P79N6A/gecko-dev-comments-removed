







#ifndef BASE_THREADING_PLATFORM_THREAD_H_
#define BASE_THREADING_PLATFORM_THREAD_H_

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/time/time.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_POSIX)
#include <pthread.h>
#include <unistd.h>
#endif

namespace base {

#if defined(OS_WIN)
typedef DWORD PlatformThreadId;
#elif defined(OS_POSIX)
typedef pid_t PlatformThreadId;
#endif

class PlatformThreadHandle {
 public:
#if defined(OS_WIN)
  typedef void* Handle;
#elif defined(OS_POSIX)
  typedef pthread_t Handle;
#endif

  PlatformThreadHandle()
      : handle_(0),
        id_(0) {
  }

  explicit PlatformThreadHandle(Handle handle)
      : handle_(handle),
        id_(0) {
  }

  PlatformThreadHandle(Handle handle,
                       PlatformThreadId id)
      : handle_(handle),
        id_(id) {
  }

  bool is_equal(const PlatformThreadHandle& other) {
    return handle_ == other.handle_;
  }

  bool is_null() {
    return !handle_;
  }

  Handle platform_handle() {
    return handle_;
  }

 private:
  friend class PlatformThread;

  Handle handle_;
  PlatformThreadId id_;
};

const PlatformThreadId kInvalidThreadId(0);


enum ThreadPriority{
  kThreadPriority_Normal,
  
  kThreadPriority_RealtimeAudio,
  
  kThreadPriority_Display,
  
  kThreadPriority_Background
};


class BASE_EXPORT PlatformThread {
 public:
  
  
  class BASE_EXPORT Delegate {
   public:
    virtual void ThreadMain() = 0;

   protected:
    virtual ~Delegate() {}
  };

  
  static PlatformThreadId CurrentId();

  
  static PlatformThreadHandle CurrentHandle();

  
  static void YieldCurrentThread();

  
  static void Sleep(base::TimeDelta duration);

  
  
  
  static void SetName(const char* name);

  
  static const char* GetName();

  
  
  
  
  
  
  
  
  static bool Create(size_t stack_size, Delegate* delegate,
                     PlatformThreadHandle* thread_handle);

  
  
  
  
  
  static bool CreateWithPriority(size_t stack_size, Delegate* delegate,
                                 PlatformThreadHandle* thread_handle,
                                 ThreadPriority priority);

  
  
  
  static bool CreateNonJoinable(size_t stack_size, Delegate* delegate);

  
  
  
  static void Join(PlatformThreadHandle thread_handle);

  static void SetThreadPriority(PlatformThreadHandle handle,
                                ThreadPriority priority);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PlatformThread);
};

}  

#endif  
