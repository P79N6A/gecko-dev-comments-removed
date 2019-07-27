



#ifndef SANDBOX_SRC_WIN2K_THREADPOOL_H_
#define SANDBOX_SRC_WIN2K_THREADPOOL_H_

#include <list>
#include <algorithm>
#include "sandbox/win/src/crosscall_server.h"

namespace sandbox {












class Win2kThreadPool : public ThreadProvider {
 public:
  Win2kThreadPool() {
    ::InitializeCriticalSection(&lock_);
  }
  virtual ~Win2kThreadPool();

  virtual bool RegisterWait(const void* cookie, HANDLE waitable_object,
                            CrossCallIPCCallback callback,
                            void* context);

  virtual bool UnRegisterWaits(void* cookie);

  
  
  size_t OutstandingWaits();

 private:
  
  struct PoolObject {
    const void* cookie;
    HANDLE wait;
  };
  
  typedef std::list<PoolObject> PoolObjects;
  PoolObjects pool_objects_;
  
  CRITICAL_SECTION lock_;
  DISALLOW_COPY_AND_ASSIGN(Win2kThreadPool);
};

}  

#endif  
