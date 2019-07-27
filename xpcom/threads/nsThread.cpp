





#include "nsThread.h"

#include "base/message_loop.h"


#ifdef LOG
#undef LOG
#endif

#include "nsMemoryPressure.h"
#include "nsThreadManager.h"
#include "nsIClassInfoImpl.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "pratom.h"
#include "prlog.h"
#include "nsIObserverService.h"
#include "mozilla/HangMonitor.h"
#include "mozilla/IOInterposer.h"
#include "mozilla/ipc/MessageChannel.h"
#include "mozilla/Services.h"
#include "nsXPCOMPrivate.h"
#include "mozilla/ChaosMode.h"
#include "mozilla/ipc/BackgroundChild.h"

#ifdef MOZ_CRASHREPORTER
#include "nsServiceManagerUtils.h"
#include "nsICrashReporter.h"
#endif

#ifdef XP_LINUX
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#endif

#define HAVE_UALARM _BSD_SOURCE || (_XOPEN_SOURCE >= 500 ||                 \
                      _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED) &&           \
                      !(_POSIX_C_SOURCE >= 200809L || _XOPEN_SOURCE >= 700)

#if defined(XP_LINUX) && !defined(ANDROID) && defined(_GNU_SOURCE)
#define HAVE_SCHED_SETAFFINITY
#endif

#ifdef MOZ_CANARY
# include <unistd.h>
# include <execinfo.h>
# include <signal.h>
# include <fcntl.h>
# include "nsXULAppAPI.h"
#endif

#if defined(NS_FUNCTION_TIMER) && defined(_MSC_VER)
#include "nsTimerImpl.h"
#include "nsStackWalk.h"
#endif
#ifdef NS_FUNCTION_TIMER
#include "nsCRT.h"
#endif

#ifdef MOZ_TASK_TRACER
#include "GeckoTaskTracer.h"
using namespace mozilla::tasktracer;
#endif

using namespace mozilla;

#ifdef PR_LOGGING
static PRLogModuleInfo*
GetThreadLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog) {
    sLog = PR_NewLogModule("nsThread");
  }
  return sLog;
}
#endif
#ifdef LOG
#undef LOG
#endif
#define LOG(args) PR_LOG(GetThreadLog(), PR_LOG_DEBUG, args)

NS_DECL_CI_INTERFACE_GETTER(nsThread)

nsIThreadObserver* nsThread::sMainThreadObserver = nullptr;





class nsThreadClassInfo : public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS_INHERITED  
  NS_DECL_NSICLASSINFO

  nsThreadClassInfo()
  {
  }
};

NS_IMETHODIMP_(MozExternalRefCountType)
nsThreadClassInfo::AddRef()
{
  return 2;
}
NS_IMETHODIMP_(MozExternalRefCountType)
nsThreadClassInfo::Release()
{
  return 1;
}
NS_IMPL_QUERY_INTERFACE(nsThreadClassInfo, nsIClassInfo)

NS_IMETHODIMP
nsThreadClassInfo::GetInterfaces(uint32_t* aCount, nsIID*** aArray)
{
  return NS_CI_INTERFACE_GETTER_NAME(nsThread)(aCount, aArray);
}

NS_IMETHODIMP
nsThreadClassInfo::GetScriptableHelper(nsIXPCScriptable** aResult)
{
  *aResult = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetContractID(char** aResult)
{
  *aResult = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetClassDescription(char** aResult)
{
  *aResult = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetClassID(nsCID** aResult)
{
  *aResult = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetFlags(uint32_t* aResult)
{
  *aResult = THREADSAFE;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetClassIDNoAlloc(nsCID* aResult)
{
  return NS_ERROR_NOT_AVAILABLE;
}



NS_IMPL_ADDREF(nsThread)
NS_IMPL_RELEASE(nsThread)
NS_INTERFACE_MAP_BEGIN(nsThread)
  NS_INTERFACE_MAP_ENTRY(nsIThread)
  NS_INTERFACE_MAP_ENTRY(nsIThreadInternal)
  NS_INTERFACE_MAP_ENTRY(nsIEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIThread)
  if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    static nsThreadClassInfo sThreadClassInfo;
    foundInterface = static_cast<nsIClassInfo*>(&sThreadClassInfo);
  } else
NS_INTERFACE_MAP_END
NS_IMPL_CI_INTERFACE_GETTER(nsThread, nsIThread, nsIThreadInternal,
                            nsIEventTarget, nsISupportsPriority)



class nsThreadStartupEvent : public nsRunnable
{
public:
  nsThreadStartupEvent()
    : mMon("nsThreadStartupEvent.mMon")
    , mInitialized(false)
  {
  }

  
  
  void Wait()
  {
    if (mInitialized) {
      
      return;
    }

    ReentrantMonitorAutoEnter mon(mMon);
    while (!mInitialized) {
      mon.Wait();
    }
  }

  
  
  virtual ~nsThreadStartupEvent() {}

private:
  NS_IMETHOD Run()
  {
    ReentrantMonitorAutoEnter mon(mMon);
    mInitialized = true;
    mon.Notify();
    return NS_OK;
  }

  ReentrantMonitor mMon;
  bool mInitialized;
};



struct nsThreadShutdownContext
{
  nsThread* joiningThread;
  bool      shutdownAck;
};



class nsThreadShutdownAckEvent : public nsRunnable
{
public:
  explicit nsThreadShutdownAckEvent(nsThreadShutdownContext* aCtx)
    : mShutdownContext(aCtx)
  {
  }
  NS_IMETHOD Run()
  {
    mShutdownContext->shutdownAck = true;
    return NS_OK;
  }
private:
  nsThreadShutdownContext* mShutdownContext;
};


class nsThreadShutdownEvent : public nsRunnable
{
public:
  nsThreadShutdownEvent(nsThread* aThr, nsThreadShutdownContext* aCtx)
    : mThread(aThr)
    , mShutdownContext(aCtx)
  {
  }
  NS_IMETHOD Run()
  {
    mThread->mShutdownContext = mShutdownContext;
    MessageLoop::current()->Quit();
    return NS_OK;
  }
private:
  nsRefPtr<nsThread>       mThread;
  nsThreadShutdownContext* mShutdownContext;
};



static void
SetupCurrentThreadForChaosMode()
{
  if (!ChaosMode::isActive(ChaosMode::ThreadScheduling)) {
    return;
  }

#ifdef XP_LINUX
  
  
  
  
  
  
  
  
  
  
  
  
  setpriority(PRIO_PROCESS, 0, ChaosMode::randomUint32LessThan(4));
#else
  
  uint32_t priority = ChaosMode::randomUint32LessThan(PR_PRIORITY_LAST + 1);
  PR_SetThreadPriority(PR_GetCurrentThread(), PRThreadPriority(priority));
#endif

#ifdef HAVE_SCHED_SETAFFINITY
  
  if (ChaosMode::randomUint32LessThan(2)) {
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(0, &cpus);
    sched_setaffinity(0, sizeof(cpus), &cpus);
  }
#endif
}

 void
nsThread::ThreadFunc(void* aArg)
{
  using mozilla::ipc::BackgroundChild;

  nsThread* self = static_cast<nsThread*>(aArg);  
  self->mThread = PR_GetCurrentThread();
  SetupCurrentThreadForChaosMode();

  
  nsThreadManager::get()->RegisterCurrentThread(self);
#ifdef MOZ_NUWA_PROCESS
  self->mThreadStatusInfo =
    static_cast<void*>(nsThreadManager::get()->GetCurrentThreadStatusInfo());
#endif

  mozilla::IOInterposer::RegisterCurrentThread();

  
  nsCOMPtr<nsIRunnable> event;
  if (!self->GetEvent(true, getter_AddRefs(event))) {
    NS_WARNING("failed waiting for thread startup event");
    return;
  }
  event->Run();  
  event = nullptr;

  {
    
    nsAutoPtr<MessageLoop> loop(
      new MessageLoop(MessageLoop::TYPE_MOZILLA_NONMAINTHREAD));

    
    loop->Run();

    BackgroundChild::CloseForCurrentThread();

    
    
    
    
    
    while (true) {
      {
        MutexAutoLock lock(self->mLock);
        if (!self->mEvents->HasPendingEvent()) {
          
          
          
          
          self->mEventsAreDoomed = true;
          break;
        }
      }
      NS_ProcessPendingEvents(self);
    }
  }

  mozilla::IOInterposer::UnregisterCurrentThread();

  
  nsThreadManager::get()->UnregisterCurrentThread(self);

  
  event = new nsThreadShutdownAckEvent(self->mShutdownContext);
  self->mShutdownContext->joiningThread->Dispatch(event, NS_DISPATCH_NORMAL);

  
  self->SetObserver(nullptr);

#ifdef MOZ_TASK_TRACER
  FreeTraceInfo();
#endif

  NS_RELEASE(self);
}



#ifdef MOZ_CRASHREPORTER


static bool SaveMemoryReportNearOOM()
{
  bool needMemoryReport = false;

#ifdef XP_WIN 
  const size_t LOWMEM_THRESHOLD_VIRTUAL = 200 * 1024 * 1024;
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  if (GlobalMemoryStatusEx(&statex)) {
    if (statex.ullAvailVirtual < LOWMEM_THRESHOLD_VIRTUAL) {
      needMemoryReport = true;
    }
  }
#endif

  if (needMemoryReport) {
    nsCOMPtr<nsICrashReporter> cr =
      do_GetService("@mozilla.org/toolkit/crash-reporter;1");
    cr->SaveMemoryReport();
  }

  return needMemoryReport;
}
#endif

#ifdef MOZ_CANARY
int sCanaryOutputFD = -1;
#endif

nsThread::nsThread(MainThreadFlag aMainThread, uint32_t aStackSize)
  : mLock("nsThread.mLock")
  , mEvents(&mEventsRoot)
  , mPriority(PRIORITY_NORMAL)
  , mThread(nullptr)
  , mRunningEvent(0)
  , mStackSize(aStackSize)
  , mShutdownContext(nullptr)
  , mShutdownRequired(false)
  , mEventsAreDoomed(false)
  , mIsMainThread(aMainThread)
#ifdef MOZ_NUWA_PROCESS
  , mThreadStatusMonitor("nsThread.mThreadStatusLock")
  , mThreadStatusInfo(nullptr)
#endif
{
}

nsThread::~nsThread()
{
}

nsresult
nsThread::Init()
{
  
  nsRefPtr<nsThreadStartupEvent> startup = new nsThreadStartupEvent();

  NS_ADDREF_THIS();

  mShutdownRequired = true;

  
  PRThread* thr = PR_CreateThread(PR_USER_THREAD, ThreadFunc, this,
                                  PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                                  PR_JOINABLE_THREAD, mStackSize);
  if (!thr) {
    NS_RELEASE_THIS();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  {
    MutexAutoLock lock(mLock);
    mEventsRoot.PutEvent(startup);
  }

  
  
  startup->Wait();
  return NS_OK;
}

nsresult
nsThread::InitCurrentThread()
{
  mThread = PR_GetCurrentThread();
  SetupCurrentThreadForChaosMode();

  nsThreadManager::get()->RegisterCurrentThread(this);
#ifdef MOZ_NUWA_PROCESS
  mThreadStatusInfo =
    static_cast<void*>(nsThreadManager::get()->GetCurrentThreadStatusInfo());
#endif

  return NS_OK;
}

nsresult
nsThread::PutEvent(nsIRunnable* aEvent, nsNestedEventTarget* aTarget)
{
  nsCOMPtr<nsIThreadObserver> obs;

  {
    MutexAutoLock lock(mLock);
    nsChainedEventQueue* queue = aTarget ? aTarget->mQueue : &mEventsRoot;
    if (!queue || (queue == &mEventsRoot && mEventsAreDoomed)) {
      NS_WARNING("An event was posted to a thread that will never run it (rejected)");
      return NS_ERROR_UNEXPECTED;
    }
#ifdef MOZ_NUWA_PROCESS
    {
      ReentrantMonitorAutoEnter mon(mThreadStatusMonitor);
      SetWorking();
#endif
      queue->PutEvent(aEvent);
#ifdef MOZ_NUWA_PROCESS
    }
#endif 

    
    
    
    
    obs = mObserver;
  }

  if (obs) {
    obs->OnDispatchedEvent(this);
  }

  return NS_OK;
}

nsresult
nsThread::DispatchInternal(nsIRunnable* aEvent, uint32_t aFlags,
                           nsNestedEventTarget* aTarget)
{
  if (NS_WARN_IF(!aEvent)) {
    return NS_ERROR_INVALID_ARG;
  }

  if (gXPCOMThreadsShutDown && MAIN_THREAD != mIsMainThread && !aTarget) {
    return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;
  }

#ifdef MOZ_TASK_TRACER
  nsCOMPtr<nsIRunnable> tracedRunnable = CreateTracedRunnable(aEvent);
  aEvent = tracedRunnable;
#endif

  if (aFlags & DISPATCH_SYNC) {
    nsThread* thread = nsThreadManager::get()->GetCurrentThread();
    if (NS_WARN_IF(!thread)) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    
    
    

    nsRefPtr<nsThreadSyncDispatch> wrapper =
      new nsThreadSyncDispatch(thread, aEvent);
    nsresult rv = PutEvent(wrapper, aTarget);
    
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    while (wrapper->IsPending()) {
      NS_ProcessNextEvent(thread, true);
    }
    return wrapper->Result();
  }

  NS_ASSERTION(aFlags == NS_DISPATCH_NORMAL, "unexpected dispatch flags");
  return PutEvent(aEvent, aTarget);
}




NS_IMETHODIMP
nsThread::Dispatch(nsIRunnable* aEvent, uint32_t aFlags)
{
  LOG(("THRD(%p) Dispatch [%p %x]\n", this, aEvent, aFlags));

  return DispatchInternal(aEvent, aFlags, nullptr);
}

NS_IMETHODIMP
nsThread::IsOnCurrentThread(bool* aResult)
{
  *aResult = (PR_GetCurrentThread() == mThread);
  return NS_OK;
}




NS_IMETHODIMP
nsThread::GetPRThread(PRThread** aResult)
{
  *aResult = mThread;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::Shutdown()
{
  LOG(("THRD(%p) shutdown\n", this));

  
  
  
  if (!mThread) {
    return NS_OK;
  }

  if (NS_WARN_IF(mThread == PR_GetCurrentThread())) {
    return NS_ERROR_UNEXPECTED;
  }

  
  {
    MutexAutoLock lock(mLock);
    if (!mShutdownRequired) {
      return NS_ERROR_UNEXPECTED;
    }
    mShutdownRequired = false;
  }

  nsThreadShutdownContext context;
  context.joiningThread = nsThreadManager::get()->GetCurrentThread();
  context.shutdownAck = false;

  
  
  nsCOMPtr<nsIRunnable> event = new nsThreadShutdownEvent(this, &context);
  
  PutEvent(event, nullptr);

  
  
  

  
  
  while (!context.shutdownAck) {
    NS_ProcessNextEvent(context.joiningThread, true);
  }

  

  PR_JoinThread(mThread);
  mThread = nullptr;

  
  
  
  ClearObservers();

#ifdef DEBUG
  {
    MutexAutoLock lock(mLock);
    MOZ_ASSERT(!mObserver, "Should have been cleared at shutdown!");
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsThread::HasPendingEvents(bool* aResult)
{
  if (NS_WARN_IF(PR_GetCurrentThread() != mThread)) {
    return NS_ERROR_NOT_SAME_THREAD;
  }

  *aResult = mEvents->GetEvent(false, nullptr);
  return NS_OK;
}

#ifdef MOZ_CANARY
void canary_alarm_handler(int signum);

class Canary
{
  
public:
  Canary()
  {
    if (sCanaryOutputFD > 0 && EventLatencyIsImportant()) {
      signal(SIGALRM, canary_alarm_handler);
      ualarm(15000, 0);
    }
  }

  ~Canary()
  {
    if (sCanaryOutputFD != 0 && EventLatencyIsImportant()) {
      ualarm(0, 0);
    }
  }

  static bool EventLatencyIsImportant()
  {
    return NS_IsMainThread() && XRE_GetProcessType() == GeckoProcessType_Default;
  }
};

void canary_alarm_handler(int signum)
{
  void* array[30];
  const char msg[29] = "event took too long to run:\n";
  
  write(sCanaryOutputFD, msg, sizeof(msg));
  backtrace_symbols_fd(array, backtrace(array, 30), sCanaryOutputFD);
}

#endif

#define NOTIFY_EVENT_OBSERVERS(func_, params_)                                 \
  PR_BEGIN_MACRO                                                               \
    if (!mEventObservers.IsEmpty()) {                                          \
      nsAutoTObserverArray<nsCOMPtr<nsIThreadObserver>, 2>::ForwardIterator    \
        iter_(mEventObservers);                                                \
      nsCOMPtr<nsIThreadObserver> obs_;                                        \
      while (iter_.HasMore()) {                                                \
        obs_ = iter_.GetNext();                                                \
        obs_ -> func_ params_ ;                                                \
      }                                                                        \
    }                                                                          \
  PR_END_MACRO

NS_IMETHODIMP
nsThread::ProcessNextEvent(bool aMayWait, bool* aResult)
{
  LOG(("THRD(%p) ProcessNextEvent [%u %u]\n", this, aMayWait, mRunningEvent));

  
  MOZ_RELEASE_ASSERT(mIsMainThread != MAIN_THREAD ||
                     !ipc::ParentProcessIsBlocked());

  if (NS_WARN_IF(PR_GetCurrentThread() != mThread)) {
    return NS_ERROR_NOT_SAME_THREAD;
  }

  
  
  
  
  
  
  
  
  bool reallyWait = aMayWait && (mRunningEvent > 0 || !ShuttingDown());

  if (MAIN_THREAD == mIsMainThread && reallyWait) {
    HangMonitor::Suspend();
  }

  
  
  if (MAIN_THREAD == mIsMainThread && !ShuttingDown()) {
    MemoryPressureState mpPending = NS_GetPendingMemoryPressure();
    if (mpPending != MemPressure_None) {
      nsCOMPtr<nsIObserverService> os = services::GetObserverService();

      
      
      NS_NAMED_LITERAL_STRING(lowMem, "low-memory-no-forward");
      NS_NAMED_LITERAL_STRING(lowMemOngoing, "low-memory-ongoing-no-forward");

      if (os) {
        os->NotifyObservers(nullptr, "memory-pressure",
                            mpPending == MemPressure_New ? lowMem.get() :
                            lowMemOngoing.get());
      } else {
        NS_WARNING("Can't get observer service!");
      }
    }
  }

#ifdef MOZ_CRASHREPORTER
  if (MAIN_THREAD == mIsMainThread && !ShuttingDown()) {
    
    
    const size_t LOW_MEMORY_CHECK_SECONDS = 30;
    const size_t LOW_MEMORY_SAVE_SECONDS = 3 * 60;

    static TimeStamp nextCheck = TimeStamp::NowLoRes()
      + TimeDuration::FromSeconds(LOW_MEMORY_CHECK_SECONDS);
    
    TimeStamp now = TimeStamp::NowLoRes();
    if (now >= nextCheck) {
      if (SaveMemoryReportNearOOM()) {
        nextCheck = now + TimeDuration::FromSeconds(LOW_MEMORY_SAVE_SECONDS);
      } else {
        nextCheck = now + TimeDuration::FromSeconds(LOW_MEMORY_CHECK_SECONDS);
      }
    }
  }
#endif

  bool notifyMainThreadObserver =
    (MAIN_THREAD == mIsMainThread) && sMainThreadObserver;
  if (notifyMainThreadObserver) {
    sMainThreadObserver->OnProcessNextEvent(this, reallyWait, mRunningEvent);
  }

  nsCOMPtr<nsIThreadObserver> obs = mObserver;
  if (obs) {
    obs->OnProcessNextEvent(this, reallyWait, mRunningEvent);
  }

  NOTIFY_EVENT_OBSERVERS(OnProcessNextEvent,
                         (this, reallyWait, mRunningEvent));

  ++mRunningEvent;

#ifdef MOZ_CANARY
  Canary canary;
#endif
  nsresult rv = NS_OK;

  {
    
    
    

    
    nsCOMPtr<nsIRunnable> event;
    mEvents->GetEvent(reallyWait, getter_AddRefs(event));

    *aResult = (event.get() != nullptr);

    if (event) {
      LOG(("THRD(%p) running [%p]\n", this, event.get()));
      if (MAIN_THREAD == mIsMainThread) {
        HangMonitor::NotifyActivity();
      }
      event->Run();
    } else if (aMayWait) {
      MOZ_ASSERT(ShuttingDown(),
                 "This should only happen when shutting down");
      rv = NS_ERROR_UNEXPECTED;
    }
  }

  --mRunningEvent;

#ifdef MOZ_NUWA_PROCESS
  nsCOMPtr<nsIRunnable> notifyAllIdleRunnable;
  {
    ReentrantMonitorAutoEnter mon(mThreadStatusMonitor);
    if ((!mEvents->GetEvent(false, nullptr)) && (mRunningEvent == 0)) {
      nsThreadManager::get()->SetThreadIsWorking(
        static_cast<nsThreadManager::ThreadStatusInfo*>(mThreadStatusInfo),
        false, getter_AddRefs(notifyAllIdleRunnable));
    }
  }
  if (notifyAllIdleRunnable) {
    
    
    
    
    
    Dispatch(notifyAllIdleRunnable, NS_DISPATCH_NORMAL);
    nsThreadManager::get()->ResetIsDispatchingToMainThread();
  }
#endif 

  NOTIFY_EVENT_OBSERVERS(AfterProcessNextEvent,
                         (this, mRunningEvent, *aResult));

  if (obs) {
    obs->AfterProcessNextEvent(this, mRunningEvent, *aResult);
  }

  if (notifyMainThreadObserver && sMainThreadObserver) {
    sMainThreadObserver->AfterProcessNextEvent(this, mRunningEvent, *aResult);
  }

  return rv;
}




NS_IMETHODIMP
nsThread::GetPriority(int32_t* aPriority)
{
  *aPriority = mPriority;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::SetPriority(int32_t aPriority)
{
  if (NS_WARN_IF(!mThread)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  
  
  
  
  

  mPriority = aPriority;

  PRThreadPriority pri;
  if (mPriority <= PRIORITY_HIGHEST) {
    pri = PR_PRIORITY_URGENT;
  } else if (mPriority < PRIORITY_NORMAL) {
    pri = PR_PRIORITY_HIGH;
  } else if (mPriority > PRIORITY_NORMAL) {
    pri = PR_PRIORITY_LOW;
  } else {
    pri = PR_PRIORITY_NORMAL;
  }
  
  if (!ChaosMode::isActive(ChaosMode::ThreadScheduling)) {
    PR_SetThreadPriority(mThread, pri);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsThread::AdjustPriority(int32_t aDelta)
{
  return SetPriority(mPriority + aDelta);
}




NS_IMETHODIMP
nsThread::GetObserver(nsIThreadObserver** aObs)
{
  MutexAutoLock lock(mLock);
  NS_IF_ADDREF(*aObs = mObserver);
  return NS_OK;
}

NS_IMETHODIMP
nsThread::SetObserver(nsIThreadObserver* aObs)
{
  if (NS_WARN_IF(PR_GetCurrentThread() != mThread)) {
    return NS_ERROR_NOT_SAME_THREAD;
  }

  MutexAutoLock lock(mLock);
  mObserver = aObs;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::GetRecursionDepth(uint32_t* aDepth)
{
  if (NS_WARN_IF(PR_GetCurrentThread() != mThread)) {
    return NS_ERROR_NOT_SAME_THREAD;
  }

  *aDepth = mRunningEvent;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::AddObserver(nsIThreadObserver* aObserver)
{
  if (NS_WARN_IF(!aObserver)) {
    return NS_ERROR_INVALID_ARG;
  }
  if (NS_WARN_IF(PR_GetCurrentThread() != mThread)) {
    return NS_ERROR_NOT_SAME_THREAD;
  }

  NS_WARN_IF_FALSE(!mEventObservers.Contains(aObserver),
                   "Adding an observer twice!");

  if (!mEventObservers.AppendElement(aObserver)) {
    NS_WARNING("Out of memory!");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsThread::RemoveObserver(nsIThreadObserver* aObserver)
{
  if (NS_WARN_IF(PR_GetCurrentThread() != mThread)) {
    return NS_ERROR_NOT_SAME_THREAD;
  }

  if (aObserver && !mEventObservers.RemoveElement(aObserver)) {
    NS_WARNING("Removing an observer that was never added!");
  }

  return NS_OK;
}

NS_IMETHODIMP
nsThread::PushEventQueue(nsIEventTarget** aResult)
{
  if (NS_WARN_IF(PR_GetCurrentThread() != mThread)) {
    return NS_ERROR_NOT_SAME_THREAD;
  }

  nsChainedEventQueue* queue = new nsChainedEventQueue();
  queue->mEventTarget = new nsNestedEventTarget(this, queue);

  {
    MutexAutoLock lock(mLock);
    queue->mNext = mEvents;
    mEvents = queue;
  }

  NS_ADDREF(*aResult = queue->mEventTarget);
  return NS_OK;
}

NS_IMETHODIMP
nsThread::PopEventQueue(nsIEventTarget* aInnermostTarget)
{
  if (NS_WARN_IF(PR_GetCurrentThread() != mThread)) {
    return NS_ERROR_NOT_SAME_THREAD;
  }

  if (NS_WARN_IF(!aInnermostTarget)) {
    return NS_ERROR_NULL_POINTER;
  }

  
  nsAutoPtr<nsChainedEventQueue> queue;
  nsRefPtr<nsNestedEventTarget> target;

  {
    MutexAutoLock lock(mLock);

    
    if (NS_WARN_IF(mEvents->mEventTarget != aInnermostTarget)) {
      return NS_ERROR_UNEXPECTED;
    }

    MOZ_ASSERT(mEvents != &mEventsRoot);

    queue = mEvents;
    mEvents = mEvents->mNext;

    nsCOMPtr<nsIRunnable> event;
    while (queue->GetEvent(false, getter_AddRefs(event))) {
      mEvents->PutEvent(event);
    }

    
    queue->mEventTarget.swap(target);
    target->mQueue = nullptr;
  }

  return NS_OK;
}

nsresult
nsThread::SetMainThreadObserver(nsIThreadObserver* aObserver)
{
  if (aObserver && nsThread::sMainThreadObserver) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (!NS_IsMainThread()) {
    return NS_ERROR_UNEXPECTED;
  }

  nsThread::sMainThreadObserver = aObserver;
  return NS_OK;
}

#ifdef MOZ_NUWA_PROCESS
void
nsThread::SetWorking()
{
  nsThreadManager::get()->SetThreadIsWorking(
    static_cast<nsThreadManager::ThreadStatusInfo*>(mThreadStatusInfo),
    true, nullptr);
}

void
nsThread::SetIdle()
{
  nsThreadManager::get()->SetThreadIsWorking(
    static_cast<nsThreadManager::ThreadStatusInfo*>(mThreadStatusInfo),
    false, nullptr);
}
#endif



NS_IMETHODIMP
nsThreadSyncDispatch::Run()
{
  if (mSyncTask) {
    mResult = mSyncTask->Run();
    mSyncTask = nullptr;
    
    mOrigin->Dispatch(this, NS_DISPATCH_NORMAL);
  }
  return NS_OK;
}



NS_IMPL_ISUPPORTS(nsThread::nsNestedEventTarget, nsIEventTarget)

NS_IMETHODIMP
nsThread::nsNestedEventTarget::Dispatch(nsIRunnable* aEvent, uint32_t aFlags)
{
  LOG(("THRD(%p) Dispatch [%p %x] to nested loop %p\n", mThread.get(), aEvent,
       aFlags, this));

  return mThread->DispatchInternal(aEvent, aFlags, this);
}

NS_IMETHODIMP
nsThread::nsNestedEventTarget::IsOnCurrentThread(bool* aResult)
{
  return mThread->IsOnCurrentThread(aResult);
}
