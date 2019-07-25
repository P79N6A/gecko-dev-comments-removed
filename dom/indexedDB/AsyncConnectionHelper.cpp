






































#include "AsyncConnectionHelper.h"

#include "mozIStorageConnection.h"
#include "nsIIDBDatabaseException.h"

#include "nsComponentManagerUtils.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"

#include "IDBEvents.h"

USING_INDEXEDDB_NAMESPACE

AsyncConnectionHelper::AsyncConnectionHelper(IDBDatabaseRequest* aDatabase,
                                             IDBRequest* aRequest)
: mDatabase(aDatabase),
  mRequest(aRequest),
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
}

NS_IMPL_THREADSAFE_ISUPPORTS1(AsyncConnectionHelper, nsIRunnable)

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

  mErrorCode = DoDatabaseWork();

  if (mErrorCode != NOREPLY) {
    mError = mErrorCode != OK;

    return NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL);
  }

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
