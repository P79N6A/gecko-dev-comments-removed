




#ifndef mozilla_dom_mobilemessage_MobileMessageDatabaseService_h
#define mozilla_dom_mobilemessage_MobileMessageDatabaseService_h

#include "nsIMobileMessageDatabaseService.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

class MobileMessageDatabaseService MOZ_FINAL : public nsIMobileMessageDatabaseService
{
private:
  ~MobileMessageDatabaseService() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILEMESSAGEDATABASESERVICE
};

} 
} 
} 

#endif
