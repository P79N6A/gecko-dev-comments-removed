




#ifndef mozilla_dom_sms_SmsService_h
#define mozilla_dom_sms_SmsService_h

#include "nsISmsService.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsService MOZ_FINAL : public nsISmsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISMSSERVICE
};

} 
} 
} 

#endif
