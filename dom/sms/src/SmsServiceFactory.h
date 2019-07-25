




































#ifndef mozilla_dom_sms_SmsServiceFactory_h
#define mozilla_dom_sms_SmsServiceFactory_h

#include "nsCOMPtr.h"

class nsISmsService;

namespace mozilla {
namespace dom {
namespace sms {

class SmsServiceFactory
{
public:
  static already_AddRefed<nsISmsService> Create();
};

} 
} 
} 

#endif 
