



#ifndef BASE_SYNCHRONIZATION_LOCK_IMPL_H_
#define BASE_SYNCHRONIZATION_LOCK_IMPL_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_POSIX)
#include <pthread.h>
#endif

#include "base/base_export.h"
#include "base/basictypes.h"

namespace base {
namespace internal {




class BASE_EXPORT LockImpl {
 public:
#if defined(OS_WIN)
  typedef CRITICAL_SECTION NativeHandle;
#elif defined(OS_POSIX)
  typedef pthread_mutex_t NativeHandle;
#endif

  LockImpl();
  ~LockImpl();

  
  
  bool Try();

  
  void Lock();

  
  
  void Unlock();

  
  
  
  NativeHandle* native_handle() { return &native_handle_; }

 private:
  NativeHandle native_handle_;

  DISALLOW_COPY_AND_ASSIGN(LockImpl);
};

}  
}  

#endif  
