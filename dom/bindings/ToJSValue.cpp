





#include "mozilla/dom/ToJSValue.h"
#include "mozilla/dom/DOMException.h"
#include "mozilla/dom/Exceptions.h"
#include "nsAString.h"
#include "nsContentUtils.h"
#include "nsStringBuffer.h"
#include "xpcpublic.h"

namespace mozilla {
namespace dom {

bool
ToJSValue(JSContext* aCx, const nsAString& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));


  
  nsStringBuffer* sharedBuffer;
  if (!XPCStringConvert::ReadableToJSVal(aCx, aArgument, &sharedBuffer,
                                         aValue)) {
    return false;
  }

  if (sharedBuffer) {
    NS_ADDREF(sharedBuffer);
  }

  return true;
}


namespace tojsvalue_detail {

bool
ISupportsToJSValue(JSContext* aCx,
                   nsISupports* aArgument,
                   JS::MutableHandle<JS::Value> aValue)
{
  nsresult rv = nsContentUtils::WrapNative(aCx, aArgument, aValue);
  return NS_SUCCEEDED(rv);
}

} 

bool
ToJSValue(JSContext* aCx,
          nsresult aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  nsRefPtr<Exception> exception = CreateException(aCx, aArgument);
  return ToJSValue(aCx, exception, aValue);
}

} 
} 
