































#ifndef GMP_PLATFORM_h_
#define GMP_PLATFORM_h_

#include "gmp-errors.h"
#include "gmp-storage.h"
#include <stdint.h>



class GMPTask {
public:
  virtual void Destroy() = 0; 
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
  virtual void Destroy() = 0; 
};



typedef int64_t GMPTimestamp;

typedef GMPErr (*GMPCreateThreadPtr)(GMPThread** aThread);
typedef GMPErr (*GMPRunOnMainThreadPtr)(GMPTask* aTask);
typedef GMPErr (*GMPSyncRunOnMainThreadPtr)(GMPTask* aTask);
typedef GMPErr (*GMPCreateMutexPtr)(GMPMutex** aMutex);


typedef GMPErr (*GMPCreateRecordPtr)(const char* aRecordName,
                                     uint32_t aRecordNameSize,
                                     GMPRecord** aOutRecord,
                                     GMPRecordClient* aClient);


typedef GMPErr (*GMPSetTimerOnMainThreadPtr)(GMPTask* aTask, int64_t aTimeoutMS);
typedef GMPErr (*GMPGetCurrentTimePtr)(GMPTimestamp* aOutTime);

struct GMPPlatformAPI {
  
  
  
  
  uint16_t version; 

  GMPCreateThreadPtr createthread;
  GMPRunOnMainThreadPtr runonmainthread;
  GMPSyncRunOnMainThreadPtr syncrunonmainthread;
  GMPCreateMutexPtr createmutex;
  GMPCreateRecordPtr createrecord;
  GMPSetTimerOnMainThreadPtr settimer;
  GMPGetCurrentTimePtr getcurrenttime;
};

#endif 
