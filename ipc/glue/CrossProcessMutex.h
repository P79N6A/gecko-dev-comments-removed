




































#ifndef mozilla_CrossProcessMutex_h
#define mozilla_CrossProcessMutex_h

#include "base/process.h"
#include "mozilla/Mutex.h"

namespace IPC {
template<typename T>
struct ParamTraits;
}











namespace mozilla {
#ifdef XP_WIN
typedef HANDLE CrossProcessMutexHandle;
#else


typedef uintptr_t CrossProcessMutexHandle;
#endif

class NS_COM_GLUE CrossProcessMutex
{
public:
  



  CrossProcessMutex(const char* aName);
  




  CrossProcessMutex(CrossProcessMutexHandle aHandle);

  


  ~CrossProcessMutex();

  








  void Lock();

  






  void Unlock();

  






  CrossProcessMutexHandle ShareToProcess(base::ProcessHandle aTarget);

private:
  friend struct IPC::ParamTraits<CrossProcessMutex>;

  CrossProcessMutex();
  CrossProcessMutex(const CrossProcessMutex&);
  CrossProcessMutex &operator=(const CrossProcessMutex&);

#ifdef XP_WIN
  HANDLE mMutex;
#endif
};

typedef BaseAutoLock<CrossProcessMutex> CrossProcessMutexAutoLock;
typedef BaseAutoUnlock<CrossProcessMutex> CrossProcessMutexAutoUnlock;

}
#endif
