




#ifndef mozilla_dom_mobilemessage_SmsService_h
#define mozilla_dom_mobilemessage_SmsService_h

#include "nsISmsService.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

class SmsService MOZ_FINAL : public nsISmsService
{
private:
  ~SmsService() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISMSSERVICE
};

} 
} 
} 

#endif
