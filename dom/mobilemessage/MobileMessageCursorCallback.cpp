




#include "MobileMessageCursorCallback.h"
#include "mozilla/dom/ScriptSettings.h"
#include "nsIDOMDOMRequest.h"
#include "nsIDOMMozSmsMessage.h"
#include "nsIMobileMessageCallback.h"
#include "DOMCursor.h"
#include "nsServiceManagerUtils.h"      

namespace mozilla {
namespace dom {
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
MobileMessageCursorCallback::NotifyCursorResult(nsISupports* aResult)
{
  MOZ_ASSERT(mDOMCursor);

  AutoJSAPI jsapi;
  if (NS_WARN_IF(!jsapi.Init(mDOMCursor->GetOwner()))) {
    return NS_ERROR_FAILURE;
  }
  JSContext* cx = jsapi.cx();

  JS::Rooted<JS::Value> wrappedResult(cx);
  nsresult rv = nsContentUtils::WrapNative(cx, aResult, &wrappedResult);
  NS_ENSURE_SUCCESS(rv, rv);

  mDOMCursor->FireSuccess(wrappedResult);
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
