




#ifndef mozilla_dom_sms_SmsServicesFactory_h
#define mozilla_dom_sms_SmsServicesFactory_h

#include "nsCOMPtr.h"

class nsISmsService;
class nsIMobileMessageDatabaseService;

namespace mozilla {
namespace dom {
namespace sms {

class SmsServicesFactory
{
public:
  static already_AddRefed<nsISmsService> CreateSmsService();
  static already_AddRefed<nsIMobileMessageDatabaseService> CreateMobileMessageDatabaseService();
};

} 
} 
} 

#endif 
