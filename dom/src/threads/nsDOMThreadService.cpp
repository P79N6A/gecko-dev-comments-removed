






































#include "nsDOMThreadService.h"


#include "nsIComponentManager.h"
#include "nsIConsoleService.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIEventTarget.h"
#include "nsIGenericFactory.h"
#include "nsIJSContextStack.h"
#include "nsIJSRuntimeService.h"
#include "nsIObserverService.h"
#include "nsIScriptError.h"
#include "nsIScriptGlobalObject.h"
#include "nsIServiceManager.h"
#include "nsISupportsPriority.h"
#include "nsIThreadPool.h"
#include "nsIXPConnect.h"


#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsDeque.h"
#include "nsIClassInfoImpl.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMCID.h"
#include "nsXPCOMCIDInternal.h"
#include "pratom.h"
#include "prthread.h"


#include "nsDOMWorkerPool.h"
#include "nsDOMWorkerSecurityManager.h"
#include "nsDOMWorkerTimeout.h"

#ifdef PR_LOGGING
PRLogModuleInfo *gDOMThreadsLog = nsnull;
#endif
#define LOG(_args) PR_LOG(gDOMThreadsLog, PR_LOG_DEBUG, _args)


#define THREADPOOL_MAX_THREADS 3

PR_STATIC_ASSERT(THREADPOOL_MAX_THREADS >= 1);


#define THREADPOOL_IDLE_THREADS 3

PR_STATIC_ASSERT(THREADPOOL_MAX_THREADS >= THREADPOOL_IDLE_THREADS);





#define THREADPOOL_THREAD_CAP 20

PR_STATIC_ASSERT(THREADPOOL_THREAD_CAP >= THREADPOOL_MAX_THREADS);



#define CALLBACK_YIELD_THRESHOLD 100


#define BAD_TLS_INDEX (PRUintn)-1


#define NS_FORWARD_NSISUPPORTS(_to) \
  NS_IMETHOD QueryInterface(const nsIID& uuid, void** result) { \
    return _to QueryInterface(uuid, result); \
  } \
  NS_IMETHOD_(nsrefcnt) AddRef(void) { return _to AddRef(); } \
  NS_IMETHOD_(nsrefcnt) Release(void) { return _to Release(); } 


static nsDOMThreadService* gDOMThreadService = nsnull;


static nsIObserverService* gObserverService = nsnull;
static nsIJSRuntimeService* gJSRuntimeService = nsnull;
static nsIThreadJSContextStack* gThreadJSContextStack = nsnull;
static nsIXPCSecurityManager* gWorkerSecurityManager = nsnull;

PRUintn gJSContextIndex = BAD_TLS_INDEX;





class JSAutoContextDestroyer
{
public:
  JSAutoContextDestroyer(JSContext* aCx)
  : mCx(aCx) { }

  ~JSAutoContextDestroyer() {
    if (mCx) {
      nsContentUtils::XPConnect()->ReleaseJSContext(mCx, PR_TRUE);
    }
  }

  operator JSContext*() {
    return mCx;
  }

  JSContext* forget() {
    JSContext* cx = mCx;
    mCx = nsnull;
    return cx;
  }

private:
  JSContext* mCx;
};





class nsReportErrorRunnable : public nsRunnable
{
public:
  nsReportErrorRunnable(nsIScriptError* aError, nsDOMWorkerThread* aWorker)
  : mError(aError), mWorker(aWorker) { }

  NS_IMETHOD Run() {
    nsresult rv;

    nsCOMPtr<nsIConsoleService> consoleService =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      consoleService->LogMessage(mError);
    }

    if (!mWorker->IsCanceled()) {
#ifdef PR_LOGGING
      nsAutoString message;
      mError->GetErrorMessage(message);
#endif
      nsRefPtr<nsDOMWorkerPool> pool = mWorker->Pool();

      LOG(("Posting error '%s' to pool [0x%p]",
           NS_LossyConvertUTF16toASCII(message).get(),
           static_cast<void*>(pool.get())));

      pool->HandleError(mError, mWorker);
    }
    return NS_OK;
  }

private:
  
  nsCOMPtr<nsIScriptError> mError;

  
  
  nsRefPtr<nsDOMWorkerThread> mWorker;
};





class nsDOMWorkerScriptError : public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO

  nsDOMWorkerScriptError(nsIScriptError* aError)
  : mScriptError(this, aError) { }

protected:

  
  
  
  class InnerScriptError : public nsIScriptError
  {
  public:
    NS_FORWARD_NSISUPPORTS(mParent->)
    NS_FORWARD_NSISCRIPTERROR(mError->)
    NS_FORWARD_NSICONSOLEMESSAGE(mError->)

    InnerScriptError(nsDOMWorkerScriptError* aParent, nsIScriptError* aError)
    : mParent(aParent), mError(aError) { }

  protected:
    nsDOMWorkerScriptError* mParent;
    nsCOMPtr<nsIScriptError> mError;
  };

  InnerScriptError mScriptError;
};

NS_IMPL_THREADSAFE_ADDREF(nsDOMWorkerScriptError)
NS_IMPL_THREADSAFE_RELEASE(nsDOMWorkerScriptError)


NS_INTERFACE_MAP_BEGIN(nsDOMWorkerScriptError)
  if (aIID.Equals(NS_GET_IID(nsIScriptError)) ||
      aIID.Equals(NS_GET_IID(nsIConsoleMessage))) {
    foundInterface = static_cast<nsIConsoleMessage*>(&mScriptError);
  }
  else
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER2(nsDOMWorkerScriptError, nsIScriptError,
                                                     nsIConsoleMessage)

NS_IMPL_THREADSAFE_DOM_CI(nsDOMWorkerScriptError)




class nsDOMWorkerTimeoutRunnable : public nsRunnable
{
public:
  nsDOMWorkerTimeoutRunnable(nsDOMWorkerTimeout* aTimeout)
  : mTimeout(aTimeout) { }

  NS_IMETHOD Run() {
    return mTimeout->Run();
  }
protected:
  nsRefPtr<nsDOMWorkerTimeout> mTimeout;
};










class nsDOMWorkerRunnable : public nsRunnable
{
  friend class nsDOMThreadService;

public:
  nsDOMWorkerRunnable(nsDOMWorkerThread* aWorker)
  : mWorker(aWorker) { }

  virtual ~nsDOMWorkerRunnable() {
    nsCOMPtr<nsIRunnable> runnable;
    while ((runnable = dont_AddRef((nsIRunnable*)mRunnables.PopFront()))) {
      
    }

    
    nsDOMWorkerThread* worker = nsnull;
    mWorker.swap(worker);

    nsISupports* supports = NS_ISUPPORTS_CAST(nsIDOMWorkerThread*, worker);
    NS_ASSERTION(supports, "This should never be null!");

    nsCOMPtr<nsIThread> mainThread(do_GetMainThread());
    NS_ProxyRelease(mainThread, supports);
  }

  void PutRunnable(nsIRunnable* aRunnable) {
    NS_ASSERTION(aRunnable, "Null pointer!");

    NS_ADDREF(aRunnable);

    

    mRunnables.Push(aRunnable);
  }

  NS_IMETHOD Run() {
    
    NS_ASSERTION(gJSContextIndex != BAD_TLS_INDEX, "No context index!");

    
    JSContext* cx = (JSContext*)PR_GetThreadPrivate(gJSContextIndex);
    NS_ASSERTION(cx, "nsDOMThreadService didn't give us a context!");

    JS_SetContextPrivate(cx, mWorker);

    
    if (mWorker->SetGlobalForContext(cx)) {
      RunQueue();

      
      
      JS_SetGlobalObject(cx, NULL);
      JS_SetContextPrivate(cx, NULL);
    }
    else {
      
      JS_SetGlobalObject(cx, NULL);

      nsAutoMonitor mon(gDOMThreadService->mMonitor);
      gDOMThreadService->WorkerComplete(this);
      mon.NotifyAll();
    }

    return NS_OK;
  }

protected:

  void RunQueue() {
    JSContext* cx = (JSContext*)PR_GetThreadPrivate(gJSContextIndex);
    NS_ASSERTION(cx, "nsDOMThreadService didn't give us a context!");

    while (1) {
      nsCOMPtr<nsIRunnable> runnable;
      {
        nsAutoMonitor mon(gDOMThreadService->mMonitor);

        runnable = dont_AddRef((nsIRunnable*)mRunnables.PopFront());

        if (!runnable || mWorker->IsCanceled()) {
#ifdef PR_LOGGING
          if (mWorker->IsCanceled()) {
            LOG(("Bailing out of run loop for canceled worker[0x%p]",
                 static_cast<void*>(mWorker.get())));
          }
#endif
          gDOMThreadService->WorkerComplete(this);
          mon.NotifyAll();
          return;
        }
      }

      
      JS_ClearRegExpStatics(cx);

#ifdef DEBUG
      nsresult rv =
#endif
      runnable->Run();
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Runnable failed!");
    }
  }

  
  nsRefPtr<nsDOMWorkerThread> mWorker;

  
  nsDeque mRunnables;
};





JSBool
DOMWorkerOperationCallback(JSContext* aCx)
{
  nsDOMWorkerThread* worker = (nsDOMWorkerThread*)JS_GetContextPrivate(aCx);

  
  
  nsRefPtr<nsDOMWorkerPool> pool;

  PRBool wasSuspended = PR_FALSE;
  PRBool extraThreadAllowed = PR_FALSE;
  jsrefcount suspendDepth = 0;

  while (1) {
    
    if (worker->IsCanceled()) {
      LOG(("Forcefully killing JS for worker [0x%p]",
           static_cast<void*>(worker)));

      if (wasSuspended) {
        if (extraThreadAllowed) {
          gDOMThreadService->ChangeThreadPoolMaxThreads(-1);
        }
        JS_ResumeRequest(aCx, suspendDepth);
      }

      
      return PR_FALSE;
    }

    
    if (!worker->IsSuspended()) {
      if (wasSuspended) {
        if (extraThreadAllowed) {
          gDOMThreadService->ChangeThreadPoolMaxThreads(-1);
        }
        JS_ResumeRequest(aCx, suspendDepth);
      }
      break;
    }

    if (!wasSuspended) {
      
      
      if (worker->IsCanceled()) {
        NS_WARNING("Tried to suspend on a pool that has gone away");
        return PR_FALSE;
      }

      pool = worker->Pool();

      
      
      suspendDepth = JS_SuspendRequest(aCx);

      
      
      
      extraThreadAllowed =
        NS_SUCCEEDED(gDOMThreadService->ChangeThreadPoolMaxThreads(1));

      
      wasSuspended = PR_TRUE;
    }

    nsAutoMonitor mon(pool->Monitor());
    mon.Wait();
  }

  
  
  if (++worker->mCallbackCount >= CALLBACK_YIELD_THRESHOLD) {
    
    JS_YieldRequest(aCx);

    
    worker->mCallbackCount = 0;
  }

  
  return JS_TRUE;
}

void
DOMWorkerErrorReporter(JSContext* aCx,
                       const char* aMessage,
                       JSErrorReport* aReport)
{
  NS_ASSERTION(!NS_IsMainThread(), "Huh?!");

  nsDOMWorkerThread* worker = (nsDOMWorkerThread*)JS_GetContextPrivate(aCx);

  if (worker->IsCanceled()) {
    
    
    
    return;
  }

  nsresult rv;
  nsCOMPtr<nsIScriptError> errorObject =
    do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,);

  const PRUnichar* message =
    reinterpret_cast<const PRUnichar*>(aReport->ucmessage);

  nsAutoString filename;
  filename.AssignWithConversion(aReport->filename);

  const PRUnichar* line =
    reinterpret_cast<const PRUnichar*>(aReport->uclinebuf);

  PRUint32 column = aReport->uctokenptr - aReport->uclinebuf;

  rv = errorObject->Init(message, filename.get(), line, aReport->lineno,
                        column, aReport->flags, "DOM Worker javascript");
  NS_ENSURE_SUCCESS(rv,);

  nsRefPtr<nsDOMWorkerScriptError> domError =
    new nsDOMWorkerScriptError(errorObject);
  NS_ENSURE_TRUE(domError,);

  nsCOMPtr<nsIScriptError> scriptError(do_QueryInterface(domError));
  NS_ENSURE_TRUE(scriptError,);

  nsCOMPtr<nsIThread> mainThread(do_GetMainThread());
  NS_ENSURE_TRUE(mainThread,);

  nsCOMPtr<nsIRunnable> runnable =
    new nsReportErrorRunnable(scriptError, worker);
  NS_ENSURE_TRUE(runnable,);

  rv = mainThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv,);
}





nsDOMThreadService::nsDOMThreadService()
: mMonitor(nsnull)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
#ifdef PR_LOGGING
  if (!gDOMThreadsLog) {
    gDOMThreadsLog = PR_NewLogModule("nsDOMThreads");
  }
#endif
  LOG(("Initializing DOM Thread service"));
}

nsDOMThreadService::~nsDOMThreadService()
{
  LOG(("DOM Thread service destroyed"));

  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  Cleanup();

  if (mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS4(nsDOMThreadService, nsIEventTarget,
                                                  nsIObserver,
                                                  nsIThreadPoolListener,
                                                  nsIDOMThreadService)

nsresult
nsDOMThreadService::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!gDOMThreadService, "Only one instance should ever be created!");

  nsresult rv;
  nsCOMPtr<nsIObserverService> obs =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  obs.forget(&gObserverService);

  mThreadPool = do_CreateInstance(NS_THREADPOOL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetThreadLimit(THREADPOOL_MAX_THREADS);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetIdleThreadLimit(THREADPOOL_IDLE_THREADS);
  NS_ENSURE_SUCCESS(rv, rv);

  mMonitor = nsAutoMonitor::NewMonitor("nsDOMThreadService::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  PRBool success = mWorkersInProgress.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIJSRuntimeService>
    runtimeSvc(do_GetService("@mozilla.org/js/xpc/RuntimeService;1"));
  NS_ENSURE_TRUE(runtimeSvc, NS_ERROR_FAILURE);
  runtimeSvc.forget(&gJSRuntimeService);

  nsCOMPtr<nsIThreadJSContextStack>
    contextStack(do_GetService("@mozilla.org/js/xpc/ContextStack;1"));
  NS_ENSURE_TRUE(contextStack, NS_ERROR_FAILURE);
  contextStack.forget(&gThreadJSContextStack);

  nsCOMPtr<nsIXPCSecurityManager> secMan(new nsDOMWorkerSecurityManager());
  NS_ENSURE_TRUE(secMan, NS_ERROR_OUT_OF_MEMORY);
  secMan.forget(&gWorkerSecurityManager);

  if (gJSContextIndex == BAD_TLS_INDEX &&
      PR_NewThreadPrivateIndex(&gJSContextIndex, NULL) != PR_SUCCESS) {
    NS_ERROR("PR_NewThreadPrivateIndex failed!");
    gJSContextIndex = BAD_TLS_INDEX;
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


already_AddRefed<nsIDOMThreadService>
nsDOMThreadService::GetOrInitService()
{
  if (!gDOMThreadService) {
    nsRefPtr<nsDOMThreadService> service = new nsDOMThreadService();
    NS_ENSURE_TRUE(service, nsnull);

    nsresult rv = service->Init();
    NS_ENSURE_SUCCESS(rv, nsnull);

    service.swap(gDOMThreadService);
  }

  nsCOMPtr<nsIDOMThreadService> service(gDOMThreadService);
  return service.forget();
}


nsDOMThreadService*
nsDOMThreadService::get()
{
  return gDOMThreadService;
}


void
nsDOMThreadService::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_IF_RELEASE(gDOMThreadService);
}

void
nsDOMThreadService::Cleanup()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
  
  

  if (gObserverService) {
    gObserverService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    NS_RELEASE(gObserverService);
  }

  
  
  if (mThreadPool) {
    mThreadPool->Shutdown();
    mThreadPool = nsnull;
  }

  
  if (gThreadJSContextStack) {
    JSContext* safeContext;
    if (NS_SUCCEEDED(gThreadJSContextStack->GetSafeJSContext(&safeContext))) {
      JS_GC(safeContext);
    }
    NS_RELEASE(gThreadJSContextStack);
  }

  
  NS_IF_RELEASE(gJSRuntimeService);
  NS_IF_RELEASE(gWorkerSecurityManager);
}

nsresult
nsDOMThreadService::Dispatch(nsDOMWorkerThread* aWorker,
                             nsIRunnable* aRunnable)
{
  NS_ASSERTION(aWorker, "Null pointer!");
  NS_ASSERTION(aRunnable, "Null pointer!");

  NS_ASSERTION(mThreadPool, "Dispatch called after 'xpcom-shutdown'!");

  if (aWorker->IsCanceled()) {
    LOG(("Will not dispatch runnable [0x%p] for canceled worker [0x%p]",
         static_cast<void*>(aRunnable), static_cast<void*>(aWorker)));
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<nsDOMWorkerRunnable> workerRunnable;
  {
    nsAutoMonitor mon(mMonitor);

    if (mWorkersInProgress.Get(aWorker, getter_AddRefs(workerRunnable))) {
      workerRunnable->PutRunnable(aRunnable);
      return NS_OK;
    }

    workerRunnable = new nsDOMWorkerRunnable(aWorker);
    NS_ENSURE_TRUE(workerRunnable, NS_ERROR_OUT_OF_MEMORY);

    workerRunnable->PutRunnable(aRunnable);

    PRBool success = mWorkersInProgress.Put(aWorker, workerRunnable);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  nsresult rv = mThreadPool->Dispatch(workerRunnable, NS_DISPATCH_NORMAL);

  
  
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch runnable to thread pool!");

    nsAutoMonitor mon(mMonitor);

    
    
    nsRefPtr<nsDOMWorkerRunnable> tableRunnable;
    if (mWorkersInProgress.Get(aWorker, getter_AddRefs(tableRunnable)) &&
        workerRunnable == tableRunnable) {
      mWorkersInProgress.Remove(aWorker);

      
      mon.NotifyAll();
    }

    return rv;
  }

  return NS_OK;
}

void
nsDOMThreadService::WorkerComplete(nsDOMWorkerRunnable* aRunnable)
{

  

#ifdef DEBUG
  nsRefPtr<nsDOMWorkerThread>& debugWorker = aRunnable->mWorker;

  nsRefPtr<nsDOMWorkerRunnable> runnable;
  NS_ASSERTION(mWorkersInProgress.Get(debugWorker, getter_AddRefs(runnable)) &&
               runnable == aRunnable,
               "Removing a worker that isn't in our hashtable?!");
#endif

  mWorkersInProgress.Remove(aRunnable->mWorker);
}

void
nsDOMThreadService::WaitForCanceledWorker(nsDOMWorkerThread* aWorker)
{
  NS_ASSERTION(aWorker->IsCanceled(),
               "Waiting on a worker that isn't canceled!");

  nsAutoMonitor mon(mMonitor);

  while (mWorkersInProgress.Get(aWorker, nsnull)) {
    mon.Wait();
  }
}


JSContext*
nsDOMThreadService::CreateJSContext()
{
  JSRuntime* rt;
  gJSRuntimeService->GetRuntime(&rt);
  NS_ENSURE_TRUE(rt, nsnull);

  JSAutoContextDestroyer cx(JS_NewContext(rt, 8192));
  NS_ENSURE_TRUE(cx, nsnull);

  JS_SetErrorReporter(cx, DOMWorkerErrorReporter);

  JS_SetOperationCallback(cx, DOMWorkerOperationCallback,
                          100 * JS_OPERATION_WEIGHT_BASE);

  static JSSecurityCallbacks securityCallbacks = {
    nsDOMWorkerSecurityManager::JSCheckAccess,
    nsDOMWorkerSecurityManager::JSTranscodePrincipals,
    nsDOMWorkerSecurityManager::JSFindPrincipal
  };

  JS_SetContextSecurityCallbacks(cx, &securityCallbacks);

  nsresult rv = nsContentUtils::XPConnect()->
    SetSecurityManagerForJSContext(cx, gWorkerSecurityManager, 0);
  NS_ENSURE_SUCCESS(rv, nsnull);

  return cx.forget();
}

#define LOOP_OVER_POOLS(_func, _args)                     \
  PR_BEGIN_MACRO                                          \
    PRUint32 poolCount = mPools.Length();                 \
    for (PRUint32 i = 0; i < poolCount; i++) {            \
      mPools[i]-> _func _args ;                           \
    }                                                     \
  PR_END_MACRO

void
nsDOMThreadService::CancelWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  LOOP_OVER_POOLS(CancelWorkersForGlobal, (aGlobalObject));
}

void
nsDOMThreadService::SuspendWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  LOOP_OVER_POOLS(SuspendWorkersForGlobal, (aGlobalObject));
}

void
nsDOMThreadService::ResumeWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  LOOP_OVER_POOLS(ResumeWorkersForGlobal, (aGlobalObject));
}

void
nsDOMThreadService::NoteDyingPool(nsDOMWorkerPool* aPool)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ASSERTION(mPools.Contains(aPool), "aPool should be in the array!");
  mPools.RemoveElement(aPool);
}

void
nsDOMThreadService::TimeoutReady(nsDOMWorkerTimeout* aTimeout)
{
  nsRefPtr<nsDOMWorkerTimeoutRunnable> runnable =
    new nsDOMWorkerTimeoutRunnable(aTimeout);
  NS_ENSURE_TRUE(runnable,);

  Dispatch(aTimeout->GetWorker(), runnable);
}

nsresult
nsDOMThreadService::ChangeThreadPoolMaxThreads(PRInt16 aDelta)
{
  NS_ENSURE_ARG(aDelta == 1 || aDelta == -1);

  PRUint32 currentThreadCount;
  nsresult rv = mThreadPool->GetThreadLimit(&currentThreadCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 newThreadCount = (PRInt32)currentThreadCount + (PRInt32)aDelta;
  NS_ASSERTION(newThreadCount >= THREADPOOL_MAX_THREADS,
               "Can't go below initial thread count!");

  if (newThreadCount > THREADPOOL_THREAD_CAP) {
    NS_WARNING("Thread pool cap reached!");
    return NS_ERROR_FAILURE;
  }

  rv = mThreadPool->SetThreadLimit((PRUint32)newThreadCount);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsIJSRuntimeService*
nsDOMThreadService::JSRuntimeService()
{
  return gJSRuntimeService;
}

nsIThreadJSContextStack*
nsDOMThreadService::ThreadJSContextStack()
{
  return gThreadJSContextStack;
}

nsIXPCSecurityManager*
nsDOMThreadService::WorkerSecurityManager()
{
  return gWorkerSecurityManager;
}




NS_IMETHODIMP
nsDOMThreadService::Dispatch(nsIRunnable* aEvent,
                             PRUint32 aFlags)
{
  NS_ENSURE_ARG_POINTER(aEvent);
  NS_ENSURE_FALSE(aFlags & NS_DISPATCH_SYNC, NS_ERROR_NOT_IMPLEMENTED);

  
  

  aEvent->Run();

  return NS_OK;
}




NS_IMETHODIMP
nsDOMThreadService::IsOnCurrentThread(PRBool* _retval)
{
  NS_NOTREACHED("No one should call this!");
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP
nsDOMThreadService::Observe(nsISupports* aSubject,
                            const char* aTopic,
                            const PRUnichar* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    Cleanup();
    return NS_OK;
  }

  NS_NOTREACHED("Unknown observer topic!");
  return NS_OK;
}




NS_IMETHODIMP
nsDOMThreadService::OnThreadCreated()
{
  LOG(("Thread created"));

  nsIThread* current = NS_GetCurrentThread();

  
  
  
  nsCOMPtr<nsISupportsPriority> priority(do_QueryInterface(current));
  NS_ENSURE_TRUE(priority, NS_ERROR_FAILURE);

  nsresult rv = priority->SetPriority(nsISupportsPriority::PRIORITY_LOWEST);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(gJSContextIndex != BAD_TLS_INDEX, "No context index!");

  
  JSContext* cx = (JSContext*)PR_GetThreadPrivate(gJSContextIndex);
  if (!cx) {
    cx = nsDOMThreadService::CreateJSContext();
    NS_ENSURE_TRUE(cx, NS_ERROR_FAILURE);

    PRStatus status = PR_SetThreadPrivate(gJSContextIndex, cx);
    if (status != PR_SUCCESS) {
      NS_WARNING("Failed to set context on thread!");
      nsContentUtils::XPConnect()->ReleaseJSContext(cx, PR_TRUE);
      return NS_ERROR_FAILURE;
    }
  }

  
  gThreadJSContextStack->Push(cx);
  gThreadJSContextStack->SetSafeJSContext(cx);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMThreadService::OnThreadShuttingDown()
{
  LOG(("Thread shutting down"));

  NS_ASSERTION(gJSContextIndex != BAD_TLS_INDEX, "No context index!");

  JSContext* cx = (JSContext*)PR_GetThreadPrivate(gJSContextIndex);
  NS_WARN_IF_FALSE(cx, "Thread died with no context?");
  if (cx) {
    JSContext* pushedCx;
    gThreadJSContextStack->Pop(&pushedCx);
    NS_ASSERTION(pushedCx == cx, "Popped the wrong context!");

    gThreadJSContextStack->SetSafeJSContext(nsnull);

    nsContentUtils::XPConnect()->ReleaseJSContext(cx, PR_TRUE);
  }

  return NS_OK;
}




NS_IMETHODIMP
nsDOMThreadService::CreatePool(nsIDOMWorkerPool** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_TRUE(mThreadPool, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);

  nsIDOMDocument* domDocument = nsContentUtils::GetDocumentFromCaller();
  NS_ENSURE_TRUE(domDocument, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDocument> callingDocument(do_QueryInterface(domDocument));
  NS_ENSURE_TRUE(callingDocument, NS_ERROR_NO_INTERFACE);

  nsRefPtr<nsDOMWorkerPool> pool(new nsDOMWorkerPool(callingDocument));
  NS_ENSURE_TRUE(pool, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = pool->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!mPools.Contains(pool), "Um?!");
  mPools.AppendElement(pool);

  NS_ADDREF(*_retval = pool);
  return NS_OK;
}
