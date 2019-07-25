






































#include "jscntxt.h"

#include "nsDOMThreadService.h"


#include "nsIComponentManager.h"
#include "nsIConsoleService.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIEventTarget.h"
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


#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsDeque.h"
#include "nsGlobalWindow.h"
#include "nsIClassInfoImpl.h"
#include "nsStringBuffer.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMCID.h"
#include "nsXPCOMCIDInternal.h"
#include "pratom.h"
#include "prthread.h"
#include "mozilla/Preferences.h"


#include "nsDOMWorker.h"
#include "nsDOMWorkerEvents.h"
#include "nsDOMWorkerMacros.h"
#include "nsDOMWorkerMessageHandler.h"
#include "nsDOMWorkerPool.h"
#include "nsDOMWorkerSecurityManager.h"
#include "nsDOMWorkerTimeout.h"

using namespace mozilla;

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

static const char* sPrefsToWatch[] = {
  "dom.max_script_run_time"
};


static PRUint32 gWorkerCloseHandlerTimeoutMS = 10000;





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

class nsDestroyJSContextRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  nsDestroyJSContextRunnable(JSContext* aCx)
  : mCx(aCx)
  {
    NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
    NS_ASSERTION(aCx, "Null pointer!");
    NS_ASSERTION(!JS_GetGlobalObject(aCx), "Should not have a global!");

    
    JS_ClearContextThread(aCx);
  }

  NS_IMETHOD Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

    
    if (!!JS_SetContextThread(mCx)) {
      NS_WARNING("JS_SetContextThread failed!");
    }

    if (nsContentUtils::XPConnect()) {
      nsContentUtils::XPConnect()->ReleaseJSContext(mCx, PR_TRUE);
    }
    else {
      NS_WARNING("Failed to release JSContext!");
    }

    return NS_OK;
  }

private:
  JSContext* mCx;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDestroyJSContextRunnable, nsIRunnable)




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

    if (mWorker->HasListeners(errorStr)) {
      
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

      event->SetTarget(static_cast<nsDOMWorkerMessageHandler*>(mWorker));

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




class nsDOMWorkerTimeoutRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  nsDOMWorkerTimeoutRunnable(nsDOMWorkerTimeout* aTimeout)
  : mTimeout(aTimeout) { }

  NS_IMETHOD Run() {
    return mTimeout->Run();
  }
protected:
  nsRefPtr<nsDOMWorkerTimeout> mTimeout;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWorkerTimeoutRunnable, nsIRunnable)

class nsDOMWorkerKillRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  nsDOMWorkerKillRunnable(nsDOMWorker* aWorker)
  : mWorker(aWorker) { }

  NS_IMETHOD Run() {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    mWorker->Kill();
    return NS_OK;
  }

private:
  nsRefPtr<nsDOMWorker> mWorker;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWorkerKillRunnable, nsIRunnable)










class nsDOMWorkerRunnable : public nsIRunnable
{
  friend class nsDOMThreadService;

public:
  NS_DECL_ISUPPORTS

  nsDOMWorkerRunnable(nsDOMWorker* aWorker)
  : mWorker(aWorker), mCloseTimeoutInterval(0), mKillWorkerWhenDone(PR_FALSE) {
  }

  virtual ~nsDOMWorkerRunnable() {
    ClearQueue();
  }

  void PutRunnable(nsIRunnable* aRunnable,
                   PRIntervalTime aTimeoutInterval,
                   PRBool aClearQueue) {
    NS_ASSERTION(aRunnable, "Null pointer!");

    gDOMThreadService->mReentrantMonitor.AssertCurrentThreadIn();

    if (NS_LIKELY(!aTimeoutInterval)) {
      NS_ADDREF(aRunnable);
      mRunnables.Push(aRunnable);
    }
    else {
      NS_ASSERTION(!mCloseRunnable, "More than one close runnable?!");
      if (aClearQueue) {
        ClearQueue();
      }
      mCloseRunnable = aRunnable;
      mCloseTimeoutInterval = aTimeoutInterval;
      mKillWorkerWhenDone = PR_TRUE;
    }
  }

  void SetCloseRunnableTimeout(PRIntervalTime aTimeoutInterval) {
    NS_ASSERTION(aTimeoutInterval, "No timeout specified!");
    NS_ASSERTION(aTimeoutInterval!= PR_INTERVAL_NO_TIMEOUT, "Bad timeout!");

    

    NS_ASSERTION(mWorker->GetExpirationTime() == PR_INTERVAL_NO_TIMEOUT,
                 "Asked to set timeout on a runnable with no close handler!");

    
    
    
    mWorker->SetExpirationTime(PR_IntervalNow() + aTimeoutInterval);
  }

  NS_IMETHOD Run() {
    NS_ASSERTION(!NS_IsMainThread(),
                 "This should *never* run on the main thread!");

    
    NS_ASSERTION(gJSContextIndex != BAD_TLS_INDEX, "No context index!");

    
    JSContext* cx = (JSContext*)PR_GetThreadPrivate(gJSContextIndex);
    if (!cx) {
        NS_ERROR("nsDOMThreadService didn't give us a context! Are we out of memory?");
        return NS_ERROR_FAILURE;
    }

    NS_ASSERTION(!JS_GetGlobalObject(cx), "Shouldn't have a global!");

    if (mWorker->IsPrivileged()) {
      JS_SetVersion(cx, JSVERSION_LATEST);
    }
    else {
      JS_SetVersion(cx, JSVERSION_DEFAULT);
    }

    JS_SetContextPrivate(cx, mWorker);

    
    
    
    JS_TriggerOperationCallback(cx);

    PRBool killWorkerWhenDone;
    {
      nsLazyAutoRequest ar;
      JSAutoEnterCompartment ac;

      
      if (mWorker->SetGlobalForContext(cx, &ar, &ac)) {
        NS_ASSERTION(ar.entered(), "SetGlobalForContext must enter request on success");
        NS_ASSERTION(ac.entered(), "SetGlobalForContext must enter compartment on success");

        RunQueue(cx, &killWorkerWhenDone);

        
        
        JS_SetGlobalObject(cx, NULL);
        JS_SetContextPrivate(cx, NULL);
      }
      else {
        NS_ASSERTION(!ar.entered(), "SetGlobalForContext must not enter request on failure");
        NS_ASSERTION(!ac.entered(), "SetGlobalForContext must not enter compartment on failure");

        {
          
          
          JSAutoRequest ar2(cx);

          
          JS_SetGlobalObject(cx, NULL);
          JS_SetContextPrivate(cx, NULL);
        }

        ReentrantMonitorAutoEnter mon(gDOMThreadService->mReentrantMonitor);
        killWorkerWhenDone = mKillWorkerWhenDone;
        gDOMThreadService->WorkerComplete(this);
        mon.NotifyAll();
      }
    }

    if (killWorkerWhenDone) {
      nsCOMPtr<nsIRunnable> runnable = new nsDOMWorkerKillRunnable(mWorker);
      NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

      nsresult rv = NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

protected:
  void ClearQueue() {
    nsCOMPtr<nsIRunnable> runnable;
    while ((runnable = dont_AddRef((nsIRunnable*)mRunnables.PopFront()))) {
      
    }
  }

  void RunQueue(JSContext* aCx, PRBool* aCloseRunnableSet) {
    while (1) {
      nsCOMPtr<nsIRunnable> runnable;
      {
        ReentrantMonitorAutoEnter mon(gDOMThreadService->mReentrantMonitor);

        runnable = dont_AddRef((nsIRunnable*)mRunnables.PopFront());

        if (!runnable && mCloseRunnable) {
          PRIntervalTime expirationTime;
          if (mCloseTimeoutInterval == PR_INTERVAL_NO_TIMEOUT) {
            expirationTime = mCloseTimeoutInterval;
          }
          else {
            expirationTime = PR_IntervalNow() + mCloseTimeoutInterval;
          }
          mWorker->SetExpirationTime(expirationTime);

          runnable.swap(mCloseRunnable);
        }

        if (!runnable || mWorker->IsCanceled()) {
#ifdef PR_LOGGING
          if (mWorker->IsCanceled()) {
            LOG(("Bailing out of run loop for canceled worker[0x%p]",
                 static_cast<void*>(mWorker.get())));
          }
#endif
          *aCloseRunnableSet = mKillWorkerWhenDone;
          gDOMThreadService->WorkerComplete(this);
          mon.NotifyAll();
          return;
        }
      }

      
      if (JSObject *global = JS_GetGlobalObject(aCx))
          JS_ClearRegExpStatics(aCx, global);

      runnable->Run();
    }
    NS_NOTREACHED("Shouldn't ever get here!");
  }

  
  nsRefPtr<nsDOMWorker> mWorker;

  
  nsDeque mRunnables;
  nsCOMPtr<nsIRunnable> mCloseRunnable;
  PRIntervalTime mCloseTimeoutInterval;
  PRBool mKillWorkerWhenDone;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWorkerRunnable, nsIRunnable)





JSBool
DOMWorkerOperationCallback(JSContext* aCx)
{
  nsDOMWorker* worker = (nsDOMWorker*)JS_GetContextPrivate(aCx);
  NS_ASSERTION(worker, "This must never be null!");

  PRBool canceled = worker->IsCanceled();
  if (!canceled && worker->IsSuspended()) {
    JSAutoSuspendRequest suspended(aCx);

    
    
    
    PRBool extraThreadAllowed =
      NS_SUCCEEDED(gDOMThreadService->ChangeThreadPoolMaxThreads(1));

    
    
    JS_FlushCaches(aCx);

    for (;;) {
      ReentrantMonitorAutoEnter mon(worker->Pool()->GetReentrantMonitor());

      
      
      
      
      canceled = worker->IsCanceled();
      if (!canceled && worker->IsSuspended()) {
        mon.Wait();
      }
      else {
        break;
      }
    }

    if (extraThreadAllowed) {
      gDOMThreadService->ChangeThreadPoolMaxThreads(-1);
    }
  }

  if (canceled) {
    LOG(("Forcefully killing JS for worker [0x%p]",
         static_cast<void*>(worker)));
    
    JS_ClearPendingException(aCx);
    return JS_FALSE;
  }
  return JS_TRUE;
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
  nsCOMPtr<nsIScriptError> scriptError;

  {
    
    JSAutoSuspendRequest ar(aCx);

    scriptError = do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
  }

  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIScriptError2> scriptError2(do_QueryInterface(scriptError));

  nsAutoString message, filename, line;
  PRUint32 lineNumber, columnNumber, flags, errorNumber;

  if (aReport) {
    if (aReport->ucmessage) {
      message.Assign(reinterpret_cast<const PRUnichar*>(aReport->ucmessage));
    }
    filename.AssignWithConversion(aReport->filename);
    line.Assign(reinterpret_cast<const PRUnichar*>(aReport->uclinebuf));
    lineNumber = aReport->lineno;
    columnNumber = aReport->uctokenptr - aReport->uclinebuf;
    flags = aReport->flags;
    errorNumber = aReport->errorNumber;
  }
  else {
    lineNumber = columnNumber = errorNumber = 0;
    flags = nsIScriptError::errorFlag | nsIScriptError::exceptionFlag;
  }

  if (message.IsEmpty()) {
    message.AssignWithConversion(aMessage);
  }

  rv = scriptError2->InitWithWindowID(message.get(), filename.get(), line.get(),
                                      lineNumber, columnNumber, flags,
                                      "DOM Worker javascript",
                                      worker->Pool()->WindowID());

  if (NS_FAILED(rv)) {
    return;
  }

  
  if (errorNumber != JSMSG_SCRIPT_STACK_QUOTA &&
      errorNumber != JSMSG_OVER_RECURSED) {
    
    nsRefPtr<nsDOMWorkerScope> scope = worker->GetInnerScope();
    NS_ASSERTION(scope, "Null scope!");

    PRBool hasListeners = scope->HasListeners(NS_LITERAL_STRING("error"));
    if (hasListeners) {
      nsRefPtr<nsDOMWorkerErrorEvent> event(new nsDOMWorkerErrorEvent());
      if (event) {
        rv = event->InitErrorEvent(NS_LITERAL_STRING("error"), PR_FALSE,
                                   PR_TRUE, nsDependentString(message),
                                   filename, lineNumber);
        if (NS_SUCCEEDED(rv)) {
          event->SetTarget(scope);

          NS_ASSERTION(worker->mErrorHandlerRecursionCount >= 0,
                       "Bad recursion count logic!");
          worker->mErrorHandlerRecursionCount++;

          PRBool preventDefaultCalled = PR_FALSE;
          scope->DispatchEvent(static_cast<nsDOMWorkerEvent*>(event),
                               &preventDefaultCalled);

          worker->mErrorHandlerRecursionCount--;

          if (preventDefaultCalled) {
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
  if (NS_FAILED(rv)) {
    return;
  }
}





nsDOMThreadService::nsDOMThreadService()
: mReentrantMonitor("nsDOMThreadServer.mReentrantMonitor"),
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

  RegisterPrefCallbacks();

  mThreadPool = do_CreateInstance(NS_THREADPOOL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetThreadLimit(THREADPOOL_MAX_THREADS);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetIdleThreadLimit(THREADPOOL_IDLE_THREADS);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mWorkersInProgress.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mPools.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mThreadsafeContractIDs.Init();
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

  
  
  

  
  CancelWorkersForGlobal(nsnull);

  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    NS_ASSERTION(!mPools.Count(), "Live workers left!");
    mPools.Clear();

    NS_ASSERTION(!mSuspendedWorkers.Length(), "Suspended workers left!");
    mSuspendedWorkers.Clear();
  }

  if (gObserverService) {
    gObserverService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    NS_RELEASE(gObserverService);

    UnregisterPrefCallbacks();
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
nsDOMThreadService::Dispatch(nsDOMWorker* aWorker,
                             nsIRunnable* aRunnable,
                             PRIntervalTime aTimeoutInterval,
                             PRBool aClearQueue)
{
  NS_ASSERTION(aWorker, "Null pointer!");
  NS_ASSERTION(aRunnable, "Null pointer!");

  if (!mThreadPool) {
    
    
    NS_ASSERTION(NS_IsMainThread(),
                 "This should be impossible on a non-main thread!");
    return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;
  }

  
  
  
  if (aWorker->IsClosing() && !aTimeoutInterval) {
    LOG(("Will not dispatch runnable [0x%p] for closing worker [0x%p]",
         static_cast<void*>(aRunnable), static_cast<void*>(aWorker)));
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<nsDOMWorkerRunnable> workerRunnable;
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    if (mWorkersInProgress.Get(aWorker, getter_AddRefs(workerRunnable))) {
      workerRunnable->PutRunnable(aRunnable, aTimeoutInterval, aClearQueue);
      return NS_OK;
    }

    workerRunnable = new nsDOMWorkerRunnable(aWorker);
    NS_ENSURE_TRUE(workerRunnable, NS_ERROR_OUT_OF_MEMORY);

    workerRunnable->PutRunnable(aRunnable, aTimeoutInterval, PR_FALSE);

    PRBool success = mWorkersInProgress.Put(aWorker, workerRunnable);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  nsresult rv = mThreadPool->Dispatch(workerRunnable, NS_DISPATCH_NORMAL);

  
  
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch runnable to thread pool!");

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    
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
nsDOMThreadService::SetWorkerTimeout(nsDOMWorker* aWorker,
                                     PRIntervalTime aTimeoutInterval)
{
  NS_ASSERTION(aWorker, "Null pointer!");
  NS_ASSERTION(aTimeoutInterval, "No timeout specified!");

  NS_ASSERTION(mThreadPool, "Dispatch called after 'xpcom-shutdown'!");

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  nsRefPtr<nsDOMWorkerRunnable> workerRunnable;
  if (mWorkersInProgress.Get(aWorker, getter_AddRefs(workerRunnable))) {
    workerRunnable->SetCloseRunnableTimeout(aTimeoutInterval);
  }
}

void
nsDOMThreadService::WorkerComplete(nsDOMWorkerRunnable* aRunnable)
{
  mReentrantMonitor.AssertCurrentThreadIn();

#ifdef DEBUG
  nsRefPtr<nsDOMWorker>& debugWorker = aRunnable->mWorker;

  nsRefPtr<nsDOMWorkerRunnable> runnable;
  NS_ASSERTION(mWorkersInProgress.Get(debugWorker, getter_AddRefs(runnable)) &&
               runnable == aRunnable,
               "Removing a worker that isn't in our hashtable?!");
#endif

  mWorkersInProgress.Remove(aRunnable->mWorker);
}

PRBool
nsDOMThreadService::QueueSuspendedWorker(nsDOMWorkerRunnable* aRunnable)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

#ifdef DEBUG
    {
      
      nsRefPtr<nsDOMWorkerRunnable> current;
      mWorkersInProgress.Get(aRunnable->mWorker, getter_AddRefs(current));
      NS_ASSERTION(current == aRunnable, "Something crazy wrong here!");
    }
#endif

  return mSuspendedWorkers.AppendElement(aRunnable) ? PR_TRUE : PR_FALSE;
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

  JS_ClearContextDebugHooks(cx);

  nsresult rv = nsContentUtils::XPConnect()->
    SetSecurityManagerForJSContext(cx, gWorkerSecurityManager, 0);
  NS_ENSURE_SUCCESS(rv, nsnull);

  JS_SetNativeStackQuota(cx, 256*1024);
  JS_SetScriptStackQuota(cx, 100*1024*1024);

  JS_SetOptions(cx,
    JS_GetOptions(cx) | JSOPTION_METHODJIT | JSOPTION_JIT |
    JSOPTION_PROFILING | JSOPTION_ANONFUNFIX);
  JS_SetGCParameterForThread(cx, JSGC_MAX_CODE_CACHE_BYTES, 1 * 1024 * 1024);

  return cx.forget();
}

already_AddRefed<nsDOMWorkerPool>
nsDOMThreadService::GetPoolForGlobal(nsIScriptGlobalObject* aGlobalObject,
                                     PRBool aRemove)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

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
  mReentrantMonitor.AssertCurrentThreadIn();

  
  
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
nsDOMThreadService::RescheduleSuspendedWorkerForPool(nsDOMWorkerPool* aPool)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  PRUint32 count = mSuspendedWorkers.Length();
  if (!count) {
    
    return;
  }

  nsTArray<nsDOMWorkerRunnable*> others(count);

  for (PRUint32 index = 0; index < count; index++) {
    nsDOMWorkerRunnable* runnable = mSuspendedWorkers[index];

#ifdef DEBUG
    {
      
      nsRefPtr<nsDOMWorkerRunnable> current;
      mWorkersInProgress.Get(runnable->mWorker, getter_AddRefs(current));
      NS_ASSERTION(current == runnable, "Something crazy wrong here!");
    }
#endif

    if (runnable->mWorker->Pool() == aPool) {
#ifdef DEBUG
      nsresult rv =
#endif
      mThreadPool->Dispatch(runnable, NS_DISPATCH_NORMAL);
      NS_ASSERTION(NS_SUCCEEDED(rv), "This shouldn't ever fail!");
    }
    else {
      others.AppendElement(runnable);
    }
  }

  mSuspendedWorkers.SwapElements(others);
}

void
nsDOMThreadService::CancelWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject)
{
  nsRefPtr<nsDOMWorkerPool> pool = GetPoolForGlobal(aGlobalObject, PR_TRUE);
  if (pool) {
    pool->Cancel();

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    TriggerOperationCallbackForPool(pool);
    RescheduleSuspendedWorkerForPool(pool);
  }
}

void
nsDOMThreadService::SuspendWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject)
{
  NS_ASSERTION(aGlobalObject, "Null pointer!");

  nsRefPtr<nsDOMWorkerPool> pool = GetPoolForGlobal(aGlobalObject, PR_FALSE);
  if (pool) {
    pool->Suspend();

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
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

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    TriggerOperationCallbackForPool(pool);
    RescheduleSuspendedWorkerForPool(pool);
  }
}

void
nsDOMThreadService::NoteEmptyPool(nsDOMWorkerPool* aPool)
{
  NS_ASSERTION(aPool, "Null pointer!");

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
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

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

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

void
nsDOMThreadService::NoteThreadsafeContractId(const nsACString& aContractId,
                                             PRBool aIsThreadsafe)
{
  NS_ASSERTION(!aContractId.IsEmpty(), "Empty contract id!");

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

#ifdef DEBUG
  {
    PRBool isThreadsafe;
    if (mThreadsafeContractIDs.Get(aContractId, &isThreadsafe)) {
      NS_ASSERTION(aIsThreadsafe == isThreadsafe, "Inconsistent threadsafety!");
    }
  }
#endif

  if (!mThreadsafeContractIDs.Put(aContractId, aIsThreadsafe)) {
    NS_WARNING("Out of memory!");
  }
}

ThreadsafeStatus
nsDOMThreadService::GetContractIdThreadsafeStatus(const nsACString& aContractId)
{
  NS_ASSERTION(!aContractId.IsEmpty(), "Empty contract id!");

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  PRBool isThreadsafe;
  if (mThreadsafeContractIDs.Get(aContractId, &isThreadsafe)) {
    return isThreadsafe ? Threadsafe : NotThreadsafe;
  }

  return Unknown;
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

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

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
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      mJSContexts.RemoveElement(cx);
    }

    JSContext* pushedCx;
    gThreadJSContextStack->Pop(&pushedCx);
    NS_ASSERTION(pushedCx == cx, "Popped the wrong context!");

    gThreadJSContextStack->SetSafeJSContext(nsnull);

    
    
    
    nsCOMPtr<nsIRunnable> runnable = new nsDestroyJSContextRunnable(cx);

    if (NS_FAILED(NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL))) {
      NS_WARNING("Failed to dispatch release runnable!");
    }
  }

  return NS_OK;
}

nsresult
nsDOMThreadService::RegisterWorker(nsDOMWorker* aWorker,
                                   nsIScriptGlobalObject* aGlobalObject)
{
  NS_ASSERTION(aWorker, "Null pointer!");

  if (aGlobalObject && NS_IsMainThread()) {
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
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    if (!mThreadPool) {
      
      return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;
    }

    mPools.Get(aGlobalObject, getter_AddRefs(pool));
  }

  nsresult rv;

  if (!pool) {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

    if (!mNavigatorStringsLoaded) {
      rv = NS_GetNavigatorAppName(mAppName);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = NS_GetNavigatorAppVersion(mAppVersion);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = NS_GetNavigatorPlatform(mPlatform);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = NS_GetNavigatorUserAgent(mUserAgent);
      NS_ENSURE_SUCCESS(rv, rv);

      mNavigatorStringsLoaded = PR_TRUE;
    }

    nsCOMPtr<nsIDocument> document;
    if (aGlobalObject) {
      nsCOMPtr<nsPIDOMWindow> domWindow(do_QueryInterface(aGlobalObject));
      NS_ENSURE_TRUE(domWindow, NS_ERROR_NO_INTERFACE);

      nsIDOMDocument* domDocument = domWindow->GetExtantDocument();
      NS_ENSURE_STATE(domDocument);

      document = do_QueryInterface(domDocument);
      NS_ENSURE_STATE(document);
    }

    pool = new nsDOMWorkerPool(aGlobalObject, document);
    NS_ENSURE_TRUE(pool, NS_ERROR_OUT_OF_MEMORY);

    rv = pool->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

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

void
nsDOMThreadService::RegisterPrefCallbacks()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  for (PRUint32 index = 0; index < NS_ARRAY_LENGTH(sPrefsToWatch); index++) {
    Preferences::RegisterCallback(PrefCallback, sPrefsToWatch[index]);
    PrefCallback(sPrefsToWatch[index], nsnull);
  }
}

void
nsDOMThreadService::UnregisterPrefCallbacks()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  for (PRUint32 index = 0; index < NS_ARRAY_LENGTH(sPrefsToWatch); index++) {
    Preferences::UnregisterCallback(PrefCallback, sPrefsToWatch[index]);
  }
}


int
nsDOMThreadService::PrefCallback(const char* aPrefName,
                                 void* aClosure)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  if(!strcmp(aPrefName, "dom.max_script_run_time")) {
    
    
    
    PRUint32 timeoutMS =
      Preferences::GetUint(aPrefName, gWorkerCloseHandlerTimeoutMS);

    
    
    if (timeoutMS) {
      gWorkerCloseHandlerTimeoutMS = timeoutMS;
    }
  }
  return 0;
}


PRUint32
nsDOMThreadService::GetWorkerCloseHandlerTimeoutMS()
{
  return gWorkerCloseHandlerTimeoutMS;
}
