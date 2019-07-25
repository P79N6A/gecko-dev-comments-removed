





































#include "nsDOMWorkerXHRProxy.h"


#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMProgressEvent.h"
#include "nsILoadGroup.h"
#include "nsIRequest.h"
#include "nsIThread.h"
#include "nsIVariant.h"
#include "nsIXMLHttpRequest.h"


#include "nsComponentManagerUtils.h"
#include "nsIClassInfoImpl.h"
#include "nsThreadUtils.h"
#include "nsXMLHttpRequest.h"
#include "prinrval.h"
#include "prthread.h"


#include "nsDOMThreadService.h"
#include "nsDOMWorker.h"
#include "nsDOMWorkerEvents.h"
#include "nsDOMWorkerMacros.h"
#include "nsDOMWorkerPool.h"
#include "nsDOMWorkerXHR.h"
#include "nsDOMWorkerXHRProxiedFunctions.h"

using namespace mozilla;

#define MAX_XHR_LISTENER_TYPE nsDOMWorkerXHREventTarget::sMaxXHREventTypes
#define MAX_UPLOAD_LISTENER_TYPE nsDOMWorkerXHREventTarget::sMaxUploadEventTypes

#define RUN_PROXIED_FUNCTION(_name, _args)                                     \
  PR_BEGIN_MACRO                                                               \
    NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");                         \
                                                                               \
    if (mCanceled) {                                                           \
      return NS_ERROR_ABORT;                                                   \
    }                                                                          \
                                                                               \
    SyncEventQueue queue;                                                      \
                                                                               \
    nsRefPtr< :: _name> method = new :: _name _args;                           \
    NS_ENSURE_TRUE(method, NS_ERROR_OUT_OF_MEMORY);                            \
                                                                               \
    method->Init(this, &queue);                                                \
                                                                               \
    nsRefPtr<nsResultReturningRunnable> runnable =                             \
      new nsResultReturningRunnable(mMainThread, method, mWorkerXHR->mWorker); \
    NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);                          \
                                                                               \
    nsresult _rv = runnable->Dispatch();                                       \
                                                                               \
    if (mCanceled) {                                                           \
      return NS_ERROR_ABORT;                                                   \
    }                                                                          \
                                                                               \
    PRUint32 queueLength = queue.Length();                                     \
    for (PRUint32 index = 0; index < queueLength; index++) {                   \
      queue[index]->Run();                                                     \
    }                                                                          \
                                                                               \
    if (NS_FAILED(_rv)) {                                                      \
      return _rv;                                                              \
    }                                                                          \
  PR_END_MACRO

using namespace nsDOMWorkerProxiedXHRFunctions;

class nsResultReturningRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  nsResultReturningRunnable(nsIEventTarget* aTarget, nsIRunnable* aRunnable,
                            nsDOMWorker* aWorker)
  : mTarget(aTarget), mRunnable(aRunnable), mWorker(aWorker),
    mResult(NS_OK), mDone(PR_FALSE) { }

  nsresult Dispatch() {
    if (!mWorker) {
      
      return NS_ERROR_ABORT;
    }

    nsIThread* currentThread = NS_GetCurrentThread();
    NS_ASSERTION(currentThread, "This should never be null!");

    nsresult rv = mTarget->Dispatch(this, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    while (!mWorker->IsCanceled() && !mDone) {
      
      if (!NS_ProcessNextEvent(currentThread, PR_FALSE)) {
        PR_Sleep(PR_INTERVAL_NO_WAIT);
      }
    }

    if (mWorker->IsCanceled()) {
      mResult = NS_ERROR_ABORT;
    }

    return mResult;
  }

  NS_IMETHOD Run() {
#ifdef DEBUG
    PRBool rightThread = PR_FALSE;
    mTarget->IsOnCurrentThread(&rightThread);
    NS_ASSERTION(rightThread, "Run called on the wrong thread!");
#endif

    mResult = mWorker->IsCanceled() ? NS_ERROR_ABORT : mRunnable->Run();
    mDone = PR_TRUE;

    return mResult;
  }

private:
  nsCOMPtr<nsIEventTarget> mTarget;
  nsCOMPtr<nsIRunnable> mRunnable;
  nsRefPtr<nsDOMWorker> mWorker;
  nsresult mResult;
  volatile PRBool mDone;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsResultReturningRunnable, nsIRunnable)

class nsDOMWorkerXHRLastProgressOrLoadEvent : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  nsDOMWorkerXHRLastProgressOrLoadEvent(nsDOMWorkerXHRProxy* aProxy)
  : mProxy(aProxy) {
    NS_ASSERTION(aProxy, "Null pointer!");
  }

  NS_IMETHOD Run() {
    nsRefPtr<nsDOMWorkerXHREvent> lastProgressOrLoadEvent;

    if (!mProxy->mCanceled) {
      MutexAutoLock lock(mProxy->mWorkerXHR->GetLock());
      mProxy->mLastProgressOrLoadEvent.swap(lastProgressOrLoadEvent);
      if (mProxy->mCanceled) {
        return NS_ERROR_ABORT;
      }
    }

    if (lastProgressOrLoadEvent) {
      return lastProgressOrLoadEvent->Run();
    }

    return NS_OK;
  }

private:
  nsRefPtr<nsDOMWorkerXHRProxy> mProxy;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWorkerXHRLastProgressOrLoadEvent,
                              nsIRunnable)

class nsDOMWorkerXHRWrappedListener : public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS

  nsDOMWorkerXHRWrappedListener(nsIDOMEventListener* aInner)
  : mInner(aInner) {
    NS_ASSERTION(aInner, "Null pointer!");
  }

  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) {
    return mInner->HandleEvent(aEvent);
  }

  nsIDOMEventListener* Inner() {
    return mInner;
  }

private:
  nsCOMPtr<nsIDOMEventListener> mInner;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWorkerXHRWrappedListener,
                              nsIDOMEventListener)

class nsDOMWorkerXHRAttachUploadListenersRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  nsDOMWorkerXHRAttachUploadListenersRunnable(nsDOMWorkerXHRProxy* aProxy)
  : mProxy(aProxy) {
    NS_ASSERTION(aProxy, "Null pointer!");
  }

  NS_IMETHOD Run() {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

    NS_ASSERTION(!mProxy->mWantUploadListeners, "Huh?!");

    nsCOMPtr<nsIDOMEventTarget> upload(do_QueryInterface(mProxy->mUpload));
    NS_ASSERTION(upload, "This shouldn't fail!");

    nsAutoString eventName;
    for (PRUint32 index = 0; index < MAX_UPLOAD_LISTENER_TYPE; index++) {
      eventName.AssignASCII(nsDOMWorkerXHREventTarget::sListenerTypes[index]);
      upload->AddEventListener(eventName, mProxy, PR_FALSE);
    }

    mProxy->mWantUploadListeners = PR_TRUE;
    return NS_OK;
  }

private:
  nsRefPtr<nsDOMWorkerXHRProxy> mProxy;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWorkerXHRAttachUploadListenersRunnable,
                              nsIRunnable)

class nsDOMWorkerXHRFinishSyncXHRRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  nsDOMWorkerXHRFinishSyncXHRRunnable(nsDOMWorkerXHRProxy* aProxy,
                                      nsIThread* aTarget)
  : mProxy(aProxy), mTarget(aTarget) {
    NS_ASSERTION(aProxy, "Null pointer!");
    NS_ASSERTION(aTarget, "Null pointer!");
  }

  NS_IMETHOD Run() {
    nsCOMPtr<nsIThread> thread;
    mProxy->mSyncXHRThread.swap(thread);
    mProxy = nsnull;

    NS_ASSERTION(thread, "Should have a thread here!");
    NS_ASSERTION(!NS_IsMainThread() && thread == NS_GetCurrentThread(),
                 "Wrong thread?!");

    return NS_ProcessPendingEvents(thread);
  }

  nsresult Dispatch() {
    nsresult rv = mTarget->Dispatch(this, NS_DISPATCH_NORMAL);
    mTarget = nsnull;
    return rv;
  }

private:
  nsRefPtr<nsDOMWorkerXHRProxy> mProxy;
  nsCOMPtr<nsIThread> mTarget;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWorkerXHRFinishSyncXHRRunnable, nsIRunnable)

nsDOMWorkerXHRProxy::nsDOMWorkerXHRProxy(nsDOMWorkerXHR* aWorkerXHR)
: mWorkerXHR(aWorkerXHR),
  mXHR(nsnull),
  mConcreteXHR(nsnull),
  mUpload(nsnull),
  mSyncEventQueue(nsnull),
  mChannelID(-1),
  mOwnedByXHR(PR_FALSE),
  mWantUploadListeners(PR_FALSE),
  mCanceled(PR_FALSE),
  mSyncRequest(PR_FALSE)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(MAX_XHR_LISTENER_TYPE >= MAX_UPLOAD_LISTENER_TYPE,
               "Upload should support a subset of XHR's event types!");
}

nsDOMWorkerXHRProxy::~nsDOMWorkerXHRProxy()
{
  if (mOwnedByXHR) {
    mWorkerXHRWN = nsnull;
  }
  NS_ASSERTION(!mXHR, "Destroy not called!");
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsDOMWorkerXHRProxy, nsIRunnable,
                                                   nsIDOMEventListener,
                                                   nsIRequestObserver)

nsresult
nsDOMWorkerXHRProxy::Init()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_FALSE(mXHR, NS_ERROR_ALREADY_INITIALIZED);

  mMainThread = do_GetMainThread();
  NS_ENSURE_TRUE(mMainThread, NS_ERROR_UNEXPECTED);

  nsRefPtr<nsResultReturningRunnable> runnable =
    new nsResultReturningRunnable(mMainThread, this, mWorkerXHR->mWorker);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = runnable->Dispatch();
  if (NS_FAILED(rv)) {
    
    NS_WARN_IF_FALSE(rv == NS_ERROR_ABORT, "Dispatch failed!");

    if (mXHR) {
      
#ifdef DEBUG
      nsresult rvDebug =
#endif
      mMainThread->Dispatch(this, NS_DISPATCH_NORMAL);
      NS_ASSERTION(NS_SUCCEEDED(rvDebug), "Going to leak!");
    }

    return rv;
  }

  return NS_OK;
}

nsIXMLHttpRequest*
nsDOMWorkerXHRProxy::GetXMLHttpRequest()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  return mCanceled ? nsnull : mXHR;
}

nsresult
nsDOMWorkerXHRProxy::Destroy()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  {
    MutexAutoLock lock(mWorkerXHR->GetLock());

    mCanceled = PR_TRUE;

    mLastProgressOrLoadEvent = nsnull;
    mLastXHRState = nsnull;
  }

  DestroyInternal();

  NS_ASSERTION(!(mLastProgressOrLoadEvent || mLastXHRState), "Going to leak!");

  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::InitInternal()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!mXHR, "InitInternal shouldn't be called twice!");

  nsDOMWorker* worker = mWorkerXHR->mWorker;
  if (worker->IsCanceled()) {
    return NS_ERROR_ABORT;
  }

  NS_ASSERTION(worker->GetPrincipal(), "Must have a principal!");
  NS_ASSERTION(worker->GetBaseURI(), "Must have a URI!");

  nsIScriptGlobalObject* sgo = worker->Pool()->ScriptGlobalObject();
  nsIScriptContext* scriptContext = sgo ? sgo->GetContext() : nsnull;

  nsCOMPtr<nsPIDOMWindow> ownerWindow = do_QueryInterface(sgo);

  nsRefPtr<nsXMLHttpRequest> xhrConcrete = new nsXMLHttpRequest();
  NS_ENSURE_TRUE(xhrConcrete, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = xhrConcrete->Init(worker->GetPrincipal(), scriptContext,
                                  ownerWindow, worker->GetBaseURI());
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIXMLHttpRequest> xhr =
    do_QueryInterface(static_cast<nsIXMLHttpRequest*>(xhrConcrete));
  NS_ENSURE_TRUE(xhr, NS_ERROR_NO_INTERFACE);

  nsCOMPtr<nsIXMLHttpRequestUpload> upload;
  rv = xhr->GetUpload(getter_AddRefs(upload));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsDOMWorkerXHRState> nullState = new nsDOMWorkerXHRState();
  NS_ENSURE_TRUE(nullState, NS_ERROR_OUT_OF_MEMORY);

  nsDOMWorkerXHREvent::SnapshotXHRState(xhr, nullState);
  mLastXHRState.swap(nullState);

  xhrConcrete->SetRequestObserver(this);

  
  xhr.swap(mXHR);

  
  mUpload = upload;
  mConcreteXHR = xhrConcrete;

  AddRemoveXHRListeners(PR_TRUE);

  return NS_OK;
}

void
nsDOMWorkerXHRProxy::DestroyInternal()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<nsDOMWorkerXHRProxy> kungFuDeathGrip(this);

  if (mConcreteXHR) {
    mConcreteXHR->SetRequestObserver(nsnull);
  }

  if (mOwnedByXHR) {
    mXHR->Abort();
  }
  else {
    
    
    nsRefPtr<nsDOMWorkerXHRFinishSyncXHRRunnable> syncFinishedRunnable;
    {
      MutexAutoLock lock(mWorkerXHR->GetLock());
      mSyncFinishedRunnable.swap(syncFinishedRunnable);
    }

    if (syncFinishedRunnable) {
      syncFinishedRunnable->Dispatch();
    }
  }

  NS_ASSERTION(!mOwnedByXHR, "Should have flipped already!");
  NS_ASSERTION(!mSyncFinishedRunnable, "Should have fired this already!");

  
  if (mXHR) {
    AddRemoveXHRListeners(PR_FALSE);

    mXHR->Release();
    mXHR = nsnull;

    mUpload = nsnull;
  }
}

void
nsDOMWorkerXHRProxy::AddRemoveXHRListeners(PRBool aAdd)
{
  nsCOMPtr<nsIDOMEventTarget> xhrTarget(do_QueryInterface(mXHR));
  NS_ASSERTION(xhrTarget, "This shouldn't fail!");

  nsAutoString eventName;
  PRUint32 index = 0;

  if (mWantUploadListeners) {
    nsCOMPtr<nsIDOMEventTarget> uploadTarget(do_QueryInterface(mUpload));
    NS_ASSERTION(uploadTarget, "This shouldn't fail!");

    for (; index < MAX_UPLOAD_LISTENER_TYPE; index++) {
      eventName.AssignASCII(nsDOMWorkerXHREventTarget::sListenerTypes[index]);
      if (aAdd) {
        xhrTarget->AddEventListener(eventName, this, PR_FALSE);
        uploadTarget->AddEventListener(eventName, this, PR_FALSE);
      }
      else {
        xhrTarget->RemoveEventListener(eventName, this, PR_FALSE);
        uploadTarget->RemoveEventListener(eventName, this, PR_FALSE);
      }
    }
  }

  for (; index < MAX_XHR_LISTENER_TYPE; index++) {
    eventName.AssignASCII(nsDOMWorkerXHREventTarget::sListenerTypes[index]);
    if (aAdd) {
      xhrTarget->AddEventListener(eventName, this, PR_FALSE);
    }
    else {
      xhrTarget->RemoveEventListener(eventName, this, PR_FALSE);
    }
  }
}

void
nsDOMWorkerXHRProxy::FlipOwnership()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
  mOwnedByXHR = !mOwnedByXHR;

  
  
  nsRefPtr<nsDOMWorkerXHRProxy> kungFuDeathGrip(this);

  if (mOwnedByXHR) {
    mWorkerXHRWN = mWorkerXHR->GetWrappedNative();
    NS_ASSERTION(mWorkerXHRWN, "Null pointer!");
    mXHR->Release();
  }
  else {
    mXHR->AddRef();
    mWorkerXHRWN = nsnull;
  }
}

nsresult
nsDOMWorkerXHRProxy::UploadEventListenerAdded()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  
  
  
  if (mWantUploadListeners) {
    return NS_OK;
  }

  nsRefPtr<nsDOMWorkerXHRAttachUploadListenersRunnable> attachRunnable =
    new nsDOMWorkerXHRAttachUploadListenersRunnable(this);
  NS_ENSURE_TRUE(attachRunnable, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<nsResultReturningRunnable> runnable =
    new nsResultReturningRunnable(mMainThread, attachRunnable,
                                  mWorkerXHR->mWorker);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = runnable->Dispatch();
  if (NS_FAILED(rv)) {
    return rv;
  }

  NS_ASSERTION(mWantUploadListeners, "Should have set this!");
  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::HandleWorkerEvent(nsDOMWorkerXHREvent* aEvent,
                                       PRBool aUploadEvent)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aEvent, "Should not be null!");

  {
    MutexAutoLock lock(mWorkerXHR->GetLock());

    if (mCanceled ||
        (aEvent->mChannelID != -1 && aEvent->mChannelID != mChannelID)) {
      return NS_OK;
    }

    mLastXHRState = aEvent->ForgetState();
  }

#ifdef DEBUG
  if (aUploadEvent) {
    NS_ASSERTION(mWorkerXHR->mUpload, "No upload object!");
  }
#endif

  PRUint32& type = aEvent->mXHREventType;
  if (type == LISTENER_TYPE_ABORT || type == LISTENER_TYPE_ERROR ||
      type == LISTENER_TYPE_LOAD) {
    nsAutoPtr<ProgressInfo>& progressInfo = aUploadEvent ?
                                            mUploadProgressInfo :
                                            mDownloadProgressInfo;
    progressInfo = nsnull;

    
    MutexAutoLock lock(mWorkerXHR->GetLock());
  }

  nsIDOMEventTarget* target = aUploadEvent ?
    static_cast<nsDOMWorkerMessageHandler*>(mWorkerXHR->mUpload) :
    static_cast<nsDOMWorkerMessageHandler*>(mWorkerXHR);

  return target->DispatchEvent(static_cast<nsDOMWorkerEvent*>(aEvent), nsnull);
}

PRBool
nsDOMWorkerXHRProxy::IsUploadEvent(nsIDOMEvent* aEvent)
{
  NS_ASSERTION(aEvent, "Null pointer!");

  nsCOMPtr<nsIDOMEventTarget> target;
  if (NS_SUCCEEDED(aEvent->GetTarget(getter_AddRefs(target)))) {
    nsCOMPtr<nsIXMLHttpRequestUpload> upload(do_QueryInterface(target));
    if (upload) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsresult
nsDOMWorkerXHRProxy::DispatchPrematureAbortEvents(PRUint32 aType,
                                                  nsIDOMEventTarget* aTarget,
                                                  ProgressInfo* aProgressInfo)
{
  nsAutoString type;
  type.AssignASCII(nsDOMWorkerXHREventTarget::sListenerTypes[aType]);

  nsresult rv;

  nsRefPtr<nsDOMWorkerEvent> event;
  if (aProgressInfo) {
    nsRefPtr<nsDOMWorkerProgressEvent> progressEvent =
      new nsDOMWorkerProgressEvent();
    NS_ENSURE_TRUE(progressEvent, NS_ERROR_OUT_OF_MEMORY);

    rv = progressEvent->InitProgressEvent(type, PR_FALSE, PR_FALSE,
                                          aProgressInfo->computable,
                                          aProgressInfo->loaded,
                                          aProgressInfo->total);
    NS_ENSURE_SUCCESS(rv, rv);

    event = progressEvent;
  }
  else {
    event = new nsDOMWorkerEvent();
    NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

    rv = event->InitEvent(type, PR_FALSE, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  event->SetTarget(aTarget);

  nsRefPtr<nsDOMWorkerXHREvent> xhrEvent = new nsDOMWorkerXHREvent(this);
  NS_ENSURE_TRUE(xhrEvent, NS_ERROR_OUT_OF_MEMORY);

  rv = xhrEvent->Init(aType, type, event, nsDOMWorkerXHREvent::NO_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  nsDOMWorkerXHRState* state = xhrEvent->GetState();
  NS_ASSERTION(state, "Should never be null if Init succeeded!");

  NS_ASSERTION(state->responseText.IsEmpty(), "Should be empty!");
  state->readyState = 4; 
  mXHR->GetStatusText(state->statusText);
  mXHR->GetStatus(&state->status);

  return HandleEventRunnable(xhrEvent);
}

nsresult
nsDOMWorkerXHRProxy::MaybeDispatchPrematureAbortEvents(PRBool aFromOpen)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsresult rv;
  nsIDOMEventTarget* target;

  if (mDownloadProgressInfo) {
    target = static_cast<nsDOMWorkerMessageHandler*>(mWorkerXHR);
    NS_ASSERTION(target, "Must have target here!");

    rv = DispatchPrematureAbortEvents(LISTENER_TYPE_READYSTATECHANGE, target,
                                      nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    if (aFromOpen) {
      rv = DispatchPrematureAbortEvents(LISTENER_TYPE_ABORT, target,
                                        mDownloadProgressInfo);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (mUploadProgressInfo) {
    target = static_cast<nsDOMWorkerMessageHandler*>(mWorkerXHR->mUpload);
    NS_ASSERTION(target, "Must have upload here!");

    rv = DispatchPrematureAbortEvents(LISTENER_TYPE_ABORT, target,
                                      mUploadProgressInfo);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsDOMWorkerXHRProxy::HandleEvent(nsIDOMEvent* aEvent)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aEvent);

  nsAutoString typeString;
  nsresult rv = aEvent->GetType(typeString);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 type =
    nsDOMWorkerXHREventTarget::GetListenerTypeFromString(typeString);

  PRBool isUpload = IsUploadEvent(aEvent);

  if ((isUpload && type >= MAX_UPLOAD_LISTENER_TYPE) ||
      (!isUpload && type >= MAX_XHR_LISTENER_TYPE)) {
    NS_ERROR("We shouldn't ever get a strange event from main thread XHR!");
    return NS_OK;
  }

  
  
  nsRefPtr<nsDOMWorkerXHRFinishSyncXHRRunnable> syncFinishedRunnable;

  PRBool requestDone;
  if (type == LISTENER_TYPE_ABORT || type == LISTENER_TYPE_ERROR ||
      type == LISTENER_TYPE_LOAD) {
    requestDone = PR_TRUE;

    nsAutoPtr<ProgressInfo>& progressInfo = isUpload ?
                                            mUploadProgressInfo :
                                            mDownloadProgressInfo;
    if (!progressInfo) {
      progressInfo = new ProgressInfo();
      NS_WARN_IF_FALSE(progressInfo, "Out of memory!");
    }

    if (progressInfo) {
      nsCOMPtr<nsIDOMProgressEvent> progressEvent(do_QueryInterface(aEvent));
      NS_ASSERTION(progressEvent, "Should always QI to nsIDOMProgressEvent!");
      if (progressEvent) {
        progressEvent->GetLengthComputable(&progressInfo->computable);
        progressEvent->GetLoaded(&progressInfo->loaded);
        progressEvent->GetTotal(&progressInfo->total);
      }
    }

    NS_ASSERTION(!syncFinishedRunnable, "This shouldn't be set!");

    MutexAutoLock lock(mWorkerXHR->GetLock());
    mSyncFinishedRunnable.swap(syncFinishedRunnable);
  }
  else {
    requestDone = PR_FALSE;
  }

  if (mCanceled) {
    
    
    
    
    
    if (type == LISTENER_TYPE_ABORT) {
      OnStopRequest(nsnull, nsnull, NS_ERROR_ABORT);
    }

    
    return NS_ERROR_ABORT;
  }

  
  
  if (!requestDone &&
      !mWorkerXHR->HasListeners(typeString) &&
      !(isUpload && mWorkerXHR->mUpload->HasListeners(typeString))) {
    return NS_OK;
  }

  nsRefPtr<nsDOMWorkerXHREvent> newEvent = new nsDOMWorkerXHREvent(this);
  NS_ENSURE_TRUE(newEvent, NS_ERROR_OUT_OF_MEMORY);

  rv = newEvent->Init(type, typeString, aEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> runnable(newEvent);

  if (type == LISTENER_TYPE_LOAD || type == LISTENER_TYPE_PROGRESS) {
    runnable = new nsDOMWorkerXHRLastProgressOrLoadEvent(this);
    NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

    {
      MutexAutoLock lock(mWorkerXHR->GetLock());

      if (mCanceled) {
        return NS_ERROR_ABORT;
      }

      mLastProgressOrLoadEvent.swap(newEvent);

      if (newEvent) {
        
        
        return NS_OK;
      }
    }
  }

  rv = HandleEventRunnable(runnable);

  if (syncFinishedRunnable) {
    syncFinishedRunnable->Dispatch();
  }

  return rv;
}

nsresult
nsDOMWorkerXHRProxy::HandleEventRunnable(nsIRunnable* aRunnable)
{
  NS_ASSERTION(aRunnable, "Null pointer!");

  nsresult rv;

  if (mSyncEventQueue) {
    
    
    nsCOMPtr<nsIRunnable>* newElement =
      mSyncEventQueue->AppendElement(aRunnable);
    NS_ENSURE_TRUE(newElement, NS_ERROR_OUT_OF_MEMORY);
  }
  else if (mSyncXHRThread) {
    
    
    rv = mSyncXHRThread->Dispatch(aRunnable, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    rv = nsDOMThreadService::get()->Dispatch(mWorkerXHR->mWorker, aRunnable);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::Open(const nsACString& aMethod,
                          const nsACString& aUrl,
                          PRBool aAsync,
                          const nsAString& aUser,
                          const nsAString& aPassword)
{
  if (!NS_IsMainThread()) {
    mSyncRequest = !aAsync;

    
    RUN_PROXIED_FUNCTION(Open,
                         (aMethod, aUrl, PR_TRUE, aUser, aPassword));
    return NS_OK;
  }

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = MaybeDispatchPrematureAbortEvents(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mXHR->Open(aMethod, aUrl, aAsync, aUser, aPassword);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  mChannelID++;

  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::Abort()
{
  if (!NS_IsMainThread()) {
    RUN_PROXIED_FUNCTION(Abort, ());
    return NS_OK;
  }

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = MaybeDispatchPrematureAbortEvents(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mXHR->Abort();
  NS_ENSURE_SUCCESS(rv, rv);

  
  mChannelID++;

  return NS_OK;
}

nsDOMWorkerXHRProxy::SyncEventQueue*
nsDOMWorkerXHRProxy::SetSyncEventQueue(SyncEventQueue* aQueue)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  SyncEventQueue* oldQueue = mSyncEventQueue;
  mSyncEventQueue = aQueue;
  return oldQueue;
}

nsresult
nsDOMWorkerXHRProxy::GetAllResponseHeaders(char** _retval)
{
  RUN_PROXIED_FUNCTION(GetAllResponseHeaders, (_retval));
  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::GetResponseHeader(const nsACString& aHeader,
                                       nsACString& _retval)
{
  RUN_PROXIED_FUNCTION(GetResponseHeader, (aHeader, _retval));
  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::Send(nsIVariant* aBody)
{
  NS_ASSERTION(!mSyncXHRThread, "Shouldn't reenter here!");

  if (mSyncRequest) {

    mSyncXHRThread = NS_GetCurrentThread();
    NS_ENSURE_TRUE(mSyncXHRThread, NS_ERROR_FAILURE);

    MutexAutoLock lock(mWorkerXHR->GetLock());

    if (mCanceled) {
      return NS_ERROR_ABORT;
    }

    mSyncFinishedRunnable =
      new nsDOMWorkerXHRFinishSyncXHRRunnable(this, mSyncXHRThread);
    NS_ENSURE_TRUE(mSyncFinishedRunnable, NS_ERROR_FAILURE);
  }

  RUN_PROXIED_FUNCTION(Send, (aBody));

  return RunSyncEventLoop();
}

nsresult
nsDOMWorkerXHRProxy::SendAsBinary(const nsAString& aBody)
{
  NS_ASSERTION(!mSyncXHRThread, "Shouldn't reenter here!");

  if (mSyncRequest) {
    mSyncXHRThread = NS_GetCurrentThread();
    NS_ENSURE_TRUE(mSyncXHRThread, NS_ERROR_FAILURE);

    MutexAutoLock lock(mWorkerXHR->GetLock());

    if (mCanceled) {
      return NS_ERROR_ABORT;
    }

    mSyncFinishedRunnable =
      new nsDOMWorkerXHRFinishSyncXHRRunnable(this, mSyncXHRThread);
    NS_ENSURE_TRUE(mSyncFinishedRunnable, NS_ERROR_FAILURE);
  }

  RUN_PROXIED_FUNCTION(SendAsBinary, (aBody));

  return RunSyncEventLoop();
}

nsresult
nsDOMWorkerXHRProxy::SetRequestHeader(const nsACString& aHeader,
                                      const nsACString& aValue)
{
  RUN_PROXIED_FUNCTION(SetRequestHeader, (aHeader, aValue));
  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::OverrideMimeType(const nsACString& aMimetype)
{
  RUN_PROXIED_FUNCTION(OverrideMimeType, (aMimetype));
  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::GetMultipart(PRBool* aMultipart)
{
  NS_ASSERTION(aMultipart, "Null pointer!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  RUN_PROXIED_FUNCTION(GetMultipart, (aMultipart));
  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::SetMultipart(PRBool aMultipart)
{
  RUN_PROXIED_FUNCTION(SetMultipart, (aMultipart));
  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::GetWithCredentials(PRBool* aWithCredentials)
{
  RUN_PROXIED_FUNCTION(GetWithCredentials, (aWithCredentials));
  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::SetWithCredentials(PRBool aWithCredentials)
{
  RUN_PROXIED_FUNCTION(SetWithCredentials, (aWithCredentials));
  return NS_OK;
}

nsresult
nsDOMWorkerXHRProxy::GetResponseText(nsAString& _retval)
{
  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  if (NS_SUCCEEDED(mLastXHRState->responseTextResult)) {
    _retval.Assign(mLastXHRState->responseText);
  }
  return mLastXHRState->responseTextResult;
}

nsresult
nsDOMWorkerXHRProxy::GetStatusText(nsACString& _retval)
{
  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  if (NS_SUCCEEDED(mLastXHRState->statusTextResult)) {
    _retval.Assign(mLastXHRState->statusText);
  }
  return mLastXHRState->statusTextResult;
}

nsresult
nsDOMWorkerXHRProxy::GetStatus(nsresult* _retval)
{
  NS_ASSERTION(_retval, "Null pointer!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  if (NS_SUCCEEDED(mLastXHRState->statusResult)) {
    *_retval = mLastXHRState->status;
  }
  return mLastXHRState->statusResult;
}

nsresult
nsDOMWorkerXHRProxy::GetReadyState(PRUint16* _retval)
{
  NS_ASSERTION(_retval, "Null pointer!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  if (NS_SUCCEEDED(mLastXHRState->readyStateResult)) {
    *_retval = mLastXHRState->readyState;
  }
  return mLastXHRState->readyStateResult;
}

nsresult
nsDOMWorkerXHRProxy::RunSyncEventLoop()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (!mSyncRequest) {
    return NS_OK;
  }

  NS_ASSERTION(mSyncXHRThread == NS_GetCurrentThread(), "Wrong thread!");

  while (mSyncXHRThread) {
    if (NS_UNLIKELY(!NS_ProcessNextEvent(mSyncXHRThread))) {
      NS_ERROR("Something wrong here, this shouldn't fail!");
      return NS_ERROR_UNEXPECTED;
    }
  }

  NS_ASSERTION(!NS_HasPendingEvents(NS_GetCurrentThread()),
               "Unprocessed events remaining!");

  return NS_OK;
}


NS_IMETHODIMP
nsDOMWorkerXHRProxy::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsresult rv = NS_OK;

  if (!mXHR) {
    rv = InitInternal();
    if (NS_SUCCEEDED(rv)) {
      return NS_OK;
    }
    NS_WARNING("InitInternal failed!");
  }

  DestroyInternal();
  return rv;
}


NS_IMETHODIMP
nsDOMWorkerXHRProxy::OnStartRequest(nsIRequest* ,
                                    nsISupports* )
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ASSERTION(!mOwnedByXHR, "Inconsistent state!");

  if (mCanceled) {
    return NS_OK;
  }

  FlipOwnership();

  return NS_OK;
}


NS_IMETHODIMP
nsDOMWorkerXHRProxy::OnStopRequest(nsIRequest* ,
                                   nsISupports* ,
                                   nsresult )
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ASSERTION(mOwnedByXHR, "Inconsistent state!");

  FlipOwnership();

  return NS_OK;
}
