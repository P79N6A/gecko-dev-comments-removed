




































#ifndef mozilla_dom_sms_SmsService_h
#define mozilla_dom_sms_SmsService_h

#include "nsISmsService.h"
#include "nsCOMPtr.h"
#include "nsITelephone.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsService : public nsISmsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISMSSERVICE
  SmsService();

protected:
  nsCOMPtr<nsITelephone> mRIL;
};

} 
} 
} 

#endif 
