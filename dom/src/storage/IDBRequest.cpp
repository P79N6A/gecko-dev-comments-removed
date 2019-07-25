






































#include "IDBRequest.h"
#include "nsString.h"
#include "nsIScriptContext.h"

namespace mozilla {
namespace dom {
namespace idb {




#define SUCCESS_EVT_STR "success"
#define ERROR_EVT_STR "error"

Request::Request()
: mReadyState(INITIAL)
{
}




NS_IMETHODIMP
Request::GetReadyState(PRUint16* aReadyState)
{
  *aReadyState = mReadyState;
  return NS_OK;
}

NS_IMETHODIMP
Request::SetOnsuccess(nsIDOMEventListener* aSuccessListener)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(SUCCESS_EVT_STR),
                                mOnSuccessListener, aSuccessListener);
}

NS_IMETHODIMP
Request::GetOnsuccess(nsIDOMEventListener** aSuccessListener)
{
  return GetInnerEventListener(mOnSuccessListener, aSuccessListener);
}

NS_IMETHODIMP
Request::SetOnerror(nsIDOMEventListener* aErrorListener)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(ERROR_EVT_STR),
                                mOnErrorListener, aErrorListener);
}

NS_IMETHODIMP
Request::GetOnerror(nsIDOMEventListener** aErrorListener)
{
  return GetInnerEventListener(mOnErrorListener, aErrorListener);
}




NS_IMPL_CYCLE_COLLECTION_CLASS(Request)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(Request,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnSuccessListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnErrorListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(Request,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnSuccessListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnErrorListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(Request)
  NS_INTERFACE_MAP_ENTRY(nsIIDBRequest)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(Request, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(Request, nsDOMEventTargetHelper)

} 
} 
} 
