















#ifndef mozilla_dom_CallbackFunction_h
#define mozilla_dom_CallbackFunction_h

#include "mozilla/dom/CallbackObject.h"

namespace mozilla {
namespace dom {

class CallbackFunction : public CallbackObject
{
public:
  






  CallbackFunction(JSContext* cx, JSObject* aOwner, JSObject* aCallable,
                   bool* aInited)
    : CallbackObject(cx, aOwner, aCallable, aInited)
  {
    MOZ_ASSERT(JS_ObjectIsCallable(cx, aCallable));
  }

  JSObject* Callable() const
  {
    return Callback();
  }

  bool HasGrayCallable() const
  {
    
    return mCallback && xpc_IsGrayGCThing(mCallback);
  }

protected:
  explicit CallbackFunction(CallbackFunction* aCallbackFunction)
    : CallbackObject(aCallbackFunction)
  {
  }
};

} 
} 

#endif 
