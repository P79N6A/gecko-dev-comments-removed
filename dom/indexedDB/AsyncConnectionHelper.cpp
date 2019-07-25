






































#include "AsyncConnectionHelper.h"

#include "nsIIDBDatabaseException.h"

#include "mozilla/storage.h"
#include "nsComponentManagerUtils.h"
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

} 

AsyncConnectionHelper::AsyncConnectionHelper(IDBDatabase* aDatabase,
                                             IDBRequest* aRequest)
: mDatabase(aDatabase),
  mRequest(aRequest),
  mTimeoutDuration(TimeDuration::FromMilliseconds(kDefaultTimeoutMS)),
  mErrorCode(0),
  mError(PR_FALSE)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mRequest, "Null request!");
}

AsyncConnectionHelper::AsyncConnectionHelper(IDBTransaction* aTransaction,
                                             IDBRequest* aRequest)
: mDatabase(aTransaction->mDatabase),
  mTransaction(aTransaction),
  mRequest(aRequest),
  mTimeoutDuration(TimeDuration::FromMilliseconds(kDefaultTimeoutMS)),
  mErrorCode(0),
  mError(PR_FALSE)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mRequest, "Null request!");
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
    mRequest->SetDone();

    
    
    if (mError || ((mErrorCode = OnSuccess(mRequest)) != OK)) {
      OnError(mRequest, mErrorCode);
    }

    if (mTransaction) {
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
  else if (mDatabase) {
    rv = mDatabase->GetOrCreateConnection(getter_AddRefs(connection));
    if (NS_SUCCEEDED(rv)) {
      NS_ASSERTION(connection, "This should never be null!");
    }
  }

  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "GetOrCreateConnection failed!");

  if (connection) {
    rv = connection->SetProgressHandler(kProgressHandlerGranularity, this,
                                        getter_AddRefs(mOldProgressHandler));
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetProgressHandler failed!");
    if (NS_SUCCEEDED(rv)) {
      mStartTime = TimeStamp::Now();
    }
  }

  if (NS_SUCCEEDED(rv)) {
    if (mDatabase) {
      IDBFactory::SetCurrentDatabase(mDatabase);
    }
    mErrorCode = DoDatabaseWork(connection);
    if (mDatabase) {
      IDBFactory::SetCurrentDatabase(nsnull);
    }
  }
  else {
    mErrorCode = nsIIDBDatabaseException::UNKNOWN_ERR;
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

  mError = mErrorCode != OK;
  return NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
AsyncConnectionHelper::OnProgress(mozIStorageConnection* aConnection,
                                  PRBool* _retval)
{
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

  return NS_OK;
}

nsresult
AsyncConnectionHelper::DispatchToTransactionPool()
{
  NS_ASSERTION(mTransaction, "Only ok to call this with a transaction!");
  TransactionPoolEventTarget target(mTransaction);
  return Dispatch(&target);
}

nsresult
AsyncConnectionHelper::Init()
{
  return NS_OK;
}

PRUint16
AsyncConnectionHelper::OnSuccess(nsIDOMEventTarget* aTarget)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIWritableVariant> variant =
    do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!variant) {
    NS_ERROR("Couldn't create variant!");
    return nsIIDBDatabaseException::UNKNOWN_ERR;
  }

  PRUint16 result = GetSuccessResult(variant);
  if (result != OK) {
    return result;
  }

  
  nsCOMPtr<nsPIDOMEventTarget> target(do_QueryInterface(aTarget));
  if (target) {
    nsIEventListenerManager* manager = target->GetListenerManager(PR_FALSE);
    if (!manager ||
        !manager->HasListenersFor(NS_LITERAL_STRING(SUCCESS_EVT_STR))) {
      
      return OK;
    }
  }

  if (NS_FAILED(variant->SetWritable(PR_FALSE))) {
    NS_ERROR("Failed to make variant readonly!");
    return nsIIDBDatabaseException::UNKNOWN_ERR;
  }

  nsCOMPtr<nsIDOMEvent> event =
    IDBSuccessEvent::Create(mRequest, variant, mTransaction);
  if (!event) {
    NS_ERROR("Failed to create event!");
    return nsIIDBDatabaseException::UNKNOWN_ERR;
  }

  PRBool dummy;
  aTarget->DispatchEvent(event, &dummy);
  return OK;
}

void
AsyncConnectionHelper::OnError(nsIDOMEventTarget* aTarget,
                               PRUint16 aErrorCode)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
  nsCOMPtr<nsIDOMEvent> event(IDBErrorEvent::Create(mRequest, aErrorCode));
  if (!event) {
    NS_ERROR("Failed to create event!");
    return;
  }

  PRBool dummy;
  aTarget->DispatchEvent(event, &dummy);
}

PRUint16
AsyncConnectionHelper::GetSuccessResult(nsIWritableVariant* )
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  

  return OK;
}

void
AsyncConnectionHelper::ReleaseMainThreadObjects()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  mDatabase = nsnull;
  mTransaction = nsnull;
  mRequest = nsnull;
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
