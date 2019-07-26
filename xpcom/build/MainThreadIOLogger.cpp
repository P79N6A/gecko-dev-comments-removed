





#include "MainThreadIOLogger.h"

#include "GeckoProfiler.h"
#include "IOInterposerPrivate.h"
#include "mozilla/IOInterposer.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/TimeStamp.h"






#include <prenv.h>
#include <prprf.h>
#include <prthread.h>
#include <vector>

namespace {

struct ObservationWithStack
{
  ObservationWithStack(mozilla::IOInterposeObserver::Observation& aObs,
                       ProfilerBacktrace *aStack)
    : mObservation(aObs)
    , mStack(aStack)
  {
    const char16_t* filename = aObs.Filename();
    if (filename) {
      mFilename = filename;
    }
  }
 
  mozilla::IOInterposeObserver::Observation mObservation;
  ProfilerBacktrace*                        mStack;
  nsString                                  mFilename;
};

} 

namespace mozilla {

class MainThreadIOLoggerImpl MOZ_FINAL : public IOInterposeObserver
{
public:
  MainThreadIOLoggerImpl();
  ~MainThreadIOLoggerImpl();

  bool Init();

  void Observe(Observation& aObservation);

private:
  static void sIOThreadFunc(void* aArg);
  void IOThreadFunc();

  TimeStamp             mLogStartTime;
  const char*           mFileName;
  PRThread*             mIOThread;
  IOInterposer::Monitor mMonitor;
  bool                  mShutdownRequired;
  std::vector<ObservationWithStack> mObservations;
};

static StaticAutoPtr<MainThreadIOLoggerImpl> sImpl;

MainThreadIOLoggerImpl::MainThreadIOLoggerImpl()
  : mFileName(nullptr)
  , mIOThread(nullptr)
  , mShutdownRequired(false)
{
}

MainThreadIOLoggerImpl::~MainThreadIOLoggerImpl()
{
  if (!mIOThread) {
    return;
  }
  { 
    IOInterposer::MonitorAutoLock lock(mMonitor);
    mShutdownRequired = true;
    lock.Notify();
  }
  PR_JoinThread(mIOThread);
  mIOThread = nullptr;
}

bool
MainThreadIOLoggerImpl::Init()
{
  if (mFileName) {
    
    return true;
  }
  mFileName = PR_GetEnv("MOZ_MAIN_THREAD_IO_LOG");
  if (!mFileName) {
    
    return false;
  }
  mIOThread = PR_CreateThread(PR_USER_THREAD, &sIOThreadFunc, this,
                              PR_PRIORITY_LOW, PR_GLOBAL_THREAD,
                              PR_JOINABLE_THREAD, 0);
  if (!mIOThread) {
    return false;
  }
  return true;
}

 void
MainThreadIOLoggerImpl::sIOThreadFunc(void* aArg)
{
  PR_SetCurrentThreadName("MainThreadIOLogger");
  MainThreadIOLoggerImpl* obj = static_cast<MainThreadIOLoggerImpl*>(aArg);
  obj->IOThreadFunc();
}

void
MainThreadIOLoggerImpl::IOThreadFunc()
{
  PRFileDesc* fd = PR_Open(mFileName, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE,
                           PR_IRUSR | PR_IWUSR | PR_IRGRP);
  if (!fd) {
    IOInterposer::MonitorAutoLock lock(mMonitor);
    mShutdownRequired = true;
    std::vector<ObservationWithStack>().swap(mObservations);
    return;
  }
  mLogStartTime = TimeStamp::Now();
  { 
    IOInterposer::MonitorAutoLock lock(mMonitor);
    while (true) {
      while (!mShutdownRequired && mObservations.empty()) {
        lock.Wait();
      }
      if (mShutdownRequired) {
        break;
      }
      
      std::vector<ObservationWithStack> observationsToWrite;
      observationsToWrite.swap(mObservations);
 
      
      IOInterposer::MonitorAutoUnlock unlock(mMonitor);

      
      for (std::vector<ObservationWithStack>::iterator
             i = observationsToWrite.begin(), e = observationsToWrite.end();
           i != e; ++i) {
        if (i->mObservation.ObservedOperation() == OpNextStage) {
          PR_fprintf(fd, "%f,NEXT-STAGE\n",
                     (TimeStamp::Now() - mLogStartTime).ToMilliseconds());
          continue;
        }
        double durationMs = i->mObservation.Duration().ToMilliseconds();
        nsAutoCString nativeFilename;
        nativeFilename.AssignLiteral("(not available)");
        if (!i->mFilename.IsEmpty()) {
          if (NS_FAILED(NS_CopyUnicodeToNative(i->mFilename, nativeFilename))) {
            nativeFilename.AssignLiteral("(conversion failed)");
          }
        }
        



        if (PR_fprintf(fd, "%f,%s,%f,%s,%s\n",
                       (i->mObservation.Start() - mLogStartTime).ToMilliseconds(),
                       i->mObservation.ObservedOperationString(), durationMs,
                       i->mObservation.Reference(), nativeFilename.get()) > 0) {
          ProfilerBacktrace* stack = i->mStack;
          if (stack) {
            
            
            profiler_free_backtrace(stack);
          }
        }
      }
    }
  }
  PR_Close(fd);
}

void
MainThreadIOLoggerImpl::Observe(Observation& aObservation)
{
  if (!mFileName || !IsMainThread()) {
    return;
  }
  IOInterposer::MonitorAutoLock lock(mMonitor);
  if (mShutdownRequired) {
    
    return;
  }
  
  mObservations.push_back(ObservationWithStack(aObservation, nullptr));
  lock.Notify();
}

namespace MainThreadIOLogger {

bool
Init()
{
  sImpl = new MainThreadIOLoggerImpl();
  if (!sImpl->Init()) {
    return false;
  }
  IOInterposer::Register(IOInterposeObserver::OpAllWithStaging, sImpl);
  return true;
}

} 

} 

