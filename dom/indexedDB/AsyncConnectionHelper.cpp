






































#include "AsyncConnectionHelper.h"

#include "nsComponentManagerUtils.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"

#include "IDBEvents.h"

USING_INDEXEDDB_NAMESPACE

AsyncConnectionHelper::AsyncConnectionHelper(
                                   nsCOMPtr<mozIStorageConnection>& aConnection,
                                   nsIDOMEventTarget* aTarget)
: mConnection(aConnection),
  mTarget(aTarget),
  mErrorCode(0),
  mError(PR_FALSE)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mTarget, "Null target!");
}

AsyncConnectionHelper::~AsyncConnectionHelper()
{
  if (!NS_IsMainThread()) {
    NS_ASSERTION(mErrorCode == NOREPLY,
                 "This should only happen if NOREPLY was returned!");

    nsIDOMEventTarget* target;
    mTarget.forget(&target);

    nsCOMPtr<nsIThread> mainThread;
    NS_GetMainThread(getter_AddRefs(mainThread));

    if (mainThread) {
      NS_ProxyRelease(mainThread, target);
    }
    else {
      NS_WARNING("Couldn't get the main thread?! Leaking instead of crashing.");
    }
  }

  NS_ASSERTION(!mTarget, "Should have been released before now!");
  NS_ASSERTION(!mDatabaseThread, "Should have been released before now!");
}

NS_IMPL_THREADSAFE_ISUPPORTS1(AsyncConnectionHelper, nsIRunnable)

NS_IMETHODIMP
AsyncConnectionHelper::Run()
{
  if (NS_IsMainThread()) {
    if (mError) {
      OnError(mTarget, mErrorCode);
    }
    else {
      OnSuccess(mTarget);
    }

    mTarget = nsnull;
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

  return aDatabaseThread->Dispatch(this, NS_DISPATCH_NORMAL);
}

void
AsyncConnectionHelper::OnSuccess(nsIDOMEventTarget* aTarget)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIWritableVariant> variant =
    do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!variant) {
    NS_ERROR("Couldn't create variant!");
    return;
  }

  GetSuccessResult(variant);

  if (NS_FAILED(variant->SetWritable(PR_FALSE))) {
    NS_ERROR("Failed to make variant readonly!");
    return;
  }

  nsCOMPtr<nsIDOMEvent> event(IDBSuccessEvent::Create(variant));
  if (!event) {
    NS_ERROR("Failed to create event!");
    return;
  }

  PRBool dummy;
  aTarget->DispatchEvent(event, &dummy);
}

void
AsyncConnectionHelper::OnError(nsIDOMEventTarget* aTarget,
                               PRUint16 aErrorCode)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
  nsCOMPtr<nsIDOMEvent> event(IDBErrorEvent::Create(aErrorCode));
  if (!event) {
    NS_ERROR("Failed to create event!");
    return;
  }

  PRBool dummy;
  aTarget->DispatchEvent(event, &dummy);
}

void
AsyncConnectionHelper::GetSuccessResult(nsIWritableVariant* )
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
}
