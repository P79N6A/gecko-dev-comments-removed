




#ifndef mozilla_dom_mobilemessage_SmsServicesFactory_h
#define mozilla_dom_mobilemessage_SmsServicesFactory_h

#include "nsCOMPtr.h"

class nsISmsService;
class nsIMmsService;
class nsIMobileMessageDatabaseService;

namespace mozilla {
namespace dom {
namespace mobilemessage {

class SmsServicesFactory
{
public:
  static already_AddRefed<nsISmsService> CreateSmsService();
  static already_AddRefed<nsIMobileMessageDatabaseService> CreateMobileMessageDatabaseService();
  static already_AddRefed<nsIMmsService> CreateMmsService();
};

} 
} 
} 

#endif 
