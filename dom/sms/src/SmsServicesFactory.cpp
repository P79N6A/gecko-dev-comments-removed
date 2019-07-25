




































#include "SmsServicesFactory.h"
#include "nsXULAppAPI.h"
#include "SmsService.h"
#include "SmsDatabaseService.h"
#include "SmsIPCService.h"

namespace mozilla {
namespace dom {
namespace sms {

 already_AddRefed<nsISmsService>
SmsServicesFactory::CreateSmsService()
{
  nsCOMPtr<nsISmsService> smsService;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    smsService = new SmsIPCService();
  } else {
    smsService = new SmsService();
  }

  return smsService.forget();
}

 already_AddRefed<nsISmsDatabaseService>
SmsServicesFactory::CreateSmsDatabaseService()
{
  nsCOMPtr<nsISmsDatabaseService> smsDBService;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    smsDBService = new SmsIPCService();
  } else {
    smsDBService = new SmsDatabaseService();
  }

  return smsDBService.forget();
}

} 
} 
} 
