





































#include "nsThread.h"
#include "nsThreadManager.h"
#include "nsIClassInfoImpl.h"
#include "nsIProgrammingLanguage.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "prlog.h"

#ifdef PR_LOGGING
static PRLogModuleInfo *sLog = PR_NewLogModule("nsThread");
#endif
#define LOG(args) PR_LOG(sLog, PR_LOG_DEBUG, args)

NS_DECL_CI_INTERFACE_GETTER(nsThread)





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
    nsThreadStartupEvent *startup = new nsThreadStartupEvent();
    if (startup && startup->mMon)
      return startup;
    
    delete startup;
    return nsnull;
  }

  
  
  void Wait() {
    if (mInitialized)  
      return;
    nsAutoMonitor mon(mMon);
    while (!mInitialized)
      mon.Wait();
  }

  
  
  virtual ~nsThreadStartupEvent() {
    if (mMon)
      nsAutoMonitor::DestroyMonitor(mMon);
  }

private:
  NS_IMETHOD Run() {
    nsAutoMonitor mon(mMon);
    mInitialized = PR_TRUE;
    mon.Notify();
    return NS_OK;
  }

  nsThreadStartupEvent()
    : mMon(nsAutoMonitor::NewMonitor("xpcom.threadstartup"))
    , mInitialized(PR_FALSE) {
  }

  PRMonitor *mMon;
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

  NS_ProcessPendingEvents(self);

  
  nsThreadManager::get()->UnregisterCurrentThread(self);

  
  event = new nsThreadShutdownAckEvent(self->mShutdownContext);
  self->mShutdownContext->joiningThread->Dispatch(event, NS_DISPATCH_NORMAL);

  NS_RELEASE(self);
}



nsThread::nsThread()
  : mLock(PR_NewLock())
  , mEvents(&mEventsRoot)
  , mPriority(PRIORITY_NORMAL)
  , mThread(nsnull)
  , mRunningEvent(0)
  , mShutdownContext(nsnull)
  , mShutdownRequired(PR_FALSE)
{
}

nsThread::~nsThread()
{
  if (mLock)
    PR_DestroyLock(mLock);
}

nsresult
nsThread::Init()
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

  
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
    nsAutoLock lock(mLock);
    mEvents->PutEvent(startup);
  }

  
  
  startup->Wait();
  return NS_OK;
}

nsresult
nsThread::InitCurrentThread()
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

  mThread = PR_GetCurrentThread();

  nsThreadManager::get()->RegisterCurrentThread(this);
  return NS_OK;
}

PRBool
nsThread::PutEvent(nsIRunnable *event)
{
  PRBool rv;
  {
    nsAutoLock lock(mLock);
    rv = mEvents->PutEvent(event);
  }
  if (!rv)
    return PR_FALSE;

  nsCOMPtr<nsIThreadObserver> obs = GetObserver();
  if (obs)
    obs->OnDispatchedEvent(this);

  return PR_TRUE;
}




NS_IMETHODIMP
nsThread::Dispatch(nsIRunnable *event, PRUint32 flags)
{
  LOG(("THRD(%p) Dispatch [%p %x]\n", this, event, flags));

  NS_ENSURE_ARG_POINTER(event);

  PRBool dispatched;
  if (flags & DISPATCH_SYNC) {
    nsThread *thread = nsThreadManager::get()->GetCurrentThread();
    NS_ENSURE_STATE(thread);

    
    
    
 
    nsRefPtr<nsThreadSyncDispatch> wrapper =
        new nsThreadSyncDispatch(thread, event);
    if (!wrapper)
      return NS_ERROR_OUT_OF_MEMORY;
    dispatched = PutEvent(wrapper);

    while (wrapper->IsPending())
      NS_ProcessNextEvent(thread);
  } else {
    NS_ASSERTION(flags == NS_DISPATCH_NORMAL, "unexpected dispatch flags");
    dispatched = PutEvent(event);
  }

  if (NS_UNLIKELY(!dispatched))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
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
    nsAutoLock lock(mLock);
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
  return NS_OK;
}

NS_IMETHODIMP
nsThread::HasPendingEvents(PRBool *result)
{
  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  *result = mEvents->GetEvent(PR_FALSE, nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsThread::ProcessNextEvent(PRBool mayWait, PRBool *result)
{
  LOG(("THRD(%p) ProcessNextEvent [%u %u]\n", this, mayWait, mRunningEvent));

  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  nsCOMPtr<nsIThreadObserver> obs = mObserver;
  if (obs)
    obs->OnProcessNextEvent(this, mayWait && !ShuttingDown(), mRunningEvent);

  
  nsCOMPtr<nsIRunnable> event; 
  mEvents->GetEvent(mayWait && !ShuttingDown(), getter_AddRefs(event));

  *result = (event.get() != nsnull);

  nsresult rv = NS_OK;

  if (event) {
    LOG(("THRD(%p) running [%p]\n", this, event.get()));
    ++mRunningEvent;
    event->Run();
    --mRunningEvent;
  } else if (mayWait) {
    NS_ASSERTION(ShuttingDown(), "This should only happen when shutting down");
    rv = NS_ERROR_UNEXPECTED;
  }

  if (obs)
    obs->AfterProcessNextEvent(this, mRunningEvent);

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
  nsAutoLock lock(mLock);
  NS_IF_ADDREF(*obs = mObserver);
  return NS_OK;
}

NS_IMETHODIMP
nsThread::SetObserver(nsIThreadObserver *obs)
{
  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  nsAutoLock lock(mLock);
  mObserver = obs;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::PushEventQueue(nsIThreadEventFilter *filter)
{
  nsChainedEventQueue *queue = new nsChainedEventQueue(filter);
  if (!queue || !queue->IsInitialized()) {
    delete queue;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsAutoLock lock(mLock);
  queue->mNext = mEvents;
  mEvents = queue;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::PopEventQueue()
{
  nsAutoLock lock(mLock);

  
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
nsThreadSyncDispatch::Run()
{
  if (mSyncTask) {
    mSyncTask->Run();
    mSyncTask = nsnull;
    
    mOrigin->Dispatch(this, NS_DISPATCH_NORMAL);
  }
  return NS_OK;
}
