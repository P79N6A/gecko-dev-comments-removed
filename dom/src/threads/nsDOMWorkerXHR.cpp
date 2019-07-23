





































#include "nsDOMWorkerXHR.h"


#include "nsIDocument.h"
#include "nsIDOMEvent.h"
#include "nsIThread.h"
#include "nsIXPConnect.h"


#include "nsAutoLock.h"
#include "nsAXPCNativeCallContext.h"
#include "nsComponentManagerUtils.h"
#include "nsContentUtils.h"
#include "nsIClassInfoImpl.h"
#include "nsJSUtils.h"
#include "nsThreadUtils.h"


#include "nsDOMThreadService.h"
#include "nsDOMWorkerEvents.h"
#include "nsDOMWorkerPool.h"
#include "nsDOMWorkerXHRProxy.h"







const char* const nsDOMWorkerXHREventTarget::sListenerTypes[] = {
  
  "abort",                             
  "error",                             
  "load",                              
  "loadstart",                         
  "progress",                          

  
  "readystatechange"                   
};


const PRUint32 nsDOMWorkerXHREventTarget::sMaxXHREventTypes =
  NS_ARRAY_LENGTH(nsDOMWorkerXHREventTarget::sListenerTypes);



const PRUint32 nsDOMWorkerXHREventTarget::sMaxUploadEventTypes =
  LISTENER_TYPE_READYSTATECHANGE;



PR_STATIC_ASSERT(nsDOMWorkerXHREventTarget::sMaxXHREventTypes >=
                 nsDOMWorkerXHREventTarget::sMaxUploadEventTypes);

NS_IMPL_ISUPPORTS_INHERITED1(nsDOMWorkerXHREventTarget,
                             nsDOMWorkerMessageHandler,
                             nsIXMLHttpRequestEventTarget)

PRUint32
nsDOMWorkerXHREventTarget::GetListenerTypeFromString(const nsAString& aString)
{
  for (PRUint32 index = 0; index < sMaxXHREventTypes; index++) {
    if (aString.EqualsASCII(sListenerTypes[index])) {
      return index;
    }
  }
  return PR_UINT32_MAX;
}

NS_IMETHODIMP
nsDOMWorkerXHREventTarget::GetOnabort(nsIDOMEventListener** aOnabort)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER(aOnabort);

  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_ABORT]);

  nsCOMPtr<nsIDOMEventListener> listener = GetOnXListener(type);
  listener.forget(aOnabort);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHREventTarget::SetOnabort(nsIDOMEventListener* aOnabort)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_ABORT]);

  return SetOnXListener(type, aOnabort);
}

NS_IMETHODIMP
nsDOMWorkerXHREventTarget::GetOnerror(nsIDOMEventListener** aOnerror)
{
  NS_ENSURE_ARG_POINTER(aOnerror);
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_ERROR]);

  nsCOMPtr<nsIDOMEventListener> listener = GetOnXListener(type);
  listener.forget(aOnerror);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHREventTarget::SetOnerror(nsIDOMEventListener* aOnerror)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_ERROR]);

  return SetOnXListener(type, aOnerror);
}

NS_IMETHODIMP
nsDOMWorkerXHREventTarget::GetOnload(nsIDOMEventListener** aOnload)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER(aOnload);

  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_LOAD]);

  nsCOMPtr<nsIDOMEventListener> listener = GetOnXListener(type);
  listener.forget(aOnload);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHREventTarget::SetOnload(nsIDOMEventListener* aOnload)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_LOAD]);

  return SetOnXListener(type, aOnload);
}

NS_IMETHODIMP
nsDOMWorkerXHREventTarget::GetOnloadstart(nsIDOMEventListener** aOnloadstart)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER(aOnloadstart);

  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_LOADSTART]);

  nsCOMPtr<nsIDOMEventListener> listener = GetOnXListener(type);
  listener.forget(aOnloadstart);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHREventTarget::SetOnloadstart(nsIDOMEventListener* aOnloadstart)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_LOADSTART]);

  return SetOnXListener(type, aOnloadstart);
}

NS_IMETHODIMP
nsDOMWorkerXHREventTarget::GetOnprogress(nsIDOMEventListener** aOnprogress)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER(aOnprogress);

  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_PROGRESS]);

  nsCOMPtr<nsIDOMEventListener> listener = GetOnXListener(type);
  listener.forget(aOnprogress);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHREventTarget::SetOnprogress(nsIDOMEventListener* aOnprogress)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_PROGRESS]);

  return SetOnXListener(type, aOnprogress);
}

nsDOMWorkerXHRUpload::nsDOMWorkerXHRUpload(nsDOMWorkerXHR* aWorkerXHR)
: mWorkerXHR(aWorkerXHR)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aWorkerXHR, "Null pointer!");
}

NS_IMPL_ISUPPORTS_INHERITED1(nsDOMWorkerXHRUpload, nsDOMWorkerXHREventTarget,
                                                   nsIXMLHttpRequestUpload)

NS_IMPL_CI_INTERFACE_GETTER3(nsDOMWorkerXHRUpload, nsIDOMEventTarget,
                                                   nsIXMLHttpRequestEventTarget,
                                                   nsIXMLHttpRequestUpload)

NS_IMPL_THREADSAFE_DOM_CI_GETINTERFACES(nsDOMWorkerXHRUpload)

NS_IMETHODIMP
nsDOMWorkerXHRUpload::AddEventListener(const nsAString& aType,
                                       nsIDOMEventListener* aListener,
                                       PRBool aUseCapture)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER(aListener);

  if (mWorkerXHR->mWorker->IsCanceled()) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = nsDOMWorkerXHREventTarget::AddEventListener(aType, aListener,
                                                            aUseCapture);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mWorkerXHR->mXHRProxy->UploadEventListenerAdded();
  if (NS_FAILED(rv)) {
    NS_WARNING("UploadEventListenerAdded failed!");
    RemoveEventListener(aType, aListener, aUseCapture);
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHRUpload::RemoveEventListener(const nsAString& aType,
                                          nsIDOMEventListener* aListener,
                                          PRBool aUseCapture)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER(aListener);

  if (mWorkerXHR->mWorker->IsCanceled()) {
    return NS_ERROR_ABORT;
  }

  return nsDOMWorkerXHREventTarget::RemoveEventListener(aType, aListener,
                                                        aUseCapture);
}

NS_IMETHODIMP
nsDOMWorkerXHRUpload::DispatchEvent(nsIDOMEvent* aEvent,
                                    PRBool* _retval)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER(aEvent);

  if (mWorkerXHR->mWorker->IsCanceled()) {
    return NS_ERROR_ABORT;
  }

  return nsDOMWorkerXHREventTarget::DispatchEvent(aEvent, _retval);
}

nsresult
nsDOMWorkerXHRUpload::SetOnXListener(const nsAString& aType,
                                     nsIDOMEventListener* aListener)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mWorkerXHR->mCanceled) {
    return NS_ERROR_ABORT;
  }

  PRUint32 type = GetListenerTypeFromString(aType);
  if (type > sMaxUploadEventTypes) {
    
    return NS_OK;
  }

  return nsDOMWorkerXHREventTarget::SetOnXListener(aType, aListener);
}

nsDOMWorkerXHR::nsDOMWorkerXHR(nsDOMWorker* aWorker)
: nsDOMWorkerFeature(aWorker),
  mWrappedNative(nsnull),
  mCanceled(PR_FALSE)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aWorker, "Must have a worker!");
}

nsDOMWorkerXHR::~nsDOMWorkerXHR()
{
  if (mXHRProxy) {
    if (!NS_IsMainThread()) {
      nsCOMPtr<nsIRunnable> runnable =
        NS_NEW_RUNNABLE_METHOD(nsDOMWorkerXHRProxy, mXHRProxy.get(), Destroy);

      if (runnable) {
        mXHRProxy = nsnull;
        NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL);
      }
    }
    else {
      mXHRProxy->Destroy();
    }
  }
}




NS_IMPL_ADDREF_INHERITED(nsDOMWorkerXHR, nsDOMWorkerFeature)
NS_IMPL_RELEASE_INHERITED(nsDOMWorkerXHR, nsDOMWorkerFeature)

NS_IMPL_QUERY_INTERFACE_INHERITED2(nsDOMWorkerXHR, nsDOMWorkerXHREventTarget,
                                                   nsIXMLHttpRequest,
                                                   nsIXPCScriptable)

NS_IMPL_CI_INTERFACE_GETTER3(nsDOMWorkerXHR, nsIDOMEventTarget,
                                             nsIXMLHttpRequestEventTarget,
                                             nsIXMLHttpRequest)

NS_IMPL_THREADSAFE_DOM_CI_GETINTERFACES(nsDOMWorkerXHR)

#define XPC_MAP_CLASSNAME nsDOMWorkerXHR
#define XPC_MAP_QUOTED_CLASSNAME "XMLHttpRequest"
#define XPC_MAP_WANT_POSTCREATE
#define XPC_MAP_WANT_TRACE
#define XPC_MAP_WANT_FINALIZE

#define XPC_MAP_FLAGS                                  \
  nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE        | \
  nsIXPCScriptable::CLASSINFO_INTERFACES_ONLY        | \
  nsIXPCScriptable::DONT_REFLECT_INTERFACE_NAMES

#include "xpc_map_end.h"

NS_IMETHODIMP
nsDOMWorkerXHR::Trace(nsIXPConnectWrappedNative* ,
                      JSTracer* aTracer,
                      JSObject* )
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mCanceled) {
    nsDOMWorkerMessageHandler::Trace(aTracer);
    if (mUpload) {
      mUpload->Trace(aTracer);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::Finalize(nsIXPConnectWrappedNative* ,
                         JSContext* ,
                         JSObject* )
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsDOMWorkerMessageHandler::ClearAllListeners();

  if (mUpload) {
    mUpload->ClearAllListeners();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::PostCreate(nsIXPConnectWrappedNative* aWrapper,
                           JSContext* ,
                           JSObject* )
{
  mWrappedNative = aWrapper;
  return NS_OK;
}

nsresult
nsDOMWorkerXHR::Init()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<nsDOMWorkerXHRProxy> proxy = new nsDOMWorkerXHRProxy(this);
  NS_ENSURE_TRUE(proxy, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = proxy->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  proxy.swap(mXHRProxy);
  return NS_OK;
}

void
nsDOMWorkerXHR::Cancel()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
  
  nsRefPtr<nsDOMWorkerXHR> kungFuDeathGrip(this);

  {
    
    
    nsAutoLock lock(mWorker->Lock());

    mCanceled = PR_TRUE;
    mUpload = nsnull;
  }

  if (mXHRProxy) {
    mXHRProxy->Destroy();
    mXHRProxy = nsnull;
  }

  mWorker = nsnull;
}

nsresult
nsDOMWorkerXHR::SetOnXListener(const nsAString& aType,
                               nsIDOMEventListener* aListener)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  PRUint32 type = GetListenerTypeFromString(aType);
  if (type > sMaxXHREventTypes) {
    
    return NS_OK;
  }

  return nsDOMWorkerXHREventTarget::SetOnXListener(aType, aListener);
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetChannel(nsIChannel** aChannel)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aChannel);
  *aChannel = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetResponseXML(nsIDOMDocument** aResponseXML)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aResponseXML);
  *aResponseXML = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetResponseText(nsAString& aResponseText)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = mXHRProxy->GetResponseText(aResponseText);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetStatus(PRUint32* aStatus)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER(aStatus);

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = mXHRProxy->GetStatus(aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetStatusText(nsACString& aStatusText)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = mXHRProxy->GetStatusText(aStatusText);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::Abort()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = mXHRProxy->Abort();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetAllResponseHeaders(char** _retval)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER(_retval);

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = mXHRProxy->GetAllResponseHeaders(_retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetResponseHeader(const nsACString& aHeader,
                                  nsACString& _retval)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = mXHRProxy->GetResponseHeader(aHeader, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::OpenRequest(const nsACString& aMethod,
                            const nsACString& aUrl,
                            PRBool aAsync,
                            const nsAString& aUser,
                            const nsAString& aPassword)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = mXHRProxy->OpenRequest(aMethod, aUrl, aAsync, aUser, aPassword);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::Open(const nsACString& aMethod, const nsACString& aUrl,
                     PRBool aAsync, const nsAString& aUser,
                     const nsAString& aPassword, PRUint8 optional_argc)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  if (!optional_argc) {
      aAsync = PR_TRUE;
  }

  return OpenRequest(aMethod, aUrl, aAsync, aUser, aPassword);
}

NS_IMETHODIMP
nsDOMWorkerXHR::Send(nsIVariant* aBody)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  if (mWorker->IsClosing() && !mXHRProxy->mSyncRequest) {
    
    
    return NS_OK;
  }

  nsresult rv = mXHRProxy->Send(aBody);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::SendAsBinary(const nsAString& aBody)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  if (mWorker->IsClosing() && !mXHRProxy->mSyncRequest) {
    
    
    return NS_OK;
  }

  nsresult rv = mXHRProxy->SendAsBinary(aBody);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::SetRequestHeader(const nsACString& aHeader,
                                 const nsACString& aValue)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = mXHRProxy->SetRequestHeader(aHeader, aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetReadyState(PRInt32* aReadyState)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  NS_ENSURE_ARG_POINTER(aReadyState);

  nsresult rv = mXHRProxy->GetReadyState(aReadyState);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::OverrideMimeType(const nsACString& aMimetype)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = mXHRProxy->OverrideMimeType(aMimetype);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetMultipart(PRBool* aMultipart)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  NS_ENSURE_ARG_POINTER(aMultipart);

  nsresult rv = mXHRProxy->GetMultipart(aMultipart);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::SetMultipart(PRBool aMultipart)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = mXHRProxy->SetMultipart(aMultipart);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetMozBackgroundRequest(PRBool* aMozBackgroundRequest)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  NS_ENSURE_ARG_POINTER(aMozBackgroundRequest);

  *aMozBackgroundRequest = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::SetMozBackgroundRequest(PRBool aMozBackgroundRequest)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (aMozBackgroundRequest) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::Init(nsIPrincipal* aPrincipal,
                     nsIScriptContext* aScriptContext,
                     nsPIDOMWindow* aOwnerWindow,
                     nsIURI* aBaseURI)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_NOTREACHED("No one should be calling this!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetUpload(nsIXMLHttpRequestUpload** aUpload)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<nsDOMWorker> worker = mWorker;
  if (!worker) {
    return NS_ERROR_ABORT;
  }

  nsAutoLock lock(worker->Lock());

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  NS_ENSURE_ARG_POINTER(aUpload);

  if (!mUpload) {
    mUpload = new nsDOMWorkerXHRUpload(this);
    NS_ENSURE_TRUE(mUpload, NS_ERROR_OUT_OF_MEMORY);
  }

  NS_ADDREF(*aUpload = mUpload);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetOnreadystatechange(nsIDOMEventListener** aOnreadystatechange)
{
  NS_ENSURE_ARG_POINTER(aOnreadystatechange);

  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_READYSTATECHANGE]);

  nsCOMPtr<nsIDOMEventListener> listener = GetOnXListener(type);
  listener.forget(aOnreadystatechange);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::SetOnreadystatechange(nsIDOMEventListener* aOnreadystatechange)
{
  nsAutoString type;
  type.AssignASCII(sListenerTypes[LISTENER_TYPE_READYSTATECHANGE]);

  return SetOnXListener(type, aOnreadystatechange);
}

NS_IMETHODIMP
nsDOMWorkerXHR::GetWithCredentials(PRBool* aWithCredentials)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  NS_ENSURE_ARG_POINTER(aWithCredentials);

  nsresult rv = mXHRProxy->GetWithCredentials(aWithCredentials);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerXHR::SetWithCredentials(PRBool aWithCredentials)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  nsresult rv = mXHRProxy->SetWithCredentials(aWithCredentials);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
