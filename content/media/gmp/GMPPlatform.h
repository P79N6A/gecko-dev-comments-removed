




#ifndef GMPPlatform_h_
#define GMPPlatform_h_

#include "mozilla/Mutex.h"
#include "gmp-platform.h"
#include "base/thread.h"

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

  
  virtual void Post(GMPTask* aTask) MOZ_OVERRIDE;
  virtual void Join() MOZ_OVERRIDE;

private:
  Mutex mMutex;
  base::Thread mThread;
};

class GMPMutexImpl : public GMPMutex
{
public:
  GMPMutexImpl();
  virtual ~GMPMutexImpl();

  
  virtual void Acquire() MOZ_OVERRIDE;
  virtual void Release() MOZ_OVERRIDE;
  virtual void Destroy() MOZ_OVERRIDE;

private:
  Monitor mMonitor;
};

} 
} 

#endif 
