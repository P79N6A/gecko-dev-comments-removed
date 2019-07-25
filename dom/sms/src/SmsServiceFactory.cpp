




































#include "SmsServiceFactory.h"
#include "nsXULAppAPI.h"
#include "SmsService.h"
#include "SmsIPCService.h"

namespace mozilla {
namespace dom {
namespace sms {

 already_AddRefed<nsISmsService>
SmsServiceFactory::Create()
{
  nsCOMPtr<nsISmsService> smsService;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    smsService = new SmsIPCService();
  } else {
    smsService = new SmsService();
  }

  return smsService.forget();
}

} 
} 
} 
