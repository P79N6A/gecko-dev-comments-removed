















#ifndef mozilla_dom_CallbackFunction_h
#define mozilla_dom_CallbackFunction_h

#include "mozilla/dom/CallbackObject.h"

namespace mozilla {
namespace dom {

class CallbackFunction : public CallbackObject
{
public:
  explicit CallbackFunction(JS::Handle<JSObject*> aCallable,
                            nsIGlobalObject* aIncumbentGlobal)
    : CallbackObject(aCallable, aIncumbentGlobal)
  {
  }

  JS::Handle<JSObject*> Callable() const
  {
    return Callback();
  }

  bool HasGrayCallable() const
  {
    
    return mCallback && JS::ObjectIsMarkedGray(mCallback);
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
