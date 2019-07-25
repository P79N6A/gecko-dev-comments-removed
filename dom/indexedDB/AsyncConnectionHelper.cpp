






































#include "AsyncConnectionHelper.h"

#include "nsIIDBDatabaseException.h"

#include "mozilla/Storage.h"
#include "nsComponentManagerUtils.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"

#include "IDBEvents.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;

USING_INDEXEDDB_NAMESPACE

namespace {

const PRUint32 kProgressHandlerGranularity = 1000;
const PRUint32 kDefaultTimeoutMS = 30000;

} 

AsyncConnectionHelper::AsyncConnectionHelper(IDBDatabaseRequest* aDatabase,
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

AsyncConnectionHelper::AsyncConnectionHelper(IDBDatabaseRequest* aDatabase,
                                             IDBRequest* aRequest,
                                             PRUint32 aTimeoutMS)
: mDatabase(aDatabase),
  mRequest(aRequest),
  mTimeoutDuration(TimeDuration::FromMilliseconds(aTimeoutMS)),
  mErrorCode(0),
  mError(PR_FALSE)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mRequest, "Null request!");
}

AsyncConnectionHelper::~AsyncConnectionHelper()
{
  if (!NS_IsMainThread()) {
    NS_ASSERTION(mErrorCode == NOREPLY,
                 "This should only happen if NOREPLY was returned!");

    IDBDatabaseRequest* database;
    mDatabase.forget(&database);

    IDBRequest* request;
    mRequest.forget(&request);

    nsCOMPtr<nsIThread> mainThread;
    NS_GetMainThread(getter_AddRefs(mainThread));
    NS_WARN_IF_FALSE(mainThread, "Couldn't get the main thread!");

    if (mainThread) {
      if (database) {
        NS_ProxyRelease(mainThread, static_cast<nsIIDBDatabase*>(database));
      }
      if (request) {
        NS_ProxyRelease(mainThread, static_cast<nsIDOMEventTarget*>(request));
      }
    }
  }

  NS_ASSERTION(!mDatabaseThread, "Should have been released before now!");
  NS_ASSERTION(!mOldProgressHandler, "Should not have anything here!");
}

NS_IMPL_THREADSAFE_ISUPPORTS2(AsyncConnectionHelper, nsIRunnable,
                                                     mozIStorageProgressHandler)

NS_IMETHODIMP
AsyncConnectionHelper::Run()
{
  if (NS_IsMainThread()) {
    if (mError || ((mErrorCode = OnSuccess(mRequest)) != OK)) {
      OnError(mRequest, mErrorCode);
    }

    mDatabase = nsnull;
    mRequest = nsnull;
    return NS_OK;
  }

#ifdef DEBUG
  {
    PRBool ok;
    NS_ASSERTION(NS_SUCCEEDED(mDatabaseThread->IsOnCurrentThread(&ok)) && ok,
                 "Wrong thread!");
    mDatabaseThread = nsnull;
  }
#endif

  nsresult rv = NS_OK;
  nsCOMPtr<mozIStorageConnection> connection;

  if (mDatabase) {
    rv = mDatabase->EnsureConnection();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "EnsureConnection failed!");

    if (NS_SUCCEEDED(rv)) {
      connection = mDatabase->Connection();
      NS_ASSERTION(connection, "EnsureConnection succeeded but gave us null!");

      rv = connection->SetProgressHandler(kProgressHandlerGranularity, this,
                                          getter_AddRefs(mOldProgressHandler));
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetProgressHandler failed!");
      if (NS_SUCCEEDED(rv)) {
        mStartTime = TimeStamp::Now();
      }
    }
  }

  if (NS_SUCCEEDED(rv)) {
    mozStorageTransaction transaction(connection, PR_FALSE);
    mErrorCode = DoDatabaseWork(connection);
    if (mErrorCode == OK) {
      rv = transaction.Commit();
      mErrorCode = NS_SUCCEEDED(rv) ? OK : nsIIDBDatabaseException::UNKNOWN_ERR;
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

  if (mErrorCode != NOREPLY) {
    mError = mErrorCode != OK;

    return NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL);
  }

  return NS_OK;
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
AsyncConnectionHelper::Dispatch(nsIThread* aDatabaseThread)
{
#ifdef DEBUG
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  {
    PRBool sameThread;
    nsresult rv = aDatabaseThread->IsOnCurrentThread(&sameThread);
    NS_ASSERTION(NS_SUCCEEDED(rv), "IsOnCurrentThread failed!");
    NS_ASSERTION(!sameThread, "Dispatching to main thread not supported!");
  }
  mDatabaseThread = aDatabaseThread;
#endif

  nsresult rv = Init();
  if (NS_FAILED(rv)) {
    return rv;
  }

  return aDatabaseThread->Dispatch(this, NS_DISPATCH_NORMAL);
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

  if (NS_FAILED(variant->SetWritable(PR_FALSE))) {
    NS_ERROR("Failed to make variant readonly!");
    return nsIIDBDatabaseException::UNKNOWN_ERR;
  }

  nsCOMPtr<nsIDOMEvent> event(IDBSuccessEvent::Create(mRequest, variant));
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
