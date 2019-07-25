





































#include "mozilla/ReentrantMonitor.h"
#include "nsThread.h"
#include "nsThreadManager.h"
#include "nsIClassInfoImpl.h"
#include "nsIProgrammingLanguage.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "prlog.h"
#include "nsThreadUtilsInternal.h"

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

#include "mozilla/FunctionTimer.h"
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

nsIThreadObserver* nsThread::sGlobalObserver;





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
nsThreadClassInfo::GetInterfaces(PRUint32 *count, nsIID ***array)
{
  return NS_CI_INTERFACE_GETTER_NAME(nsThread)(count, array);
}

NS_IMETHODIMP
nsThreadClassInfo::GetHelperForLanguage(PRUint32 lang, nsISupports **result)
{
  *result = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetContractID(char **result)
{
  *result = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetClassDescription(char **result)
{
  *result = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetClassID(nsCID **result)
{
  *result = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetImplementationLanguage(PRUint32 *result)
{
  *result = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetFlags(PRUint32 *result)
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
  NS_INTERFACE_MAP_ENTRY(nsIThreadInternal2)
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
    mInitialized = PR_TRUE;
    mon.Notify();
    return NS_OK;
  }

  nsThreadStartupEvent()
    : mMon("nsThreadStartupEvent.mMon")
    , mInitialized(PR_FALSE) {
  }

  ReentrantMonitor mMon;
  PRBool     mInitialized;
};



struct nsThreadShutdownContext {
  nsThread *joiningThread;
  PRBool    shutdownAck;
};



class nsThreadShutdownAckEvent : public nsRunnable {
public:
  nsThreadShutdownAckEvent(nsThreadShutdownContext *ctx)
    : mShutdownContext(ctx) {
  }
  NS_IMETHOD Run() {
    mShutdownContext->shutdownAck = PR_TRUE;
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
  if (!self->GetEvent(PR_TRUE, getter_AddRefs(event))) {
    NS_WARNING("failed waiting for thread startup event");
    return;
  }
  event->Run();  
  event = nsnull;

  
  while (!self->ShuttingDown())
    NS_ProcessNextEvent(self);

  
  
  
  
  
  while (PR_TRUE) {
    {
      MutexAutoLock lock(self->mLock);
      if (!self->mEvents->HasPendingEvent()) {
        
        
        
        
        self->mEventsAreDoomed = PR_TRUE;
        break;
      }
    }
    NS_ProcessPendingEvents(self);
  }

  
  nsThreadManager::get()->UnregisterCurrentThread(self);

  
  event = new nsThreadShutdownAckEvent(self->mShutdownContext);
  self->mShutdownContext->joiningThread->Dispatch(event, NS_DISPATCH_NORMAL);

  
  self->SetObserver(nsnull);

  NS_RELEASE(self);
}



nsThread::nsThread()
  : mLock("nsThread.mLock")
  , mEvents(&mEventsRoot)
  , mPriority(PRIORITY_NORMAL)
  , mThread(nsnull)
  , mRunningEvent(0)
  , mShutdownContext(nsnull)
  , mShutdownRequired(PR_FALSE)
  , mEventsAreDoomed(PR_FALSE)
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
 
  mShutdownRequired = PR_TRUE;

  
  PRThread *thr = PR_CreateThread(PR_USER_THREAD, ThreadFunc, this,
                                  PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                                  PR_JOINABLE_THREAD, 0);
  if (!thr) {
    NS_RELEASE_THIS();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  {
    MutexAutoLock lock(mLock);
    mEvents->PutEvent(startup);
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
    if (!mEvents->PutEvent(event))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIThreadObserver> obs = GetObserver();
  if (obs)
    obs->OnDispatchedEvent(this);

  return NS_OK;
}




NS_IMETHODIMP
nsThread::Dispatch(nsIRunnable *event, PRUint32 flags)
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
nsThread::IsOnCurrentThread(PRBool *result)
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
    mShutdownRequired = PR_FALSE;
  }

  nsThreadShutdownContext context;
  context.joiningThread = nsThreadManager::get()->GetCurrentThread();
  context.shutdownAck = PR_FALSE;

  
  
  nsCOMPtr<nsIRunnable> event = new nsThreadShutdownEvent(this, &context);
  if (!event)
    return NS_ERROR_OUT_OF_MEMORY;
  
  PutEvent(event);

  
  
  
  
  
  while (!context.shutdownAck)
    NS_ProcessNextEvent(context.joiningThread);

  

  PR_JoinThread(mThread);
  mThread = nsnull;

#ifdef DEBUG
  {
    MutexAutoLock lock(mLock);
    NS_ASSERTION(!mObserver, "Should have been cleared at shutdown!");
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsThread::HasPendingEvents(PRBool *result)
{
  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  *result = mEvents->GetEvent(PR_FALSE, nsnull);
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
nsThread::ProcessNextEvent(PRBool mayWait, PRBool *result)
{
  LOG(("THRD(%p) ProcessNextEvent [%u %u]\n", this, mayWait, mRunningEvent));

  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  PRBool notifyGlobalObserver = (sGlobalObserver != nsnull);
  if (notifyGlobalObserver) 
    sGlobalObserver->OnProcessNextEvent(this, mayWait && !ShuttingDown(),
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
    mEvents->GetEvent(mayWait && !ShuttingDown(), getter_AddRefs(event));

#ifdef NS_FUNCTION_TIMER
    char message[1024] = {'\0'};
    if (NS_IsMainThread()) {
        mozilla::FunctionTimer::ft_snprintf(message, sizeof(message), 
                                            "@ Main Thread Event %p", (void*)event.get());
    }
    
    
    NS_TIME_FUNCTION_MIN_FMT(5.0, message);
#endif

    *result = (event.get() != nsnull);

    if (event) {
      LOG(("THRD(%p) running [%p]\n", this, event.get()));
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

  if (notifyGlobalObserver && sGlobalObserver)
    sGlobalObserver->AfterProcessNextEvent(this, mRunningEvent);

  return rv;
}




NS_IMETHODIMP
nsThread::GetPriority(PRInt32 *priority)
{
  *priority = mPriority;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::SetPriority(PRInt32 priority)
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
nsThread::AdjustPriority(PRInt32 delta)
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
nsThread::PushEventQueue(nsIThreadEventFilter *filter)
{
  nsChainedEventQueue *queue = new nsChainedEventQueue(filter);

  MutexAutoLock lock(mLock);
  queue->mNext = mEvents;
  mEvents = queue;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::PopEventQueue()
{
  MutexAutoLock lock(mLock);

  
  NS_ENSURE_STATE(mEvents != &mEventsRoot);

  nsChainedEventQueue *queue = mEvents;
  mEvents = mEvents->mNext;

  nsCOMPtr<nsIRunnable> event;
  while (queue->GetEvent(PR_FALSE, getter_AddRefs(event)))
    mEvents->PutEvent(event);

  delete queue;
  
  return NS_OK;
}

PRBool
nsThread::nsChainedEventQueue::PutEvent(nsIRunnable *event)
{
  PRBool val;
  if (!mFilter || mFilter->AcceptEvent(event)) {
    val = mQueue.PutEvent(event);
  } else {
    val = mNext->PutEvent(event);
  }
  return val;
}




NS_IMETHODIMP
nsThread::GetRecursionDepth(PRUint32 *depth)
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



NS_IMETHODIMP
nsThreadSyncDispatch::Run()
{
  if (mSyncTask) {
    mResult = mSyncTask->Run();
    mSyncTask = nsnull;
    
    mOrigin->Dispatch(this, NS_DISPATCH_NORMAL);
  }
  return NS_OK;
}

nsresult
NS_SetGlobalThreadObserver(nsIThreadObserver* aObserver)
{
  if (aObserver && nsThread::sGlobalObserver) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (!NS_IsMainThread()) {
    return NS_ERROR_UNEXPECTED;
  }

  nsThread::sGlobalObserver = aObserver;
  return NS_OK;
}
