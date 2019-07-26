





#include "mozilla/dom/CallbackInterface.h"
#include "jsapi.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsPrintfCString.h"

namespace mozilla {
namespace dom {

bool
CallbackInterface::GetCallableProperty(JSContext* cx, const char* aPropName,
                                       JS::MutableHandle<JS::Value> aCallable)
{
  if (!JS_GetProperty(cx, mCallback, aPropName, aCallable)) {
    return false;
  }
  if (!aCallable.isObject() ||
      !JS_ObjectIsCallable(cx, &aCallable.toObject())) {
    nsPrintfCString description("Property '%s'", aPropName);
    ThrowErrorMessage(cx, MSG_NOT_CALLABLE, description.get());
    return false;
  }

  return true;
}

} 
} 
