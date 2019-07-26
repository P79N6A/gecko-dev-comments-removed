




#ifndef mozilla_dom_sms_MobileMessageDatabaseService_h
#define mozilla_dom_sms_MobileMessageDatabaseService_h

#include "nsISmsDatabaseService.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
namespace sms {

class MobileMessageDatabaseService MOZ_FINAL : public nsIMobileMessageDatabaseService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILEMESSAGEDATABASESERVICE
};

} 
} 
} 

#endif
