





#include "mozilla/dom/CallbackInterface.h"
#include "jsapi.h"
#include "mozilla/dom/BindingUtils.h"

namespace mozilla {
namespace dom {

bool
CallbackInterface::GetCallableProperty(JSContext* cx, const char* aPropName,
				       JS::Value* aCallable)
{
  if (!JS_GetProperty(cx, mCallback, aPropName, aCallable)) {
    return false;
  }
  if (!aCallable->isObject() ||
      !JS_ObjectIsCallable(cx, &aCallable->toObject())) {
    ThrowErrorMessage(cx, MSG_NOT_CALLABLE);
    return false;
  }

  return true;
}

} 
} 
