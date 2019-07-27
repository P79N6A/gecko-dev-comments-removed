



#ifndef mozilla_dom_payment_PaymentProviderEnabler_h
#define mozilla_dom_payment_PaymentProviderEnabler_h

#include "jsapi.h"

struct JSContext;
class JSObject;

namespace mozilla {
namespace dom {
  
class PaymentProviderUtils
{
public:
  static bool EnabledForScope(JSContext*, JSObject*);
};

} 
} 

#endif 
