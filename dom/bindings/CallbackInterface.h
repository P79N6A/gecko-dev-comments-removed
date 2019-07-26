














#ifndef mozilla_dom_CallbackInterface_h
#define mozilla_dom_CallbackInterface_h

#include "mozilla/dom/CallbackObject.h"

namespace mozilla {
namespace dom {

class CallbackInterface : public CallbackObject
{
public:
  






  CallbackInterface(JSContext* cx, JSObject* aOwner, JSObject* aCallback,
                   bool* aInited)
    : CallbackObject(cx, aOwner, aCallback, aInited)
  {
  }

};

} 
} 

#endif 
