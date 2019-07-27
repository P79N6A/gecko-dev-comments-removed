




#include "GMPPlatform.h"
#include "GMPStorageChild.h"
#include "GMPTimerChild.h"
#include "mozilla/Monitor.h"
#include "nsAutoPtr.h"
#include "GMPChild.h"
#include <ctime>

namespace mozilla {
namespace gmp {

static MessageLoop* sMainLoop = nullptr;
static GMPChild* sChild = nullptr;

static bool
IsOnChildMainThread()
{
  return sMainLoop && sMainLoop == MessageLoop::current();
}


class Runnable MOZ_FINAL
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Runnable)

  explicit Runnable(GMPTask* aTask)
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
    
    
    
    
    MOZ_ASSERT(!IsOnChildMainThread());

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
  if (!aTask || !sMainLoop || IsOnChildMainThread()) {
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

GMPErr
CreateRecord(const char* aRecordName,
             uint32_t aRecordNameSize,
             GMPRecord** aOutRecord,
             GMPRecordClient* aClient)
{
  MOZ_ASSERT(IsOnChildMainThread());

  if (sMainLoop != MessageLoop::current()) {
    MOZ_ASSERT(false, "GMP called CreateRecord() on non-main thread!");
    return GMPGenericErr;
  }
  if (aRecordNameSize > GMP_MAX_RECORD_NAME_SIZE) {
    NS_WARNING("GMP tried to CreateRecord with too long record name");
    return GMPGenericErr;
  }
  GMPStorageChild* storage = sChild->GetGMPStorage();
  if (!storage) {
    return GMPGenericErr;
  }
  MOZ_ASSERT(storage);
  return storage->CreateRecord(nsDependentCString(aRecordName, aRecordNameSize),
                               aOutRecord,
                               aClient);
}

GMPErr
SetTimerOnMainThread(GMPTask* aTask, int64_t aTimeoutMS)
{
  if (!aTask || !sMainLoop || !IsOnChildMainThread()) {
    return GMPGenericErr;
  }
  GMPTimerChild* timers = sChild->GetGMPTimers();
  NS_ENSURE_TRUE(timers, GMPGenericErr);
  return timers->SetTimer(aTask, aTimeoutMS);
}

GMPErr
GetClock(GMPTimestamp* aOutTime)
{
  *aOutTime = time(0) * 1000;
  return GMPNoErr;
}

GMPErr
CreateRecordIterator(RecvGMPRecordIteratorPtr aRecvIteratorFunc,
                     void* aUserArg)
{
  if (sMainLoop != MessageLoop::current()) {
    MOZ_ASSERT(false, "GMP called CreateRecord() on non-main thread!");
    return GMPGenericErr;
  }
  if (!aRecvIteratorFunc) {
    return GMPInvalidArgErr;
  }
  GMPStorageChild* storage = sChild->GetGMPStorage();
  if (!storage) {
    return GMPGenericErr;
  }
  MOZ_ASSERT(storage);
  return storage->EnumerateRecords(aRecvIteratorFunc, aUserArg);
}

void
InitPlatformAPI(GMPPlatformAPI& aPlatformAPI, GMPChild* aChild)
{
  if (!sMainLoop) {
    sMainLoop = MessageLoop::current();
  }
  if (!sChild) {
    sChild = aChild;
  }

  aPlatformAPI.version = 0;
  aPlatformAPI.createthread = &CreateThread;
  aPlatformAPI.runonmainthread = &RunOnMainThread;
  aPlatformAPI.syncrunonmainthread = &SyncRunOnMainThread;
  aPlatformAPI.createmutex = &CreateMutex;
  aPlatformAPI.createrecord = &CreateRecord;
  aPlatformAPI.settimer = &SetTimerOnMainThread;
  aPlatformAPI.getcurrenttime = &GetClock;
  aPlatformAPI.getrecordenumerator = &CreateRecordIterator;
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
: mMonitor("gmp-mutex")
{
  MOZ_COUNT_CTOR(GMPMutexImpl);
}

GMPMutexImpl::~GMPMutexImpl()
{
  MOZ_COUNT_DTOR(GMPMutexImpl);
}

void
GMPMutexImpl::Destroy()
{
  delete this;
}

void
GMPMutexImpl::Acquire()
{
  mMonitor.Enter();
}

void
GMPMutexImpl::Release()
{
  mMonitor.Exit();
}

} 
} 
