





#include "mozilla/dom/CallbackInterface.h"
#include "jsapi.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsPrintfCString.h"

namespace mozilla {
namespace dom {

bool
CallbackInterface::GetCallableProperty(JSContext* cx, JS::Handle<jsid> aPropId,
                                       JS::MutableHandle<JS::Value> aCallable)
{
  if (!JS_GetPropertyById(cx, CallbackPreserveColor(), aPropId, aCallable)) {
    return false;
  }
  if (!aCallable.isObject() ||
      !JS_ObjectIsCallable(cx, &aCallable.toObject())) {
    char* propName =
      JS_EncodeString(cx, JS_FORGET_STRING_FLATNESS(JSID_TO_FLAT_STRING(aPropId)));
    nsPrintfCString description("Property '%s'", propName);
    JS_free(cx, propName);
    ThrowErrorMessage(cx, MSG_NOT_CALLABLE, description.get());
    return false;
  }

  return true;
}

} 
} 
