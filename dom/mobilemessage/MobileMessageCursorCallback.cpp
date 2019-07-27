




#include "MobileMessageCursorCallback.h"
#include "mozilla/dom/ScriptSettings.h"
#include "nsIDOMDOMRequest.h"
#include "nsIDOMMozSmsMessage.h"
#include "nsIMobileMessageCallback.h"
#include "nsServiceManagerUtils.h"      

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED(MobileMessageCursor, DOMCursor,
                                   mPendingResults)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MobileMessageCursor)
NS_INTERFACE_MAP_END_INHERITING(DOMCursor)

NS_IMPL_ADDREF_INHERITED(MobileMessageCursor, DOMCursor)
NS_IMPL_RELEASE_INHERITED(MobileMessageCursor, DOMCursor)

MobileMessageCursor::MobileMessageCursor(nsPIDOMWindow* aWindow,
                                         nsICursorContinueCallback* aCallback)
  : DOMCursor(aWindow, aCallback)
{
}

NS_IMETHODIMP
MobileMessageCursor::Continue()
{
  
  
  
  
  
  
  
  
  
  
  
  return DOMCursor::Continue();
}

void
MobileMessageCursor::Continue(ErrorResult& aRv)
{
  
  
  
  
  
  
  
  
  
  if (!mPendingResults.Length()) {
    DOMCursor::Continue(aRv);
    return;
  }

  
  
  Reset();

  nsresult rv = FireSuccessWithNextPendingResult();
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }
}

nsresult
MobileMessageCursor::FireSuccessWithNextPendingResult()
{
  
  
  MOZ_ASSERT(mPendingResults.Length());

  AutoJSAPI jsapi;
  if (NS_WARN_IF(!jsapi.Init(GetOwner()))) {
    return NS_ERROR_FAILURE;
  }

  JSContext* cx = jsapi.cx();
  JS::Rooted<JS::Value> val(cx);
  nsresult rv =
    nsContentUtils::WrapNative(cx, mPendingResults.LastElement(), &val);
  NS_ENSURE_SUCCESS(rv, rv);

  mPendingResults.RemoveElementAt(mPendingResults.Length() - 1);

  FireSuccess(val);
  return NS_OK;
}

namespace mobilemessage {

NS_IMPL_CYCLE_COLLECTION(MobileMessageCursorCallback, mDOMCursor)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MobileMessageCursorCallback)
  NS_INTERFACE_MAP_ENTRY(nsIMobileMessageCursorCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(MobileMessageCursorCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MobileMessageCursorCallback)



NS_IMETHODIMP
MobileMessageCursorCallback::NotifyCursorError(int32_t aError)
{
  MOZ_ASSERT(mDOMCursor);

  nsRefPtr<DOMCursor> cursor = mDOMCursor.forget();

  switch (aError) {
    case nsIMobileMessageCallback::NO_SIGNAL_ERROR:
      cursor->FireError(NS_LITERAL_STRING("NoSignalError"));
      break;
    case nsIMobileMessageCallback::NOT_FOUND_ERROR:
      cursor->FireError(NS_LITERAL_STRING("NotFoundError"));
      break;
    case nsIMobileMessageCallback::UNKNOWN_ERROR:
      cursor->FireError(NS_LITERAL_STRING("UnknownError"));
      break;
    case nsIMobileMessageCallback::INTERNAL_ERROR:
      cursor->FireError(NS_LITERAL_STRING("InternalError"));
      break;
    default: 
      MOZ_CRASH("Should never get here!");
  }

  return NS_OK;
}

NS_IMETHODIMP
MobileMessageCursorCallback::NotifyCursorResult(nsISupports** aResults,
                                                uint32_t aSize)
{
  MOZ_ASSERT(mDOMCursor);
  
  
  MOZ_ASSERT(aResults && *aResults && aSize);
  
  nsTArray<nsCOMPtr<nsISupports>>& pending = mDOMCursor->mPendingResults;
  MOZ_ASSERT(pending.Length() == 0);

  
  pending.SetCapacity(pending.Length() + aSize);
  while (aSize) {
    --aSize;
    pending.AppendElement(aResults[aSize]);
  }

  nsresult rv = mDOMCursor->FireSuccessWithNextPendingResult();
  if (NS_FAILED(rv)) {
    NotifyCursorError(nsIMobileMessageCallback::INTERNAL_ERROR);
  }

  return NS_OK;
}

NS_IMETHODIMP
MobileMessageCursorCallback::NotifyCursorDone()
{
  MOZ_ASSERT(mDOMCursor);

  nsRefPtr<DOMCursor> cursor = mDOMCursor.forget();
  cursor->FireDone();

  return NS_OK;
}

} 
} 
} 
