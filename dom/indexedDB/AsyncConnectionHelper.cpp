






































#include "AsyncConnectionHelper.h"

#include "mozilla/storage.h"
#include "nsComponentManagerUtils.h"
#include "nsContentUtils.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"

#include "IDBEvents.h"
#include "IDBFactory.h"
#include "IDBTransaction.h"
#include "TransactionThreadPool.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;

USING_INDEXEDDB_NAMESPACE

namespace {

IDBTransaction* gCurrentTransaction = nsnull;

const PRUint32 kProgressHandlerGranularity = 1000;
const PRUint32 kDefaultTimeoutMS = 30000;

NS_STACK_CLASS
class TransactionPoolEventTarget : public nsIEventTarget
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEVENTTARGET

  TransactionPoolEventTarget(IDBTransaction* aTransaction)
  : mTransaction(aTransaction)
  { }

private:
  IDBTransaction* mTransaction;
};



inline
nsresult
ConvertCloneBuffersToArrayInternal(
                                JSContext* aCx,
                                nsTArray<JSAutoStructuredCloneBuffer>& aBuffers,
                                jsval* aResult)
{
  JSObject* array = JS_NewArrayObject(aCx, 0, nsnull);
  if (!array) {
    NS_WARNING("Failed to make array!");
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  if (!aBuffers.IsEmpty()) {
    if (!JS_SetArrayLength(aCx, array, jsuint(aBuffers.Length()))) {
      NS_WARNING("Failed to set array length!");
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    jsint count = jsint(aBuffers.Length());
    for (jsint index = 0; index < count; index++) {
      JSAutoStructuredCloneBuffer& buffer = aBuffers[index];

      jsval val;
      if (!IDBObjectStore::DeserializeValue(aCx, buffer, &val)) {
        NS_WARNING("Failed to decode!");
        return NS_ERROR_DOM_DATA_CLONE_ERR;
      }

      if (!JS_SetElement(aCx, array, index, &val)) {
        NS_WARNING("Failed to set array element!");
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }
    }
  }

  *aResult = OBJECT_TO_JSVAL(array);
  return NS_OK;
}

} 

AsyncConnectionHelper::AsyncConnectionHelper(IDBDatabase* aDatabase,
                                             IDBRequest* aRequest)
: mDatabase(aDatabase),
  mRequest(aRequest),
  mTimeoutDuration(TimeDuration::FromMilliseconds(kDefaultTimeoutMS)),
  mResultCode(NS_OK),
  mDispatched(PR_FALSE)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

AsyncConnectionHelper::AsyncConnectionHelper(IDBTransaction* aTransaction,
                                             IDBRequest* aRequest)
: mDatabase(aTransaction->mDatabase),
  mTransaction(aTransaction),
  mRequest(aRequest),
  mTimeoutDuration(TimeDuration::FromMilliseconds(kDefaultTimeoutMS)),
  mResultCode(NS_OK),
  mDispatched(PR_FALSE)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

AsyncConnectionHelper::~AsyncConnectionHelper()
{
  if (!NS_IsMainThread()) {
    IDBDatabase* database;
    mDatabase.forget(&database);

    IDBTransaction* transaction;
    mTransaction.forget(&transaction);

    IDBRequest* request;
    mRequest.forget(&request);

    nsCOMPtr<nsIThread> mainThread;
    NS_GetMainThread(getter_AddRefs(mainThread));
    NS_WARN_IF_FALSE(mainThread, "Couldn't get the main thread!");

    if (mainThread) {
      if (database) {
        NS_ProxyRelease(mainThread, static_cast<nsIIDBDatabase*>(database));
      }
      if (transaction) {
        NS_ProxyRelease(mainThread,
                        static_cast<nsIIDBTransaction*>(transaction));
      }
      if (request) {
        NS_ProxyRelease(mainThread, static_cast<nsIDOMEventTarget*>(request));
      }
    }
  }

  NS_ASSERTION(!mOldProgressHandler, "Should not have anything here!");
}

NS_IMPL_THREADSAFE_ISUPPORTS2(AsyncConnectionHelper, nsIRunnable,
                                                     mozIStorageProgressHandler)

NS_IMETHODIMP
AsyncConnectionHelper::Run()
{
  if (NS_IsMainThread()) {
    if (mTransaction &&
        mTransaction->IsAborted() &&
        NS_SUCCEEDED(mResultCode)) {
      
      
      mResultCode = NS_ERROR_DOM_INDEXEDDB_ABORT_ERR;
    }

    IDBTransaction* oldTransaction = gCurrentTransaction;
    gCurrentTransaction = mTransaction;

    if (mRequest) {
      nsresult rv = mRequest->SetDone(this);
      if (NS_SUCCEEDED(mResultCode) && NS_FAILED(rv)) {
        mResultCode = rv;
      }
    }

    
    
    if (NS_FAILED(mResultCode) ||
        NS_FAILED((mResultCode = OnSuccess()))) {
      OnError();
    }

    NS_ASSERTION(gCurrentTransaction == mTransaction, "Should be unchanged!");
    gCurrentTransaction = oldTransaction;

    if (mDispatched && mTransaction) {
      mTransaction->OnRequestFinished();
    }

    ReleaseMainThreadObjects();

    NS_ASSERTION(!(mDatabase || mTransaction || mRequest), "Subclass didn't "
                 "call AsyncConnectionHelper::ReleaseMainThreadObjects!");

    return NS_OK;
  }

  nsresult rv = NS_OK;
  nsCOMPtr<mozIStorageConnection> connection;

  if (mTransaction) {
    rv = mTransaction->GetOrCreateConnection(getter_AddRefs(connection));
    if (NS_SUCCEEDED(rv)) {
      NS_ASSERTION(connection, "This should never be null!");
    }
  }

  if (connection) {
    rv = connection->SetProgressHandler(kProgressHandlerGranularity, this,
                                        getter_AddRefs(mOldProgressHandler));
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetProgressHandler failed!");
    if (NS_SUCCEEDED(rv)) {
      mStartTime = TimeStamp::Now();
    }
  }

  if (NS_SUCCEEDED(rv)) {
    bool hasSavepoint = false;
    if (mDatabase) {
      IDBFactory::SetCurrentDatabase(mDatabase);

      
      if (mTransaction) {
        if (!(hasSavepoint = mTransaction->StartSavepoint())) {
          NS_WARNING("Failed to make savepoint!");
        }
      }
    }

    mResultCode = DoDatabaseWork(connection);

    if (mDatabase) {
      IDBFactory::SetCurrentDatabase(nsnull);

      
      if (hasSavepoint) {
        NS_ASSERTION(mTransaction, "Huh?!");
        if (NS_SUCCEEDED(mResultCode)) {
          mTransaction->ReleaseSavepoint();
        }
        else {
          mTransaction->RollbackSavepoint();
        }
      }
    }
  }
  else {
    
    
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      mResultCode = NS_ERROR_DOM_INDEXEDDB_RECOVERABLE_ERR;
    }
    else {
      mResultCode = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
  }

  if (!mStartTime.IsNull()) {
    nsCOMPtr<mozIStorageProgressHandler> handler;
    rv = connection->RemoveProgressHandler(getter_AddRefs(handler));
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "RemoveProgressHandler failed!");
#ifdef DEBUG
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsISupports> handlerSupports(do_QueryInterface(handler));
      nsCOMPtr<nsISupports> thisSupports =
        do_QueryInterface(static_cast<nsIRunnable*>(this));
      NS_ASSERTION(thisSupports == handlerSupports, "Mismatch!");
    }
#endif
    mStartTime = TimeStamp();
  }

  return NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
AsyncConnectionHelper::OnProgress(mozIStorageConnection* aConnection,
                                  PRBool* _retval)
{
  if (mDatabase && mDatabase->IsInvalidated()) {
    
    *_retval = PR_TRUE;
    return NS_OK;
  }

  TimeDuration elapsed = TimeStamp::Now() - mStartTime;
  if (elapsed >= mTimeoutDuration) {
    *_retval = PR_TRUE;
    return NS_OK;
  }

  if (mOldProgressHandler) {
    return mOldProgressHandler->OnProgress(aConnection, _retval);
  }

  *_retval = PR_FALSE;
  return NS_OK;
}

nsresult
AsyncConnectionHelper::Dispatch(nsIEventTarget* aDatabaseThread)
{
#ifdef DEBUG
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  {
    PRBool sameThread;
    nsresult rv = aDatabaseThread->IsOnCurrentThread(&sameThread);
    NS_ASSERTION(NS_SUCCEEDED(rv), "IsOnCurrentThread failed!");
    NS_ASSERTION(!sameThread, "Dispatching to main thread not supported!");
  }
#endif

  nsresult rv = Init();
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = aDatabaseThread->Dispatch(this, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mTransaction) {
    mTransaction->OnNewRequest();
  }

  mDispatched = PR_TRUE;

  return NS_OK;
}

nsresult
AsyncConnectionHelper::DispatchToTransactionPool()
{
  NS_ASSERTION(mTransaction, "Only ok to call this with a transaction!");
  TransactionPoolEventTarget target(mTransaction);
  return Dispatch(&target);
}


IDBTransaction*
AsyncConnectionHelper::GetCurrentTransaction()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  return gCurrentTransaction;
}

nsresult
AsyncConnectionHelper::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  return NS_OK;
}

nsresult
AsyncConnectionHelper::OnSuccess()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mRequest, "Null request!");

  nsRefPtr<nsDOMEvent> event =
    CreateGenericEvent(NS_LITERAL_STRING(SUCCESS_EVT_STR));
  if (!event) {
    NS_ERROR("Failed to create event!");
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  PRBool dummy;
  nsresult rv = mRequest->DispatchEvent(event, &dummy);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  nsEvent* internalEvent = event->GetInternalNSEvent();
  NS_ASSERTION(internalEvent, "This should never be null!");

  NS_ASSERTION(!mTransaction ||
               mTransaction->IsOpen() ||
               mTransaction->IsAborted(),
               "How else can this be closed?!");

  if ((internalEvent->flags & NS_EVENT_FLAG_EXCEPTION_THROWN) &&
      mTransaction &&
      mTransaction->IsOpen()) {
    rv = mTransaction->Abort();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
AsyncConnectionHelper::OnError()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mRequest, "Null request!");

  
  nsRefPtr<nsDOMEvent> event =
    CreateGenericEvent(NS_LITERAL_STRING(ERROR_EVT_STR), PR_TRUE);
  if (!event) {
    NS_ERROR("Failed to create event!");
    return;
  }

  PRBool doDefault;
  nsresult rv = mRequest->DispatchEvent(event, &doDefault);
  if (NS_SUCCEEDED(rv)) {
    NS_ASSERTION(!mTransaction ||
                 mTransaction->IsOpen() ||
                 mTransaction->IsAborted(),
                 "How else can this be closed?!");

    if (doDefault &&
        mTransaction &&
        mTransaction->IsOpen() &&
        NS_FAILED(mTransaction->Abort())) {
      NS_WARNING("Failed to abort transaction!");
    }
  }
  else {
    NS_WARNING("DispatchEvent failed!");
  }
}

nsresult
AsyncConnectionHelper::GetSuccessResult(JSContext* aCx,
                                        jsval* aVal)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  *aVal = JSVAL_VOID;
  return NS_OK;
}

void
AsyncConnectionHelper::ReleaseMainThreadObjects()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  mDatabase = nsnull;
  mTransaction = nsnull;
  mRequest = nsnull;
}

nsresult
AsyncConnectionHelper::WrapNative(JSContext* aCx,
                                  nsISupports* aNative,
                                  jsval* aResult)
{
  NS_ASSERTION(aCx, "Null context!");
  NS_ASSERTION(aNative, "Null pointer!");
  NS_ASSERTION(aResult, "Null pointer!");
  NS_ASSERTION(mRequest, "Null request!");

  JSObject* global =
    static_cast<JSObject*>(mRequest->ScriptContext()->GetNativeGlobal());
  NS_ENSURE_TRUE(global, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  nsresult rv =
    nsContentUtils::WrapNative(aCx, global, aNative, aResult);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  return NS_OK;
}


nsresult
AsyncConnectionHelper::ConvertCloneBuffersToArray(
                                JSContext* aCx,
                                nsTArray<JSAutoStructuredCloneBuffer>& aBuffers,
                                jsval* aResult)
{
  NS_ASSERTION(aCx, "Null context!");
  NS_ASSERTION(aResult, "Null pointer!");

  JSAutoRequest ar(aCx);

  nsresult rv = ConvertCloneBuffersToArrayInternal(aCx, aBuffers, aResult);

  for (PRUint32 index = 0; index < aBuffers.Length(); index++) {
    aBuffers[index].clear(aCx);
  }
  aBuffers.Clear();

  return rv;
}

NS_IMETHODIMP_(nsrefcnt)
TransactionPoolEventTarget::AddRef()
{
  NS_NOTREACHED("Don't call me!");
  return 2;
}

NS_IMETHODIMP_(nsrefcnt)
TransactionPoolEventTarget::Release()
{
  NS_NOTREACHED("Don't call me!");
  return 1;
}

NS_IMPL_QUERY_INTERFACE1(TransactionPoolEventTarget, nsIEventTarget)

NS_IMETHODIMP
TransactionPoolEventTarget::Dispatch(nsIRunnable* aRunnable,
                                     PRUint32 aFlags)
{
  NS_ASSERTION(aRunnable, "Null pointer!");
  NS_ASSERTION(aFlags == NS_DISPATCH_NORMAL, "Unsupported!");

  TransactionThreadPool* pool = TransactionThreadPool::GetOrCreate();
  NS_ENSURE_TRUE(pool, NS_ERROR_FAILURE);

  return pool->Dispatch(mTransaction, aRunnable, false, nsnull);
}

NS_IMETHODIMP
TransactionPoolEventTarget::IsOnCurrentThread(PRBool* aResult)
{
  *aResult = PR_FALSE;
  return NS_OK;
}
