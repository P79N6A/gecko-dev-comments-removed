



#ifndef BASE_LOCK_IMPL_H_
#define BASE_LOCK_IMPL_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_POSIX)
#include <pthread.h>
#endif

#include "base/basictypes.h"
#include "base/platform_thread.h"




class LockImpl {
 public:
#if defined(OS_WIN)
  typedef CRITICAL_SECTION OSLockType;
#elif defined(OS_POSIX)
  typedef pthread_mutex_t OSLockType;
#endif

  LockImpl();
  ~LockImpl();

  
  
  bool Try();

  
  void Lock();

  
  
  void Unlock();

  
  
  
  
  
#if defined(NDEBUG) || !defined(OS_WIN)
  void AssertAcquired() const {}
#else
  void AssertAcquired() const;
#endif

  
  
  
#if !defined(OS_WIN)
  OSLockType* os_lock() { return &os_lock_; }
#endif

 private:
  OSLockType os_lock_;

#if !defined(NDEBUG) && defined(OS_WIN)
  
  
  PlatformThreadId owning_thread_id_;
  int32 recursion_count_shadow_;
  bool recursion_used_;      
#endif  

  DISALLOW_COPY_AND_ASSIGN(LockImpl);
};


#endif  
