















#ifndef mozilla_dom_CallbackFunction_h
#define mozilla_dom_CallbackFunction_h

#include "mozilla/dom/CallbackObject.h"

namespace mozilla {
namespace dom {

class CallbackFunction : public CallbackObject
{
public:
  explicit CallbackFunction(JSObject* aCallable)
    : CallbackObject(aCallable)
  {
    MOZ_ASSERT(JS_ObjectIsCallable(nullptr, aCallable));
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
