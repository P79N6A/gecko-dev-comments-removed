



#ifndef mozilla_dom_mobilemessage_MobileMessageService_h
#define mozilla_dom_mobilemessage_MobileMessageService_h

#include "mozilla/Attributes.h" 
#include "nsIMobileMessageService.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

class MobileMessageService MOZ_FINAL : public nsIMobileMessageService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILEMESSAGESERVICE

  MobileMessageService() { MOZ_COUNT_CTOR(MobileMessageService); }

private:
  
  ~MobileMessageService() { MOZ_COUNT_DTOR(MobileMessageService); }
};

} 
} 
} 

#endif 
