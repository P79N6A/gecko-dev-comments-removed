






































#include "nsDOMThreadService.h"


#include "nsIComponentManager.h"
#include "nsIConsoleService.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNavigator.h"
#include "nsIDOMWindowInternal.h"
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
#include "nsPIDOMWindow.h"


#include "jscntxt.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsDeque.h"
#include "nsIClassInfoImpl.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMCID.h"
#include "nsXPCOMCIDInternal.h"
#include "pratom.h"
#include "prthread.h"


#include "nsDOMWorker.h"
#include "nsDOMWorkerEvents.h"
#include "nsDOMWorkerMacros.h"
#include "nsDOMWorkerMessageHandler.h"
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


#define BAD_TLS_INDEX (PRUintn)-1


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




class nsReportErrorRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  nsReportErrorRunnable(nsDOMWorker* aWorker,
                        nsIScriptError* aScriptError)
  : mWorker(aWorker), mWorkerWN(aWorker->GetWrappedNative()),
    mScriptError(aScriptError) {
      NS_ASSERTION(aScriptError, "Null pointer!");
    }

  NS_IMETHOD Run() {
    if (mWorker->IsCanceled()) {
      return NS_OK;
    }

#ifdef DEBUG
    {
      nsRefPtr<nsDOMWorker> parent = mWorker->GetParent();
      if (NS_IsMainThread()) {
        NS_ASSERTION(!parent, "Shouldn't have a parent on the main thread!");
      }
      else {
        NS_ASSERTION(parent, "Should have a parent!");

        JSContext* cx = nsDOMThreadService::get()->GetCurrentContext();
        NS_ASSERTION(cx, "No context!");

        nsDOMWorker* currentWorker = (nsDOMWorker*)JS_GetContextPrivate(cx);
        NS_ASSERTION(currentWorker == parent, "Wrong worker!");
      }
    }
#endif

    NS_NAMED_LITERAL_STRING(errorStr, "error");

    nsresult rv;

    if (mWorker->mOuterHandler->HasListeners(errorStr)) {
      
      nsString message;
      rv = mScriptError->GetErrorMessage(message);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString filename;
      rv = mScriptError->GetSourceName(filename);
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 lineno;
      rv = mScriptError->GetLineNumber(&lineno);
      NS_ENSURE_SUCCESS(rv, rv);

      nsRefPtr<nsDOMWorkerErrorEvent> event(new nsDOMWorkerErrorEvent());
      NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

      rv = event->InitErrorEvent(errorStr, PR_FALSE, PR_TRUE, message,
                                 filename, lineno);
      NS_ENSURE_SUCCESS(rv, rv);

      event->SetTarget(mWorker);

      PRBool stopPropagation = PR_FALSE;
      rv = mWorker->DispatchEvent(static_cast<nsDOMWorkerEvent*>(event),
                                  &stopPropagation);
      if (NS_SUCCEEDED(rv) && stopPropagation) {
        return NS_OK;
      }
    }

    nsRefPtr<nsDOMWorker> parent = mWorker->GetParent();
    if (!parent) {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
      nsCOMPtr<nsIConsoleService> consoleService =
        do_GetService(NS_CONSOLESERVICE_CONTRACTID);
      if (consoleService) {
        rv = consoleService->LogMessage(mScriptError);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      return NS_OK;
    }

    nsRefPtr<nsReportErrorRunnable> runnable =
      new nsReportErrorRunnable(parent, mScriptError);
    if (runnable) {
      nsRefPtr<nsDOMWorker> grandparent = parent->GetParent();
      rv = grandparent ?
           nsDOMThreadService::get()->Dispatch(grandparent, runnable) :
           NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

private:
  nsRefPtr<nsDOMWorker> mWorker;
  nsCOMPtr<nsIXPConnectWrappedNative> mWorkerWN;
  nsCOMPtr<nsIScriptError> mScriptError;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsReportErrorRunnable, nsIRunnable)




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
  nsDOMWorkerRunnable(nsDOMWorker* aWorker)
  : mWorker(aWorker) { }

  virtual ~nsDOMWorkerRunnable() {
    nsCOMPtr<nsIRunnable> runnable;
    while ((runnable = dont_AddRef((nsIRunnable*)mRunnables.PopFront()))) {
      
    }
  }

  void PutRunnable(nsIRunnable* aRunnable) {
    NS_ASSERTION(aRunnable, "Null pointer!");

    NS_ADDREF(aRunnable);

    

    mRunnables.Push(aRunnable);
  }

  NS_IMETHOD Run() {
    NS_ASSERTION(!NS_IsMainThread(),
                 "This should *never* run on the main thread!");

    
    NS_ASSERTION(gJSContextIndex != BAD_TLS_INDEX, "No context index!");

    
    JSContext* cx = (JSContext*)PR_GetThreadPrivate(gJSContextIndex);
    NS_ASSERTION(cx, "nsDOMThreadService didn't give us a context!");

    NS_ASSERTION(!JS_GetGlobalObject(cx), "Shouldn't have a global!");

    JS_SetContextPrivate(cx, mWorker);

    
    if (mWorker->SetGlobalForContext(cx)) {
      RunQueue(cx);

      
      
      JS_SetGlobalObject(cx, NULL);
      JS_SetContextPrivate(cx, NULL);
    }
    else {
      
      JS_SetGlobalObject(cx, NULL);
      JS_SetContextPrivate(cx, NULL);

      nsAutoMonitor mon(gDOMThreadService->mMonitor);
      gDOMThreadService->WorkerComplete(this);
      mon.NotifyAll();
    }

    return NS_OK;
  }

protected:

  void RunQueue(JSContext* aCx) {
    PRBool operationCallbackTriggered = PR_FALSE;

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

      if (!operationCallbackTriggered) {
        
        
        JS_TriggerOperationCallback(aCx);

        
        operationCallbackTriggered = PR_TRUE;
      }

      
      JS_ClearRegExpStatics(aCx);

      runnable->Run();
    }
  }

  
  nsRefPtr<nsDOMWorker> mWorker;

  
  nsDeque mRunnables;
};





JSBool
DOMWorkerOperationCallback(JSContext* aCx)
{
  nsDOMWorker* worker = (nsDOMWorker*)JS_GetContextPrivate(aCx);

  
  
  nsRefPtr<nsDOMWorkerPool> pool;

  PRBool wasSuspended = PR_FALSE;
  PRBool extraThreadAllowed = PR_FALSE;
  jsrefcount suspendDepth = 0;

  for (;;) {
    
    if (worker->IsCanceled()) {
      LOG(("Forcefully killing JS for worker [0x%p]",
           static_cast<void*>(worker)));

      if (wasSuspended) {
        if (extraThreadAllowed) {
          gDOMThreadService->ChangeThreadPoolMaxThreads(-1);
        }
        JS_ResumeRequest(aCx, suspendDepth);
      }

      
      JS_ClearPendingException(aCx);
      return JS_FALSE;
    }

    
    if (!worker->IsSuspended()) {
      if (wasSuspended) {
        if (extraThreadAllowed) {
          gDOMThreadService->ChangeThreadPoolMaxThreads(-1);
        }
        JS_ResumeRequest(aCx, suspendDepth);
      }
      return JS_TRUE;
    }

    if (!wasSuspended) {
      
      
      if (worker->IsCanceled()) {
        NS_WARNING("Tried to suspend on a pool that has gone away");
        JS_ClearPendingException(aCx);
        return JS_FALSE;
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
}

void
DOMWorkerErrorReporter(JSContext* aCx,
                       const char* aMessage,
                       JSErrorReport* aReport)
{
  NS_ASSERTION(!NS_IsMainThread(), "Huh?!");

  nsDOMWorker* worker = (nsDOMWorker*)JS_GetContextPrivate(aCx);

  if (worker->IsCanceled()) {
    
    
    
    return;
  }

  if (worker->mErrorHandlerRecursionCount == 2) {
    
    return;
  }

  nsresult rv;
  nsCOMPtr<nsIScriptError> scriptError =
    do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,);

  const PRUnichar* message =
    reinterpret_cast<const PRUnichar*>(aReport->ucmessage);

  nsAutoString filename;
  filename.AssignWithConversion(aReport->filename);

  const PRUnichar* line =
    reinterpret_cast<const PRUnichar*>(aReport->uclinebuf);

  PRUint32 column = aReport->uctokenptr - aReport->uclinebuf;

  rv = scriptError->Init(message, filename.get(), line, aReport->lineno,
                         column, aReport->flags, "DOM Worker javascript");
  NS_ENSURE_SUCCESS(rv,);

  
  if (aReport->errorNumber != JSMSG_SCRIPT_STACK_QUOTA &&
      aReport->errorNumber != JSMSG_OVER_RECURSED) {
    
    nsCOMPtr<nsIDOMEventListener> handler =
      worker->mInnerHandler->GetOnXListener(NS_LITERAL_STRING("error"));

    if (handler) {
      nsRefPtr<nsDOMWorkerErrorEvent> event(new nsDOMWorkerErrorEvent());
      if (event) {
        rv = event->InitErrorEvent(NS_LITERAL_STRING("error"), PR_FALSE, PR_TRUE,
                                   nsDependentString(message), filename,
                                   aReport->lineno);
        if (NS_SUCCEEDED(rv)) {
          NS_ASSERTION(worker->GetInnerScope(), "Null scope!");
          event->SetTarget(worker->GetInnerScope());

          NS_ASSERTION(worker->mErrorHandlerRecursionCount >= 0,
                       "Bad recursion count logic!");
          worker->mErrorHandlerRecursionCount++;

          handler->HandleEvent(static_cast<nsDOMWorkerEvent*>(event));

          worker->mErrorHandlerRecursionCount--;

          if (event->PreventDefaultCalled()) {
            return;
          }
        }
      }
    }
  }

  
  nsCOMPtr<nsIRunnable> runnable =
    new nsReportErrorRunnable(worker, scriptError);
  NS_ENSURE_TRUE(runnable,);

  nsRefPtr<nsDOMWorker> parent = worker->GetParent();

  
  
  
  rv = parent ? nsDOMThreadService::get()->Dispatch(parent, runnable)
              : NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv,);
}





nsDOMThreadService::nsDOMThreadService()
: mMonitor(nsnull),
  mNavigatorStringsLoaded(PR_FALSE)
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

NS_IMPL_THREADSAFE_ISUPPORTS3(nsDOMThreadService, nsIEventTarget,
                                                  nsIObserver,
                                                  nsIThreadPoolListener)

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

  success = mPools.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mJSContexts.SetCapacity(THREADPOOL_THREAD_CAP);
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


already_AddRefed<nsDOMThreadService>
nsDOMThreadService::GetOrInitService()
{
  if (!gDOMThreadService) {
    nsRefPtr<nsDOMThreadService> service = new nsDOMThreadService();
    NS_ENSURE_TRUE(service, nsnull);

    nsresult rv = service->Init();
    NS_ENSURE_SUCCESS(rv, nsnull);

    service.swap(gDOMThreadService);
  }

  nsRefPtr<nsDOMThreadService> service(gDOMThreadService);
  return service.forget();
}


nsDOMThreadService*
nsDOMThreadService::get()
{
  return gDOMThreadService;
}


JSContext*
nsDOMThreadService::GetCurrentContext()
{
  JSContext* cx;

  if (NS_IsMainThread()) {
    nsresult rv = ThreadJSContextStack()->GetSafeJSContext(&cx);
    NS_ENSURE_SUCCESS(rv, nsnull);
  }
  else {
    NS_ENSURE_TRUE(gJSContextIndex, nsnull);
    cx = static_cast<JSContext*>(PR_GetThreadPrivate(gJSContextIndex));
  }

  return cx;
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

  nsAutoMonitor mon(mMonitor);
  NS_ASSERTION(!mPools.Count(), "Live workers left!");

  mPools.Clear();
}

nsresult
nsDOMThreadService::Dispatch(nsDOMWorker* aWorker,
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
  nsRefPtr<nsDOMWorker>& debugWorker = aRunnable->mWorker;

  nsRefPtr<nsDOMWorkerRunnable> runnable;
  NS_ASSERTION(mWorkersInProgress.Get(debugWorker, getter_AddRefs(runnable)) &&
               runnable == aRunnable,
               "Removing a worker that isn't in our hashtable?!");
#endif

  mWorkersInProgress.Remove(aRunnable->mWorker);
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

  JS_SetOperationCallback(cx, DOMWorkerOperationCallback);

  static JSSecurityCallbacks securityCallbacks = {
    nsDOMWorkerSecurityManager::JSCheckAccess,
    nsDOMWorkerSecurityManager::JSTranscodePrincipals,
    nsDOMWorkerSecurityManager::JSFindPrincipal
  };

  JS_SetContextSecurityCallbacks(cx, &securityCallbacks);

  nsresult rv = nsContentUtils::XPConnect()->
    SetSecurityManagerForJSContext(cx, gWorkerSecurityManager, 0);
  NS_ENSURE_SUCCESS(rv, nsnull);

  PRUint32 stackDummy;
  jsuword stackLimit, currentStackAddr = (jsuword)&stackDummy;

  
  const jsuword kStackSize = 0x40000;

#if JS_STACK_GROWTH_DIRECTION < 0
  stackLimit = (currentStackAddr > kStackSize) ?
               currentStackAddr - kStackSize :
               0;
#else
  stackLimit = (currentStackAddr + kStackSize > currentStackAddr) ?
               currentStackAddr + kStackSize :
               (jsuword) -1;
#endif

  JS_SetThreadStackLimit(cx, stackLimit);
  JS_SetScriptStackQuota(cx, 100*1024*1024);

  JS_SetOptions(cx, JS_GetOptions(cx) | JSOPTION_JIT | JSOPTION_ANONFUNFIX);

  return cx.forget();
}

already_AddRefed<nsDOMWorkerPool>
nsDOMThreadService::GetPoolForGlobal(nsIScriptGlobalObject* aGlobalObject,
                                     PRBool aRemove)
{
  NS_ASSERTION(aGlobalObject, "Null pointer!");

  nsAutoMonitor mon(mMonitor);

  nsRefPtr<nsDOMWorkerPool> pool;
  mPools.Get(aGlobalObject, getter_AddRefs(pool));

  if (aRemove) {
    mPools.Remove(aGlobalObject);
  }

  return pool.forget();
}

void
nsDOMThreadService::TriggerOperationCallbackForPool(nsDOMWorkerPool* aPool)
{
  nsAutoMonitor mon(mMonitor);

  
  
  PRUint32 contextCount = mJSContexts.Length();
  for (PRUint32 index = 0; index < contextCount; index++) {
    JSContext*& cx = mJSContexts[index];
    nsDOMWorker* worker = (nsDOMWorker*)JS_GetContextPrivate(cx);
    if (worker && worker->Pool() == aPool) {
      JS_TriggerOperationCallback(cx);
    }
  }
}

void
nsDOMThreadService::CancelWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject)
{
  NS_ASSERTION(aGlobalObject, "Null pointer!");

  nsRefPtr<nsDOMWorkerPool> pool = GetPoolForGlobal(aGlobalObject, PR_TRUE);
  if (pool) {
    pool->Cancel();
    TriggerOperationCallbackForPool(pool);
  }
}

void
nsDOMThreadService::SuspendWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject)
{
  NS_ASSERTION(aGlobalObject, "Null pointer!");

  nsRefPtr<nsDOMWorkerPool> pool = GetPoolForGlobal(aGlobalObject, PR_FALSE);
  if (pool) {
    pool->Suspend();
    TriggerOperationCallbackForPool(pool);
  }
}

void
nsDOMThreadService::ResumeWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject)
{
  NS_ASSERTION(aGlobalObject, "Null pointer!");

  nsRefPtr<nsDOMWorkerPool> pool = GetPoolForGlobal(aGlobalObject, PR_FALSE);
  if (pool) {
    pool->Resume();
    TriggerOperationCallbackForPool(pool);
  }
}

void
nsDOMThreadService::NoteEmptyPool(nsDOMWorkerPool* aPool)
{
  NS_ASSERTION(aPool, "Null pointer!");

  nsAutoMonitor mon(mMonitor);
  mPools.Remove(aPool->ScriptGlobalObject());
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

  nsAutoMonitor mon(mMonitor);

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

  
  
  
  if (aDelta == 1) {
    nsCOMPtr<nsIRunnable> dummy(new nsRunnable());
    if (dummy) {
      rv = mThreadPool->Dispatch(dummy, NS_DISPATCH_NORMAL);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

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

    nsAutoMonitor mon(mMonitor);

#ifdef DEBUG
    JSContext** newContext =
#endif
    mJSContexts.AppendElement(cx);

    
    NS_ASSERTION(newContext, "Should never fail!");
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
    {
      nsAutoMonitor mon(mMonitor);
      mJSContexts.RemoveElement(cx);
    }

    JSContext* pushedCx;
    gThreadJSContextStack->Pop(&pushedCx);
    NS_ASSERTION(pushedCx == cx, "Popped the wrong context!");

    gThreadJSContextStack->SetSafeJSContext(nsnull);

    nsContentUtils::XPConnect()->ReleaseJSContext(cx, PR_TRUE);
  }

  return NS_OK;
}

nsresult
nsDOMThreadService::RegisterWorker(nsDOMWorker* aWorker,
                                   nsIScriptGlobalObject* aGlobalObject)
{
  NS_ASSERTION(aWorker, "Null pointer!");
  NS_ASSERTION(aGlobalObject, "Null pointer!");

  if (NS_IsMainThread()) {
    nsCOMPtr<nsPIDOMWindow> domWindow(do_QueryInterface(aGlobalObject));
    NS_ENSURE_TRUE(domWindow, NS_ERROR_NO_INTERFACE);

    nsPIDOMWindow* innerWindow = domWindow->IsOuterWindow() ?
                                 domWindow->GetCurrentInnerWindow() :
                                 domWindow.get();
    NS_ENSURE_STATE(innerWindow);

    nsCOMPtr<nsIScriptGlobalObject> newGlobal(do_QueryInterface(innerWindow));
    NS_ENSURE_TRUE(newGlobal, NS_ERROR_NO_INTERFACE);

    aGlobalObject = newGlobal;
  }

  nsRefPtr<nsDOMWorkerPool> pool;
  {
    nsAutoMonitor mon(mMonitor);

    if (!mThreadPool) {
      
      return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;
    }

    mPools.Get(aGlobalObject, getter_AddRefs(pool));
  }

  nsresult rv;

  if (!pool) {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

    if (!mNavigatorStringsLoaded) {
      nsCOMPtr<nsIDOMWindowInternal> internal(do_QueryInterface(aGlobalObject));
      NS_ENSURE_TRUE(internal, NS_ERROR_NO_INTERFACE);

      nsCOMPtr<nsIDOMNavigator> navigator;
      rv = internal->GetNavigator(getter_AddRefs(navigator));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = navigator->GetAppName(mAppName);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = navigator->GetAppVersion(mAppVersion);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = navigator->GetPlatform(mPlatform);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = navigator->GetUserAgent(mUserAgent);
      NS_ENSURE_SUCCESS(rv, rv);

      mNavigatorStringsLoaded = PR_TRUE;
    }

    nsCOMPtr<nsPIDOMWindow> domWindow(do_QueryInterface(aGlobalObject));
    NS_ENSURE_TRUE(domWindow, NS_ERROR_NO_INTERFACE);

    nsIDOMDocument* domDocument = domWindow->GetExtantDocument();
    NS_ENSURE_STATE(domDocument);

    nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
    NS_ENSURE_STATE(document);

    pool = new nsDOMWorkerPool(aGlobalObject, document);
    NS_ENSURE_TRUE(pool, NS_ERROR_OUT_OF_MEMORY);

    rv = pool->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoMonitor mon(mMonitor);

    PRBool success = mPools.Put(aGlobalObject, pool);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  rv = pool->NoteWorker(aWorker);
  NS_ENSURE_SUCCESS(rv, rv);

  aWorker->SetPool(pool);
  return NS_OK;
}

void
nsDOMThreadService::GetAppName(nsAString& aAppName)
{
  NS_ASSERTION(mNavigatorStringsLoaded,
               "Shouldn't call this before we have loaded strings!");
  aAppName.Assign(mAppName);
}

void
nsDOMThreadService::GetAppVersion(nsAString& aAppVersion)
{
  NS_ASSERTION(mNavigatorStringsLoaded,
               "Shouldn't call this before we have loaded strings!");
  aAppVersion.Assign(mAppVersion);
}

void
nsDOMThreadService::GetPlatform(nsAString& aPlatform)
{
  NS_ASSERTION(mNavigatorStringsLoaded,
               "Shouldn't call this before we have loaded strings!");
  aPlatform.Assign(mPlatform);
}

void
nsDOMThreadService::GetUserAgent(nsAString& aUserAgent)
{
  NS_ASSERTION(mNavigatorStringsLoaded,
               "Shouldn't call this before we have loaded strings!");
  aUserAgent.Assign(mUserAgent);
}
