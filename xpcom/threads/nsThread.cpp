





#include "mozilla/ReentrantMonitor.h"
#include "nsThread.h"
#include "nsThreadManager.h"
#include "nsIClassInfoImpl.h"
#include "nsIProgrammingLanguage.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "prlog.h"
#include "nsIObserverService.h"
#include "mozilla/HangMonitor.h"
#include "mozilla/Services.h"

#define HAVE_UALARM _BSD_SOURCE || (_XOPEN_SOURCE >= 500 ||                 \
                      _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED) &&           \
                      !(_POSIX_C_SOURCE >= 200809L || _XOPEN_SOURCE >= 700)

#if defined(XP_UNIX) && !defined(ANDROID) && !defined(DEBUG) && HAVE_UALARM \
  && defined(_GNU_SOURCE)
# define MOZ_CANARY
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

using namespace mozilla;

#ifdef PR_LOGGING
static PRLogModuleInfo *sLog = PR_NewLogModule("nsThread");
#endif
#define LOG(args) PR_LOG(sLog, PR_LOG_DEBUG, args)

NS_DECL_CI_INTERFACE_GETTER(nsThread)

nsIThreadObserver* nsThread::sMainThreadObserver = nullptr;

namespace mozilla {



static int32_t sMemoryPressurePending = 0;





void ScheduleMemoryPressureEvent()
{
  PR_ATOMIC_SET(&sMemoryPressurePending, 1);
}

} 





class nsThreadClassInfo : public nsIClassInfo {
public:
  NS_DECL_ISUPPORTS_INHERITED  
  NS_DECL_NSICLASSINFO

  nsThreadClassInfo() {}
};

static nsThreadClassInfo sThreadClassInfo;

NS_IMETHODIMP_(nsrefcnt) nsThreadClassInfo::AddRef() { return 2; }
NS_IMETHODIMP_(nsrefcnt) nsThreadClassInfo::Release() { return 1; }
NS_IMPL_QUERY_INTERFACE1(nsThreadClassInfo, nsIClassInfo)

NS_IMETHODIMP
nsThreadClassInfo::GetInterfaces(uint32_t *count, nsIID ***array)
{
  return NS_CI_INTERFACE_GETTER_NAME(nsThread)(count, array);
}

NS_IMETHODIMP
nsThreadClassInfo::GetHelperForLanguage(uint32_t lang, nsISupports **result)
{
  *result = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetContractID(char **result)
{
  *result = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetClassDescription(char **result)
{
  *result = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetClassID(nsCID **result)
{
  *result = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetImplementationLanguage(uint32_t *result)
{
  *result = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetFlags(uint32_t *result)
{
  *result = THREADSAFE;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetClassIDNoAlloc(nsCID *result)
{
  return NS_ERROR_NOT_AVAILABLE;
}



NS_IMPL_THREADSAFE_ADDREF(nsThread)
NS_IMPL_THREADSAFE_RELEASE(nsThread)
NS_INTERFACE_MAP_BEGIN(nsThread)
  NS_INTERFACE_MAP_ENTRY(nsIThread)
  NS_INTERFACE_MAP_ENTRY(nsIThreadInternal)
  NS_INTERFACE_MAP_ENTRY(nsIEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIThread)
  if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    foundInterface = static_cast<nsIClassInfo*>(&sThreadClassInfo);
  } else
NS_INTERFACE_MAP_END
NS_IMPL_CI_INTERFACE_GETTER4(nsThread, nsIThread, nsIThreadInternal,
                             nsIEventTarget, nsISupportsPriority)



class nsThreadStartupEvent : public nsRunnable {
public:
  
  static nsThreadStartupEvent *Create() {
    return new nsThreadStartupEvent();
  }

  
  
  void Wait() {
    if (mInitialized)  
      return;
    ReentrantMonitorAutoEnter mon(mMon);
    while (!mInitialized)
      mon.Wait();
  }

  
  
  virtual ~nsThreadStartupEvent() {
  }

private:
  NS_IMETHOD Run() {
    ReentrantMonitorAutoEnter mon(mMon);
    mInitialized = true;
    mon.Notify();
    return NS_OK;
  }

  nsThreadStartupEvent()
    : mMon("nsThreadStartupEvent.mMon")
    , mInitialized(false) {
  }

  ReentrantMonitor mMon;
  bool       mInitialized;
};



struct nsThreadShutdownContext {
  nsThread *joiningThread;
  bool      shutdownAck;
};



class nsThreadShutdownAckEvent : public nsRunnable {
public:
  nsThreadShutdownAckEvent(nsThreadShutdownContext *ctx)
    : mShutdownContext(ctx) {
  }
  NS_IMETHOD Run() {
    mShutdownContext->shutdownAck = true;
    return NS_OK;
  }
private:
  nsThreadShutdownContext *mShutdownContext;
};


class nsThreadShutdownEvent : public nsRunnable {
public:
  nsThreadShutdownEvent(nsThread *thr, nsThreadShutdownContext *ctx)
    : mThread(thr), mShutdownContext(ctx) {
  } 
  NS_IMETHOD Run() {
    mThread->mShutdownContext = mShutdownContext;
    return NS_OK;
  }
private:
  nsRefPtr<nsThread>       mThread;
  nsThreadShutdownContext *mShutdownContext;
};



 void
nsThread::ThreadFunc(void *arg)
{
  nsThread *self = static_cast<nsThread *>(arg);  
  self->mThread = PR_GetCurrentThread();

  
  nsThreadManager::get()->RegisterCurrentThread(self);

  
  nsCOMPtr<nsIRunnable> event;
  if (!self->GetEvent(true, getter_AddRefs(event))) {
    NS_WARNING("failed waiting for thread startup event");
    return;
  }
  event->Run();  
  event = nullptr;

  
  while (!self->ShuttingDown())
    NS_ProcessNextEvent(self);

  
  
  
  
  
  while (true) {
    {
      MutexAutoLock lock(self->mLock);
      if (!self->mEvents.HasPendingEvent()) {
        
        
        
        
        self->mEventsAreDoomed = true;
        break;
      }
    }
    NS_ProcessPendingEvents(self);
  }

  
  nsThreadManager::get()->UnregisterCurrentThread(self);

  
  event = new nsThreadShutdownAckEvent(self->mShutdownContext);
  self->mShutdownContext->joiningThread->Dispatch(event, NS_DISPATCH_NORMAL);

  
  self->SetObserver(nullptr);

  NS_RELEASE(self);
}



nsThread::nsThread(MainThreadFlag aMainThread, uint32_t aStackSize)
  : mLock("nsThread.mLock")
  , mPriority(PRIORITY_NORMAL)
  , mThread(nullptr)
  , mRunningEvent(0)
  , mStackSize(aStackSize)
  , mShutdownContext(nullptr)
  , mShutdownRequired(false)
  , mEventsAreDoomed(false)
  , mIsMainThread(aMainThread)
{
}

nsThread::~nsThread()
{
}

nsresult
nsThread::Init()
{
  
  nsRefPtr<nsThreadStartupEvent> startup = nsThreadStartupEvent::Create();
  NS_ENSURE_TRUE(startup, NS_ERROR_OUT_OF_MEMORY);
 
  NS_ADDREF_THIS();
 
  mShutdownRequired = true;

  
  PRThread *thr = PR_CreateThread(PR_USER_THREAD, ThreadFunc, this,
                                  PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                                  PR_JOINABLE_THREAD, mStackSize);
  if (!thr) {
    NS_RELEASE_THIS();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  {
    MutexAutoLock lock(mLock);
    mEvents.PutEvent(startup);
  }

  
  
  startup->Wait();
  return NS_OK;
}

nsresult
nsThread::InitCurrentThread()
{
  mThread = PR_GetCurrentThread();

  nsThreadManager::get()->RegisterCurrentThread(this);
  return NS_OK;
}

nsresult
nsThread::PutEvent(nsIRunnable *event)
{
  {
    MutexAutoLock lock(mLock);
    if (mEventsAreDoomed) {
      NS_WARNING("An event was posted to a thread that will never run it (rejected)");
      return NS_ERROR_UNEXPECTED;
    }
    if (!mEvents.PutEvent(event))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIThreadObserver> obs = GetObserver();
  if (obs)
    obs->OnDispatchedEvent(this);

  return NS_OK;
}




NS_IMETHODIMP
nsThread::Dispatch(nsIRunnable *event, uint32_t flags)
{
  LOG(("THRD(%p) Dispatch [%p %x]\n", this, event, flags));

  NS_ENSURE_ARG_POINTER(event);

  if (flags & DISPATCH_SYNC) {
    nsThread *thread = nsThreadManager::get()->GetCurrentThread();
    NS_ENSURE_STATE(thread);

    
    
    
 
    nsRefPtr<nsThreadSyncDispatch> wrapper =
        new nsThreadSyncDispatch(thread, event);
    if (!wrapper)
      return NS_ERROR_OUT_OF_MEMORY;
    nsresult rv = PutEvent(wrapper);
    
    if (NS_FAILED(rv))
      return rv;

    while (wrapper->IsPending())
      NS_ProcessNextEvent(thread);
    return wrapper->Result();
  }

  NS_ASSERTION(flags == NS_DISPATCH_NORMAL, "unexpected dispatch flags");
  return PutEvent(event);
}

NS_IMETHODIMP
nsThread::IsOnCurrentThread(bool *result)
{
  *result = (PR_GetCurrentThread() == mThread);
  return NS_OK;
}




NS_IMETHODIMP
nsThread::GetPRThread(PRThread **result)
{
  *result = mThread;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::Shutdown()
{
  LOG(("THRD(%p) shutdown\n", this));

  
  
  
  if (!mThread)
    return NS_OK;

  NS_ENSURE_STATE(mThread != PR_GetCurrentThread());

  
  {
    MutexAutoLock lock(mLock);
    if (!mShutdownRequired)
      return NS_ERROR_UNEXPECTED;
    mShutdownRequired = false;
  }

  nsThreadShutdownContext context;
  context.joiningThread = nsThreadManager::get()->GetCurrentThread();
  context.shutdownAck = false;

  
  
  nsCOMPtr<nsIRunnable> event = new nsThreadShutdownEvent(this, &context);
  if (!event)
    return NS_ERROR_OUT_OF_MEMORY;
  
  PutEvent(event);

  
  
  
  
  
  while (!context.shutdownAck)
    NS_ProcessNextEvent(context.joiningThread);

  

  PR_JoinThread(mThread);
  mThread = nullptr;

  
  
  
  ClearObservers();

#ifdef DEBUG
  {
    MutexAutoLock lock(mLock);
    NS_ASSERTION(!mObserver, "Should have been cleared at shutdown!");
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsThread::HasPendingEvents(bool *result)
{
  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  *result = mEvents.GetEvent(false, nullptr);
  return NS_OK;
}

#ifdef MOZ_CANARY
void canary_alarm_handler (int signum);

class Canary {

public:
  Canary() {
    if (sOutputFD != 0 && EventLatencyIsImportant()) {
      if (sOutputFD == -1) {
        const int flags = O_WRONLY | O_APPEND | O_CREAT | O_NONBLOCK;
        const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        char* env_var_flag = getenv("MOZ_KILL_CANARIES");
        sOutputFD = env_var_flag ? (env_var_flag[0] ?
                                    open(env_var_flag, flags, mode) :
                                    STDERR_FILENO) : 0;
        if (sOutputFD == 0)
          return;
      }
      signal(SIGALRM, canary_alarm_handler);
      ualarm(15000, 0);      
    }
  }

  ~Canary() {
    if (sOutputFD != 0 && EventLatencyIsImportant())
      ualarm(0, 0);
  }

  static bool EventLatencyIsImportant() {
    return NS_IsMainThread() && XRE_GetProcessType() == GeckoProcessType_Default;
  }

  static int sOutputFD;
};

int Canary::sOutputFD = -1;

void canary_alarm_handler (int signum)
{
  void *array[30];
  const char msg[29] = "event took too long to run:\n";
  
  write(Canary::sOutputFD, msg, sizeof(msg)); 
  backtrace_symbols_fd(array, backtrace(array, 30), Canary::sOutputFD);
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
nsThread::ProcessNextEvent(bool mayWait, bool *result)
{
  LOG(("THRD(%p) ProcessNextEvent [%u %u]\n", this, mayWait, mRunningEvent));

  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  if (MAIN_THREAD == mIsMainThread && mayWait && !ShuttingDown())
    HangMonitor::Suspend();

  
  
  if (MAIN_THREAD == mIsMainThread && !ShuttingDown()) {
    bool mpPending = PR_ATOMIC_SET(&sMemoryPressurePending, 0);
    if (mpPending) {
      nsCOMPtr<nsIObserverService> os = services::GetObserverService();
      if (os) {
        os->NotifyObservers(nullptr, "memory-pressure",
                            NS_LITERAL_STRING("low-memory").get());
      }
      else {
        NS_WARNING("Can't get observer service!");
      }
    }
  }

  bool notifyMainThreadObserver =
    (MAIN_THREAD == mIsMainThread) && sMainThreadObserver;
  if (notifyMainThreadObserver) 
   sMainThreadObserver->OnProcessNextEvent(this, mayWait && !ShuttingDown(),
                                           mRunningEvent);

  nsCOMPtr<nsIThreadObserver> obs = mObserver;
  if (obs)
    obs->OnProcessNextEvent(this, mayWait && !ShuttingDown(), mRunningEvent);

  NOTIFY_EVENT_OBSERVERS(OnProcessNextEvent,
                         (this, mayWait && !ShuttingDown(), mRunningEvent));

  ++mRunningEvent;

#ifdef MOZ_CANARY
  Canary canary;
#endif
  nsresult rv = NS_OK;

  {
    
    
    

    
    nsCOMPtr<nsIRunnable> event;
    mEvents.GetEvent(mayWait && !ShuttingDown(), getter_AddRefs(event));

    *result = (event.get() != nullptr);

    if (event) {
      LOG(("THRD(%p) running [%p]\n", this, event.get()));
      if (MAIN_THREAD == mIsMainThread)
        HangMonitor::NotifyActivity();
      event->Run();
    } else if (mayWait) {
      NS_ASSERTION(ShuttingDown(),
                   "This should only happen when shutting down");
      rv = NS_ERROR_UNEXPECTED;
    }
  }

  --mRunningEvent;

  NOTIFY_EVENT_OBSERVERS(AfterProcessNextEvent, (this, mRunningEvent));

  if (obs)
    obs->AfterProcessNextEvent(this, mRunningEvent);

  if (notifyMainThreadObserver && sMainThreadObserver)
    sMainThreadObserver->AfterProcessNextEvent(this, mRunningEvent);

  return rv;
}




NS_IMETHODIMP
nsThread::GetPriority(int32_t *priority)
{
  *priority = mPriority;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::SetPriority(int32_t priority)
{
  NS_ENSURE_STATE(mThread);

  
  
  
  
  
  

  mPriority = priority;

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
  PR_SetThreadPriority(mThread, pri);

  return NS_OK;
}

NS_IMETHODIMP
nsThread::AdjustPriority(int32_t delta)
{
  return SetPriority(mPriority + delta);
}




NS_IMETHODIMP
nsThread::GetObserver(nsIThreadObserver **obs)
{
  MutexAutoLock lock(mLock);
  NS_IF_ADDREF(*obs = mObserver);
  return NS_OK;
}

NS_IMETHODIMP
nsThread::SetObserver(nsIThreadObserver *obs)
{
  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  MutexAutoLock lock(mLock);
  mObserver = obs;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::GetRecursionDepth(uint32_t *depth)
{
  NS_ENSURE_ARG_POINTER(depth);
  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  *depth = mRunningEvent;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::AddObserver(nsIThreadObserver *observer)
{
  NS_ENSURE_ARG_POINTER(observer);
  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  NS_WARN_IF_FALSE(!mEventObservers.Contains(observer),
                   "Adding an observer twice!");

  if (!mEventObservers.AppendElement(observer)) {
    NS_WARNING("Out of memory!");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsThread::RemoveObserver(nsIThreadObserver *observer)
{
  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  if (observer && !mEventObservers.RemoveElement(observer)) {
    NS_WARNING("Removing an observer that was never added!");
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
