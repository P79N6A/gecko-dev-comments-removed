














#ifndef mozilla_dom_CallbackInterface_h
#define mozilla_dom_CallbackInterface_h

#include "mozilla/dom/CallbackObject.h"

namespace mozilla {
namespace dom {

class CallbackInterface : public CallbackObject
{
public:
  
  explicit CallbackInterface(JSContext* aCx, JS::Handle<JSObject*> aCallback,
                             nsIGlobalObject *aIncumbentGlobal)
    : CallbackObject(aCx, aCallback, aIncumbentGlobal)
  {
  }

protected:
  bool GetCallableProperty(JSContext* cx, JS::Handle<jsid> aPropId,
                           JS::MutableHandle<JS::Value> aCallable);

};

} 
} 

#endif 
