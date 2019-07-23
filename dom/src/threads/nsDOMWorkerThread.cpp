






































#include "nsDOMWorkerThread.h"


#include "nsIDOMClassInfo.h"
#include "nsIJSContextStack.h"
#include "nsIJSRuntimeService.h"
#include "nsIScriptContext.h"
#include "nsIXPConnect.h"


#ifdef MOZ_SHARK
#include "jsdbgapi.h"
#endif
#include "nsAutoLock.h"
#include "nsContentUtils.h"
#include "nsJSUtils.h"
#include "nsJSEnvironment.h"


#include "nsDOMWorkerPool.h"
#include "nsDOMThreadService.h"
#include "nsDOMWorkerTimeout.h"

#define LOG(_args) PR_LOG(gDOMThreadsLog, PR_LOG_DEBUG, _args)


class nsDOMWorkerFunctions
{
public:
  
  static JSBool Dump(JSContext* aCx, JSObject* aObj, uintN aArgc, jsval* aArgv,
                     jsval* aRval);

  
  static JSBool DebugDump(JSContext* aCx, JSObject* aObj, uintN aArgc,
                          jsval* aArgv, jsval* aRval);

  
  static JSBool PostMessage(JSContext* aCx, JSObject* aObj, uintN aArgc,
                            jsval* aArgv, jsval* aRval);

  
  static JSBool SetTimeout(JSContext* aCx, JSObject* aObj, uintN aArgc,
                           jsval* aArgv, jsval* aRval) {
    return MakeTimeout(aCx, aObj, aArgc, aArgv, aRval, PR_FALSE);
  }

  
  static JSBool SetInterval(JSContext* aCx, JSObject* aObj, uintN aArgc,
                            jsval* aArgv, jsval* aRval) {
    return MakeTimeout(aCx, aObj, aArgc, aArgv, aRval, PR_TRUE);
  }

  
  static JSBool KillTimeout(JSContext* aCx, JSObject* aObj, uintN aArgc,
                            jsval* aArgv, jsval* aRval);

private:
  
  static JSBool MakeTimeout(JSContext* aCx, JSObject* aObj, uintN aArgc,
                            jsval* aArgv, jsval* aRval, PRBool aIsInterval);
};

JSBool JS_DLL_CALLBACK
nsDOMWorkerFunctions::Dump(JSContext* aCx,
                           JSObject* ,
                           uintN aArgc,
                           jsval* aArgv,
                           jsval* )
{
  

  JSString* str;
  if (aArgc && (str = JS_ValueToString(aCx, aArgv[0])) && str) {
    nsDependentJSString string(str);
    fputs(NS_ConvertUTF16toUTF8(nsDependentJSString(str)).get(), stderr);
    fflush(stderr);
  }
  return JS_TRUE;
}

JSBool JS_DLL_CALLBACK
nsDOMWorkerFunctions::DebugDump(JSContext* aCx,
                                JSObject* aObj,
                                uintN aArgc,
                                jsval* aArgv,
                                jsval* aRval)
{
#ifdef DEBUG
  return nsDOMWorkerFunctions::Dump(aCx, aObj, aArgc, aArgv, aRval);
#else
  return JS_TRUE;
#endif
}

JSBool JS_DLL_CALLBACK
nsDOMWorkerFunctions::PostMessage(JSContext* aCx,
                                  JSObject* ,
                                  uintN aArgc,
                                  jsval* aArgv,
                                  jsval* )
{
  nsDOMWorkerThread* worker =
    static_cast<nsDOMWorkerThread*>(JS_GetContextPrivate(aCx));
  NS_ASSERTION(worker, "This should be set by the DOM thread service!");

  if (worker->IsCanceled()) {
    return JS_FALSE;
  }

  nsRefPtr<nsDOMWorkerPool> pool = worker->Pool();
  NS_ASSERTION(pool, "Shouldn't ever be null!");

  nsresult rv;

  JSString* str;
  if (aArgc && (str = JS_ValueToString(aCx, aArgv[0])) && str) {
    rv = pool->PostMessageInternal(nsDependentJSString(str), worker);
  }
  else {
    rv = pool->PostMessageInternal(EmptyString(), worker);
  }
  NS_ENSURE_SUCCESS(rv, JS_FALSE);

  return JS_TRUE;
}

JSBool JS_DLL_CALLBACK
nsDOMWorkerFunctions::MakeTimeout(JSContext* aCx,
                                  JSObject* ,
                                  uintN aArgc,
                                  jsval* aArgv,
                                  jsval* aRval,
                                  PRBool aIsInterval)
{
  nsDOMWorkerThread* worker =
    static_cast<nsDOMWorkerThread*>(JS_GetContextPrivate(aCx));
  NS_ASSERTION(worker, "This should be set by the DOM thread service!");

  if (worker->IsCanceled()) {
    return JS_FALSE;
  }

  PRUint32 id = ++worker->mNextTimeoutId;

  nsAutoPtr<nsDOMWorkerTimeout>
    timeout(new nsDOMWorkerTimeout(worker, id));
  NS_ENSURE_TRUE(timeout, JS_FALSE);

  nsresult rv = timeout->Init(aCx, aArgc, aArgv, aIsInterval);
  NS_ENSURE_SUCCESS(rv, JS_FALSE);

  timeout.forget();

  *aRval = INT_TO_JSVAL(id);
  return JS_TRUE;
}

JSBool JS_DLL_CALLBACK
nsDOMWorkerFunctions::KillTimeout(JSContext* aCx,
                                  JSObject* ,
                                  uintN aArgc,
                                  jsval* aArgv,
                                  jsval* )
{
  nsDOMWorkerThread* worker =
    static_cast<nsDOMWorkerThread*>(JS_GetContextPrivate(aCx));
  NS_ASSERTION(worker, "This should be set by the DOM thread service!");

  
  if (worker->IsCanceled()) {
    return JS_TRUE;
  }

  if (!aArgc) {
    JS_ReportError(aCx, "Function requires at least 1 parameter");
    return JS_FALSE;
  }

  uint32 id;
  if (!JS_ValueToECMAUint32(aCx, aArgv[0], &id)) {
    JS_ReportError(aCx, "First argument must be a timeout id");
    return JS_FALSE;
  }

  worker->CancelTimeout(PRUint32(id));
  return JS_TRUE;
}

JSFunctionSpec gDOMWorkerFunctions[] = {
  { "dump",                  nsDOMWorkerFunctions::Dump,              1, 0, 0 },
  { "debug",                 nsDOMWorkerFunctions::DebugDump,         1, 0, 0 },
  { "postMessageToPool",     nsDOMWorkerFunctions::PostMessage,       1, 0, 0 },
  { "setTimeout",            nsDOMWorkerFunctions::SetTimeout,        1, 0, 0 },
  { "clearTimeout",          nsDOMWorkerFunctions::KillTimeout,       1, 0, 0 },
  { "setInterval",           nsDOMWorkerFunctions::SetInterval,       1, 0, 0 },
  { "clearInterval",         nsDOMWorkerFunctions::KillTimeout,       1, 0, 0 },
#ifdef MOZ_SHARK
  { "startShark",            js_StartShark,                           0, 0, 0 },
  { "stopShark",             js_StopShark,                            0, 0, 0 },
  { "connectShark",          js_ConnectShark,                         0, 0, 0 },
  { "disconnectShark",       js_DisconnectShark,                      0, 0, 0 },
#endif
  { nsnull,                  nsnull,                                  0, 0, 0 }
};





class nsDOMWorkerThreadWeakRef : public nsIDOMWorkerThread,
                                 public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_NSIDOMWORKERTHREAD(mWorker->)
  NS_FORWARD_NSICLASSINFO(mWorker->)

  nsDOMWorkerThreadWeakRef(nsDOMWorkerThread* aWorker)
  : mWorker(aWorker) { }

protected:
  nsDOMWorkerThread* mWorker;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(nsDOMWorkerThreadWeakRef, nsIDOMWorkerThread,
                                                        nsIClassInfo)




class nsDOMWorkerThreadContext : public nsIDOMWorkerThreadContext,
                                 public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMWORKERTHREADCONTEXT
  NS_DECL_NSICLASSINFO

  nsDOMWorkerThreadContext(nsDOMWorkerThread* aWorker)
  : mWorker(aWorker) { }

protected:
  nsDOMWorkerThread* mWorker;
  nsCOMPtr<nsIDOMWorkerThread> mWeakRef;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(nsDOMWorkerThreadContext,
                              nsIDOMWorkerThreadContext,
                              nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER1(nsDOMWorkerThreadContext,
                             nsIDOMWorkerThreadContext)

NS_IMPL_THREADSAFE_DOM_CI(nsDOMWorkerThreadContext)

NS_IMETHODIMP
nsDOMWorkerThreadContext::GetThisThread(nsIDOMWorkerThread** aThisThread)
{
  if (!mWeakRef) {
    mWeakRef = new nsDOMWorkerThreadWeakRef(mWorker);
    NS_ENSURE_TRUE(mWeakRef, NS_ERROR_OUT_OF_MEMORY);
  }

  NS_ADDREF(*aThisThread = mWeakRef);
  return NS_OK;
}

nsDOMWorkerThread::nsDOMWorkerThread(nsDOMWorkerPool* aPool,
                                     const nsAString& aSource)
: mPool(aPool),
  mSource(aSource),
  mGlobal(nsnull),
  mCompiled(PR_FALSE),
  mCallbackCount(0),
  mNextTimeoutId(0),
  mLock(nsnull)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aSource.IsEmpty(), "Empty source string!");

  PR_INIT_CLIST(&mTimeouts);
}

nsDOMWorkerThread::~nsDOMWorkerThread()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!IsCanceled()) {
    nsRefPtr<nsDOMWorkerPool> pool = Pool();
    pool->NoteDyingWorker(this);
  }

  ClearTimeouts();

  
  if (mGlobal) {
    JSRuntime* rt;
    if (NS_SUCCEEDED(nsDOMThreadService::JSRuntimeService()->GetRuntime(&rt))) {
      JS_RemoveRootRT(rt, &mGlobal);
    }
    else {
      NS_ERROR("This shouldn't fail!");
    }
  }

  if (mLock) {
    nsAutoLock::DestroyLock(mLock);
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsDOMWorkerThread, nsIDOMWorkerThread,
                                                 nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER1(nsDOMWorkerThread, nsIDOMWorkerThread)

NS_IMPL_THREADSAFE_DOM_CI(nsDOMWorkerThread)

nsresult
nsDOMWorkerThread::Init()
{
  mLock = nsAutoLock::NewLock("nsDOMWorkerThread::mLock");
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

  NS_ASSERTION(!mGlobal, "Already got a global?!");

  
  
  
  nsCOMPtr<nsIRunnable> runnable(new nsRunnable());
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = nsDOMThreadService::get()->Dispatch(this, runnable);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
nsDOMWorkerThread::HandleMessage(const nsAString& aMessage,
                                 nsDOMWorkerBase* aSource)
{
  nsCOMPtr<nsIDOMWorkerMessageListener> messageListener = GetMessageListener();
  if (!messageListener) {
    LOG(("Message received on a worker with no listener!"));
    return NS_OK;
  }

  
  
  

  JSContext* cx;
  nsresult rv =
    nsDOMThreadService::ThreadJSContextStack()->GetSafeJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest ar(cx);

  if (JS_IsExceptionPending(cx)) {
    JS_ClearPendingException(cx);
  }

  
  JSString* message = JS_NewUCStringCopyN(cx, (jschar*)aMessage.BeginReading(),
                                          aMessage.Length());
  NS_ENSURE_TRUE(message, NS_ERROR_FAILURE);

  
  jsval messageVal = STRING_TO_JSVAL(message);
  nsAutoGCRoot rootedMessage(&messageVal, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIXPConnect* xpc = nsContentUtils::XPConnect();

  nsCOMPtr<nsISupports> source;
  aSource->QueryInterface(NS_GET_IID(nsISupports), getter_AddRefs(source));
  NS_ASSERTION(source, "Impossible!");

  
  nsCOMPtr<nsIXPConnectJSObjectHolder> wrappedThread;
  rv = xpc->WrapNative(cx, mGlobal, source, NS_GET_IID(nsISupports),
                       getter_AddRefs(wrappedThread));
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject* sourceThread;
  rv = wrappedThread->GetJSObject(&sourceThread);
  NS_ENSURE_SUCCESS(rv, rv);

  
  jsval argv[2] = {
    STRING_TO_JSVAL(message),
    OBJECT_TO_JSVAL(sourceThread)
  };

  
  nsCOMPtr<nsIXPConnectJSObjectHolder> wrappedListener =
    do_QueryInterface(messageListener);
  NS_ENSURE_TRUE(wrappedListener, NS_ERROR_NO_INTERFACE);

  JSObject* listener;
  rv = wrappedListener->GetJSObject(&listener);
  NS_ENSURE_SUCCESS(rv, rv);

  
  jsval rval;
  PRBool success = JS_CallFunctionValue(cx, mGlobal, OBJECT_TO_JSVAL(listener),
                                        2, argv, &rval);
  if (!success) {
    
    JS_ReportPendingException(cx);
  }

  
  
  NS_ASSERTION(!JS_IsExceptionPending(cx), "Huh?!");

  return NS_OK;
}


nsresult
nsDOMWorkerThread::DispatchMessage(nsIRunnable* aRunnable)
{
  nsresult rv = nsDOMThreadService::get()->Dispatch(this, aRunnable);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
nsDOMWorkerThread::Cancel()
{
  nsDOMWorkerBase::Cancel();

  
  
  if (!IsSuspended()) {
    nsDOMThreadService::get()->WaitForCanceledWorker(this);
  }

  ClearTimeouts();
}

void
nsDOMWorkerThread::Suspend()
{
  nsDOMWorkerBase::Suspend();
  SuspendTimeouts();
}

void
nsDOMWorkerThread::Resume()
{
  nsDOMWorkerBase::Resume();
  ResumeTimeouts();
}

PRBool
nsDOMWorkerThread::SetGlobalForContext(JSContext* aCx)
{
  PRBool success = CompileGlobalObject(aCx);
  NS_ENSURE_TRUE(success, PR_FALSE);

  JS_SetGlobalObject(aCx, mGlobal);
  return PR_TRUE;
}

PRBool
nsDOMWorkerThread::CompileGlobalObject(JSContext* aCx)
{
  if (mGlobal) {
    return PR_TRUE;
  }

  if (mCompiled) {
    
    return PR_FALSE;
  }

  mCompiled = PR_TRUE;

  JSAutoRequest ar(aCx);

  JSObject* global = JS_NewObject(aCx, nsnull, nsnull, nsnull);
  NS_ENSURE_TRUE(global, PR_FALSE);

  NS_ASSERTION(!JS_GetGlobalObject(aCx), "Global object should be unset!");

  
  PRBool success = JS_InitStandardClasses(aCx, global);
  NS_ENSURE_TRUE(success, PR_FALSE);

  
  success = JS_DefineFunctions(aCx, global, gDOMWorkerFunctions);
  NS_ENSURE_TRUE(success, PR_FALSE);

  nsRefPtr<nsDOMWorkerThreadContext>
    context(new nsDOMWorkerThreadContext(this));
  NS_ENSURE_TRUE(context, NS_ERROR_OUT_OF_MEMORY);

  nsIXPConnect* xpc = nsContentUtils::XPConnect();
  nsresult rv = xpc->InitClasses(aCx, global);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  
  success = JS_DeleteProperty(aCx, global, "Components");
  NS_ENSURE_TRUE(success, PR_FALSE);

  nsCOMPtr<nsIXPConnectJSObjectHolder> contextWrapper;
  rv = xpc->WrapNative(aCx, global,
                       NS_ISUPPORTS_CAST(nsIDOMWorkerThreadContext*, context),
                       NS_GET_IID(nsIDOMWorkerThreadContext),
                       getter_AddRefs(contextWrapper));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  JSObject* contextObj;
  rv = contextWrapper->GetJSObject(&contextObj);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  
  success = JS_DefineProperty(aCx, global, "threadContext",
                              OBJECT_TO_JSVAL(contextObj), nsnull, nsnull,
                              JSPROP_ENUMERATE);
  NS_ENSURE_TRUE(success, PR_FALSE);

  JSScript* script = JS_CompileUCScript(aCx, global,
                                        reinterpret_cast<const jschar*>
                                            (mSource.BeginReading()),
                                        mSource.Length(), nsnull, 1);
  NS_ENSURE_TRUE(script, PR_FALSE);

  JSRuntime* rt;
  rv = nsDOMThreadService::JSRuntimeService()->GetRuntime(&rt);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  mGlobal = global;
  success = JS_AddNamedRootRT(rt, &mGlobal, "nsDOMWorkerThread Global Object");
  if (!success) {
    NS_WARNING("Failed to root global object for worker thread!");
    mGlobal = nsnull;
    return PR_FALSE;
  }

  
  jsval val;
  success = JS_ExecuteScript(aCx, global, script, &val);
  if (!success) {
    NS_WARNING("Failed to evaluate script for worker thread!");
    JS_RemoveRootRT(rt, &mGlobal);
    mGlobal = nsnull;
    return PR_FALSE;
  }

  
  nsCOMPtr<nsIDOMWorkerMessageListener> listener;
  if (JS_LookupProperty(aCx, global, "messageListener", &val) &&
      JSVAL_IS_OBJECT(val) &&
      NS_SUCCEEDED(xpc->WrapJS(aCx, JSVAL_TO_OBJECT(val),
                               NS_GET_IID(nsIDOMWorkerMessageListener),
                               getter_AddRefs(listener)))) {
    SetMessageListener(listener);
  }

  return PR_TRUE;
}

nsDOMWorkerTimeout*
nsDOMWorkerThread::FirstTimeout()
{
  
  PRCList* first = PR_LIST_HEAD(&mTimeouts);
  return first == &mTimeouts ?
                  nsnull :
                  static_cast<nsDOMWorkerTimeout*>(first);
}

nsDOMWorkerTimeout*
nsDOMWorkerThread::NextTimeout(nsDOMWorkerTimeout* aTimeout)
{
  
  nsDOMWorkerTimeout* next =
    static_cast<nsDOMWorkerTimeout*>(PR_NEXT_LINK(aTimeout));
  return next == &mTimeouts ? nsnull : next;
}

void
nsDOMWorkerThread::AddTimeout(nsDOMWorkerTimeout* aTimeout)
{
  
  
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aTimeout, "Null pointer!");

  PRIntervalTime newInterval = aTimeout->GetInterval();

  if (IsSuspended()) {
    aTimeout->Suspend(PR_Now());
  }

  nsAutoLock lock(mLock);

  
  
  
  for (nsDOMWorkerTimeout* timeout = FirstTimeout();
       timeout;
       timeout = NextTimeout(timeout)) {
    if (timeout->GetInterval() > newInterval) {
      PR_INSERT_BEFORE(aTimeout, timeout);
      return;
    }
  }

  PR_APPEND_LINK(aTimeout, &mTimeouts);
}

void
nsDOMWorkerThread::RemoveTimeout(nsDOMWorkerTimeout* aTimeout)
{
  nsAutoLock lock(mLock);

  PR_REMOVE_LINK(aTimeout);
}

void
nsDOMWorkerThread::ClearTimeouts()
{
  nsAutoTArray<nsRefPtr<nsDOMWorkerTimeout>, 20> timeouts;
  {
    nsAutoLock lock(mLock);
    for (nsDOMWorkerTimeout* timeout = FirstTimeout();
         timeout;
         timeout = NextTimeout(timeout)) {
      timeouts.AppendElement(timeout);
    }
  }

  PRUint32 count = timeouts.Length();
  for (PRUint32 i = 0; i < count; i++) {
    timeouts[i]->Cancel();
  }
}

void
nsDOMWorkerThread::CancelTimeout(PRUint32 aId)
{
  nsRefPtr<nsDOMWorkerTimeout> foundTimeout;
  {
    nsAutoLock lock(mLock);
    for (nsDOMWorkerTimeout* timeout = FirstTimeout();
         timeout;
         timeout = NextTimeout(timeout)) {
      if (timeout->GetId() == aId) {
        foundTimeout = timeout;
        break;
      }
    }
  }

  if (foundTimeout) {
    foundTimeout->Cancel();
  }
}

void
nsDOMWorkerThread::SuspendTimeouts()
{
  nsAutoTArray<nsRefPtr<nsDOMWorkerTimeout>, 20> timeouts;
  {
    nsAutoLock lock(mLock);
    for (nsDOMWorkerTimeout* timeout = FirstTimeout();
         timeout;
         timeout = NextTimeout(timeout)) {
      timeouts.AppendElement(timeout);
    }
  }

  PRTime now = PR_Now();

  PRUint32 count = timeouts.Length();
  for (PRUint32 i = 0; i < count; i++) {
    timeouts[i]->Suspend(now);
  }
}

void
nsDOMWorkerThread::ResumeTimeouts()
{
  nsAutoTArray<nsRefPtr<nsDOMWorkerTimeout>, 20> timeouts;
  {
    nsAutoLock lock(mLock);
    for (nsDOMWorkerTimeout* timeout = FirstTimeout();
         timeout;
         timeout = NextTimeout(timeout)) {
      NS_ASSERTION(timeout->IsSuspended(), "Should be suspended!");
      timeouts.AppendElement(timeout);
    }
  }

  PRTime now = PR_Now();

  PRUint32 count = timeouts.Length();
  for (PRUint32 i = 0; i < count; i++) {
    timeouts[i]->Resume(now);
  }
}

NS_IMETHODIMP
nsDOMWorkerThread::PostMessage(const nsAString& aMessage)
{
  nsresult rv = PostMessageInternal(aMessage);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}
