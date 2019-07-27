




#include "GMPPlatform.h"
#include "mozilla/Monitor.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace gmp {

static MessageLoop* sMainLoop = nullptr;


class Runnable MOZ_FINAL
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Runnable)

  Runnable(GMPTask* aTask)
  : mTask(aTask)
  {
    MOZ_ASSERT(mTask);
  }

  void Run()
  {
    mTask->Run();
    mTask->Destroy();
    mTask = nullptr;
  }

private:
  ~Runnable()
  {
  }

  GMPTask* mTask;
};

class SyncRunnable MOZ_FINAL
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SyncRunnable)

  SyncRunnable(GMPTask* aTask, MessageLoop* aMessageLoop)
  : mDone(false)
  , mTask(aTask)
  , mMessageLoop(aMessageLoop)
  , mMonitor("GMPSyncRunnable")
  {
    MOZ_ASSERT(mTask);
    MOZ_ASSERT(mMessageLoop);
  }

  void Post()
  {
    
    
    
    
    MOZ_ASSERT(MessageLoop::current() != sMainLoop);

    mMessageLoop->PostTask(FROM_HERE, NewRunnableMethod(this, &SyncRunnable::Run));
    MonitorAutoLock lock(mMonitor);
    while (!mDone) {
      lock.Wait();
    }
  }

  void Run()
  {
    mTask->Run();
    mTask->Destroy();
    mTask = nullptr;
    MonitorAutoLock lock(mMonitor);
    mDone = true;
    lock.Notify();
  }

private:
  ~SyncRunnable()
  {
  }

  bool mDone;
  GMPTask* mTask;
  MessageLoop* mMessageLoop;
  Monitor mMonitor;
};

GMPErr
CreateThread(GMPThread** aThread)
{
  if (!aThread) {
    return GMPGenericErr;
  }

  *aThread = new GMPThreadImpl();

  return GMPNoErr;
}

GMPErr
RunOnMainThread(GMPTask* aTask)
{
  if (!aTask || !sMainLoop) {
    return GMPGenericErr;
  }

  nsRefPtr<Runnable> r = new Runnable(aTask);
  sMainLoop->PostTask(FROM_HERE, NewRunnableMethod(r.get(), &Runnable::Run));

  return GMPNoErr;
}

GMPErr
SyncRunOnMainThread(GMPTask* aTask)
{
  if (!aTask || !sMainLoop || sMainLoop == MessageLoop::current()) {
    return GMPGenericErr;
  }

  nsRefPtr<SyncRunnable> r = new SyncRunnable(aTask, sMainLoop);

  r->Post();

  return GMPNoErr;
}

GMPErr
CreateMutex(GMPMutex** aMutex)
{
  if (!aMutex) {
    return GMPGenericErr;
  }

  *aMutex = new GMPMutexImpl();

  return GMPNoErr;
}

void
InitPlatformAPI(GMPPlatformAPI& aPlatformAPI)
{
  if (!sMainLoop) {
    sMainLoop = MessageLoop::current();
  }

  aPlatformAPI.version = 0;
  aPlatformAPI.createthread = &CreateThread;
  aPlatformAPI.runonmainthread = &RunOnMainThread;
  aPlatformAPI.syncrunonmainthread = &SyncRunOnMainThread;
  aPlatformAPI.createmutex = &CreateMutex;
  aPlatformAPI.createrecord = nullptr;
  aPlatformAPI.settimer = nullptr;
  aPlatformAPI.getcurrenttime = nullptr;
}

GMPThreadImpl::GMPThreadImpl()
: mMutex("GMPThreadImpl"),
  mThread("GMPThread")
{
  MOZ_COUNT_CTOR(GMPThread);
}

GMPThreadImpl::~GMPThreadImpl()
{
  MOZ_COUNT_DTOR(GMPThread);
}

void
GMPThreadImpl::Post(GMPTask* aTask)
{
  MutexAutoLock lock(mMutex);

  if (!mThread.IsRunning()) {
    bool started = mThread.Start();
    if (!started) {
      NS_WARNING("Unable to start GMPThread!");
      return;
    }
  }

  nsRefPtr<Runnable> r = new Runnable(aTask);

  mThread.message_loop()->PostTask(FROM_HERE, NewRunnableMethod(r.get(), &Runnable::Run));
}

void
GMPThreadImpl::Join()
{
  {
    MutexAutoLock lock(mMutex);
    if (mThread.IsRunning()) {
      mThread.Stop();
    }
  }
  delete this;
}

GMPMutexImpl::GMPMutexImpl()
: mMutex("gmp-mutex")
{
}

GMPMutexImpl::~GMPMutexImpl()
{
}

void
GMPMutexImpl::Acquire()
{
  mMutex.Lock();
}

void
GMPMutexImpl::Release()
{
  mMutex.Unlock();
}

} 
} 
