







































#include "IDBRequest.h"

#include "nsIScriptContext.h"
#include "nsIVariant.h"

#include "nsComponentManagerUtils.h"
#include "nsDOMClassInfoID.h"
#include "nsDOMJSUtils.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsPIDOMWindow.h"
#include "nsStringGlue.h"
#include "nsThreadUtils.h"

#include "AsyncConnectionHelper.h"
#include "IDBEvents.h"
#include "IDBTransaction.h"
#include "nsContentUtils.h"

USING_INDEXEDDB_NAMESPACE

IDBRequest::IDBRequest()
: mResultVal(JSVAL_VOID),
  mErrorCode(0),
  mResultValRooted(false),
  mHaveResultOrErrorCode(false)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

IDBRequest::~IDBRequest()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (mResultValRooted) {
    
    
    
    UnrootResultVal();
  }

  if (mListenerManager) {
    mListenerManager->Disconnect();
  }
}


already_AddRefed<IDBRequest>
IDBRequest::Create(nsISupports* aSource,
                   nsIScriptContext* aScriptContext,
                   nsPIDOMWindow* aOwner,
                   IDBTransaction* aTransaction)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!aScriptContext || !aOwner) {
    NS_ERROR("Null context and owner!");
    return nsnull;
  }

  nsRefPtr<IDBRequest> request(new IDBRequest());

  request->mSource = aSource;
  request->mTransaction = aTransaction;
  request->mScriptContext = aScriptContext;
  request->mOwner = aOwner;

  return request.forget();
}

void
IDBRequest::Reset()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  mResultVal = JSVAL_VOID;
  mHaveResultOrErrorCode = false;
  mErrorCode = 0;
  if (mResultValRooted) {
    UnrootResultVal();
  }
}

nsresult
IDBRequest::NotifyHelperCompleted(HelperBase* aHelper)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!mHaveResultOrErrorCode, "Already called!");
  NS_ASSERTION(!mResultValRooted, "Already rooted?!");
  NS_ASSERTION(JSVAL_IS_VOID(mResultVal), "Should be undefined!");

  
  
  if (NS_FAILED(CheckInnerWindowCorrectness())) {
    return NS_OK;
  }

  mHaveResultOrErrorCode = true;

  nsresult rv = aHelper->GetResultCode();

  
  if (NS_FAILED(rv)) {
    mErrorCode = NS_ERROR_GET_CODE(rv);
    return NS_OK;
  }

  
  JSContext* cx = mScriptContext->GetNativeContext();
  NS_ASSERTION(cx, "Failed to get a context!");

  JSObject* global = static_cast<JSObject*>(mScriptContext->GetNativeGlobal());
  NS_ASSERTION(global, "Failed to get global object!");

  JSAutoRequest ar(cx);
  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, global)) {
    rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }
  else {
    RootResultVal();

    rv = aHelper->GetSuccessResult(cx, &mResultVal);
    if (NS_SUCCEEDED(rv)) {
      
      if (!JSVAL_IS_GCTHING(mResultVal)) {
        UnrootResultVal();
      }
    }
    else {
      NS_WARNING("GetSuccessResult failed!");
    }
  }

  if (NS_SUCCEEDED(rv)) {
    mErrorCode = 0;
  }
  else {
    mErrorCode = NS_ERROR_GET_CODE(rv);
    mResultVal = JSVAL_VOID;
  }

  return rv;
}

void
IDBRequest::RootResultVal()
{
  NS_ASSERTION(!mResultValRooted, "This should be false!");
  NS_HOLD_JS_OBJECTS(this, IDBRequest);
  mResultValRooted = true;
}

void
IDBRequest::UnrootResultVal()
{
  NS_ASSERTION(mResultValRooted, "This should be true!");
  NS_DROP_JS_OBJECTS(this, IDBRequest);
  mResultValRooted = false;
}

NS_IMETHODIMP
IDBRequest::GetReadyState(PRUint16* aReadyState)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  *aReadyState = mHaveResultOrErrorCode ?
                 nsIIDBRequest::DONE :
                 nsIIDBRequest::LOADING;

  return NS_OK;
}

NS_IMETHODIMP
IDBRequest::GetSource(nsISupports** aSource)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsISupports> source(mSource);
  source.forget(aSource);
  return NS_OK;
}

NS_IMETHODIMP
IDBRequest::GetTransaction(nsIIDBTransaction** aTransaction)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIIDBTransaction> transaction(mTransaction);
  transaction.forget(aTransaction);
  return NS_OK;
}

NS_IMETHODIMP
IDBRequest::GetResult(jsval* aResult)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mHaveResultOrErrorCode) {
    
    return NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
  }

  *aResult = mResultVal;
  return NS_OK;
}

NS_IMETHODIMP
IDBRequest::GetErrorCode(PRUint16* aErrorCode)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mHaveResultOrErrorCode) {
    
    return NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
  }

  *aErrorCode = mErrorCode;
  return NS_OK;
}

NS_IMETHODIMP
IDBRequest::SetOnsuccess(nsIDOMEventListener* aSuccessListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  return RemoveAddEventListener(NS_LITERAL_STRING(SUCCESS_EVT_STR),
                                mOnSuccessListener, aSuccessListener);
}

NS_IMETHODIMP
IDBRequest::GetOnsuccess(nsIDOMEventListener** aSuccessListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  return GetInnerEventListener(mOnSuccessListener, aSuccessListener);
}

NS_IMETHODIMP
IDBRequest::SetOnerror(nsIDOMEventListener* aErrorListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  return RemoveAddEventListener(NS_LITERAL_STRING(ERROR_EVT_STR),
                                mOnErrorListener, aErrorListener);
}

NS_IMETHODIMP
IDBRequest::GetOnerror(nsIDOMEventListener** aErrorListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  return GetInnerEventListener(mOnErrorListener, aErrorListener);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(IDBRequest)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBRequest,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnSuccessListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSource)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mTransaction,
                                                       nsPIDOMEventTarget)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBRequest,
                                                nsDOMEventTargetHelper)
  if (tmp->mResultValRooted) {
    tmp->mResultVal = JSVAL_VOID;
    tmp->UnrootResultVal();
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnSuccessListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSource)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mTransaction)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(IDBRequest)
  if (JSVAL_IS_GCTHING(tmp->mResultVal)) {
    void *gcThing = JSVAL_TO_GCTHING(tmp->mResultVal);
    NS_IMPL_CYCLE_COLLECTION_TRACE_JS_CALLBACK(gcThing, "mResultVal")
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBRequest)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBRequest)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(IDBRequest, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(IDBRequest, nsDOMEventTargetHelper)

DOMCI_DATA(IDBRequest, IDBRequest)

nsresult
IDBRequest::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  aVisitor.mCanHandle = true;
  aVisitor.mParentTarget = mTransaction;
  return NS_OK;
}

IDBOpenDBRequest::~IDBOpenDBRequest()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (mResultValRooted) {
    UnrootResultVal();
  }
}


already_AddRefed<IDBOpenDBRequest>
IDBOpenDBRequest::Create(nsIScriptContext* aScriptContext,
                         nsPIDOMWindow* aOwner)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!aScriptContext || !aOwner) {
    NS_ERROR("Null context and owner!");
    return nsnull;
  }

  nsRefPtr<IDBOpenDBRequest> request(new IDBOpenDBRequest());

  request->mScriptContext = aScriptContext;
  request->mOwner = aOwner;

  return request.forget();
}

void
IDBOpenDBRequest::SetTransaction(IDBTransaction* aTransaction)
{
  mTransaction = aTransaction;
}

void
IDBOpenDBRequest::RootResultVal()
{
  NS_ASSERTION(!mResultValRooted, "This should be false!");
  NS_HOLD_JS_OBJECTS(this, IDBOpenDBRequest);
  mResultValRooted = true;
}

void
IDBOpenDBRequest::UnrootResultVal()
{
  NS_ASSERTION(mResultValRooted, "This should be true!");
  NS_DROP_JS_OBJECTS(this, IDBOpenDBRequest);
  mResultValRooted = false;
}

NS_IMPL_EVENT_HANDLER(IDBOpenDBRequest, blocked)
NS_IMPL_EVENT_HANDLER(IDBOpenDBRequest, upgradeneeded)

NS_IMPL_CYCLE_COLLECTION_CLASS(IDBOpenDBRequest)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBOpenDBRequest,
                                                  IDBRequest)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnupgradeneededListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnblockedListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBOpenDBRequest,
                                                IDBRequest)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnupgradeneededListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnblockedListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBOpenDBRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBOpenDBRequest)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBOpenDBRequest)
NS_INTERFACE_MAP_END_INHERITING(IDBRequest)

NS_IMPL_ADDREF_INHERITED(IDBOpenDBRequest, IDBRequest)
NS_IMPL_RELEASE_INHERITED(IDBOpenDBRequest, IDBRequest)

DOMCI_DATA(IDBOpenDBRequest, IDBOpenDBRequest)
