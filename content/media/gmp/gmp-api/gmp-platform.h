































#ifndef GMP_PLATFORM_h_
#define GMP_PLATFORM_h_

#include "gmp-errors.h"
#include <stdint.h>



class GMPTask {
public:
  virtual ~GMPTask() {}
  virtual void Run() = 0;
};

class GMPThread {
public:
  virtual ~GMPThread() {}
  virtual void Post(GMPTask* aTask) = 0;
  virtual void Join() = 0;
};

class GMPMutex {
public:
  virtual ~GMPMutex() {}
  virtual void Acquire() = 0;
  virtual void Release() = 0;
};

typedef GMPErr (*GMPCreateThreadPtr)(GMPThread** aThread);
typedef GMPErr (*GMPRunOnMainThreadPtr)(GMPTask* aTask);
typedef GMPErr (*GMPSyncRunOnMainThreadPtr)(GMPTask* aTask);
typedef GMPErr (*GMPCreateMutexPtr)(GMPMutex** aMutex);

struct GMPPlatformAPI {
  
  
  
  
  uint16_t version; 

  GMPCreateThreadPtr createthread;
  GMPRunOnMainThreadPtr runonmainthread;
  GMPSyncRunOnMainThreadPtr syncrunonmainthread;
  GMPCreateMutexPtr createmutex;
};

#endif 
