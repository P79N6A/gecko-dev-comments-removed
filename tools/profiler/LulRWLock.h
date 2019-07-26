





#ifndef LulRWLock_h
#define LulRWLock_h

#include "LulPlatformMacros.h"
#include <pthread.h>






namespace lul {

class LulRWLock {

public:
  LulRWLock();
  ~LulRWLock();
  void WrLock();
  void RdLock();
  void Unlock();

private:
#if defined(LUL_OS_android)
  pthread_mutex_t mLock;
#elif defined(LUL_OS_linux)
  pthread_rwlock_t mLock;
#else
# error "Unsupported OS"
#endif

};

} 

#endif 
