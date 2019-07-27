




#ifndef GMPPlatform_h_
#define GMPPlatform_h_

#include "mozilla/Mutex.h"
#include "gmp-platform.h"
#include "base/thread.h"
#include "mozilla/ReentrantMonitor.h"

namespace mozilla {
namespace gmp {

class GMPChild;

void InitPlatformAPI(GMPPlatformAPI& aPlatformAPI, GMPChild* aChild);

GMPErr RunOnMainThread(GMPTask* aTask);

class GMPThreadImpl : public GMPThread
{
public:
  GMPThreadImpl();
  virtual ~GMPThreadImpl();

  
  virtual void Post(GMPTask* aTask) override;
  virtual void Join() override;

private:
  Mutex mMutex;
  base::Thread mThread;
};

class GMPMutexImpl : public GMPMutex
{
public:
  GMPMutexImpl();
  virtual ~GMPMutexImpl();

  
  virtual void Acquire() override;
  virtual void Release() override;
  virtual void Destroy() override;

private:
  ReentrantMonitor mMonitor;
};

} 
} 

#endif 
