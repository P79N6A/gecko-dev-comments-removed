







































#include "IDBRequest.h"

#include "nsIScriptContext.h"
#include "nsIVariant.h"

#include "nsComponentManagerUtils.h"
#include "nsDOMClassInfo.h"
#include "nsDOMJSUtils.h"
#include "nsPIDOMWindow.h"
#include "nsStringGlue.h"
#include "nsThreadUtils.h"

#include "IDBEvents.h"

USING_INDEXEDDB_NAMESPACE

already_AddRefed<IDBRequest>
IDBRequest::Generator::GenerateRequestInternal(nsIScriptContext* aScriptContext,
                                               nsPIDOMWindow* aOwner,
                                               PRBool aWriteRequest)
{
  if (!aScriptContext || !aOwner) {
    NS_ERROR("Null context and owner!");
    return nsnull;
  }

  nsRefPtr<IDBRequest> request(new IDBRequest());

  request->mGenerator = this;
  request->mWriteRequest = aWriteRequest;
  request->mScriptContext = aScriptContext;
  request->mOwner = aOwner;

  if (!mLiveRequests.AppendElement(request)) {
    NS_ERROR("Append failed!");
    return nsnull;
  }

  return request.forget();
}

IDBRequest::~IDBRequest()
{
  mGenerator->NoteDyingRequest(this);

  if (mListenerManager) {
    mListenerManager->Disconnect();
  }
}

NS_IMETHODIMP
IDBRequest::Abort()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (mAborted || mReadyState != nsIIDBRequest::LOADING) {
    return NS_OK;
  }

  if (mWriteRequest) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  mAborted = PR_TRUE;
  mReadyState = nsIIDBRequest::DONE;
  return NS_OK;
}

NS_IMETHODIMP
IDBRequest::GetReadyState(PRUint16* aReadyState)
{
  *aReadyState = mReadyState;
  return NS_OK;
}

NS_IMETHODIMP
IDBRequest::SetOnsuccess(nsIDOMEventListener* aSuccessListener)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(SUCCESS_EVT_STR),
                                mOnSuccessListener, aSuccessListener);
}

NS_IMETHODIMP
IDBRequest::GetOnsuccess(nsIDOMEventListener** aSuccessListener)
{
  return GetInnerEventListener(mOnSuccessListener, aSuccessListener);
}

NS_IMETHODIMP
IDBRequest::SetOnerror(nsIDOMEventListener* aErrorListener)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(ERROR_EVT_STR),
                                mOnErrorListener, aErrorListener);
}

NS_IMETHODIMP
IDBRequest::GetOnerror(nsIDOMEventListener** aErrorListener)
{
  return GetInnerEventListener(mOnErrorListener, aErrorListener);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(IDBRequest)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBRequest,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnSuccessListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnErrorListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBRequest,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnSuccessListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnErrorListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBRequest)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBRequest)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(IDBRequest, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(IDBRequest, nsDOMEventTargetHelper)

DOMCI_DATA(IDBRequest, IDBRequest)
