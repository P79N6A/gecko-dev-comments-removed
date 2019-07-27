




#ifndef mozilla_CrossProcessMutex_h
#define mozilla_CrossProcessMutex_h

#include "base/process.h"
#include "mozilla/Mutex.h"

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <pthread.h>
#include "SharedMemoryBasic.h"
#include "mozilla/Atomics.h"
#include "nsAutoPtr.h"
#endif

namespace IPC {
template<typename T>
struct ParamTraits;
}











namespace mozilla {
#if defined(OS_WIN)
typedef HANDLE CrossProcessMutexHandle;
#elif defined(OS_LINUX) || defined(OS_MACOSX)
typedef mozilla::ipc::SharedMemoryBasic::Handle CrossProcessMutexHandle;
#else


typedef uintptr_t CrossProcessMutexHandle;
#endif

class CrossProcessMutex
{
public:
  



  explicit CrossProcessMutex(const char* aName);
  




  explicit CrossProcessMutex(CrossProcessMutexHandle aHandle);

  


  ~CrossProcessMutex();

  








  void Lock();

  






  void Unlock();

  






  CrossProcessMutexHandle ShareToProcess(base::ProcessId aTargetPid);

private:
  friend struct IPC::ParamTraits<CrossProcessMutex>;

  CrossProcessMutex();
  CrossProcessMutex(const CrossProcessMutex&);
  CrossProcessMutex &operator=(const CrossProcessMutex&);

#if defined(OS_WIN)
  HANDLE mMutex;
#elif defined(OS_LINUX) || defined(OS_MACOSX)
  nsRefPtr<mozilla::ipc::SharedMemoryBasic> mSharedBuffer;
  pthread_mutex_t* mMutex;
  mozilla::Atomic<int32_t>* mCount;
#endif
};

typedef BaseAutoLock<CrossProcessMutex> CrossProcessMutexAutoLock;
typedef BaseAutoUnlock<CrossProcessMutex> CrossProcessMutexAutoUnlock;

}
#endif
