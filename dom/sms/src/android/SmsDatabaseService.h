




#ifndef mozilla_dom_sms_SmsDatabaseService_h
#define mozilla_dom_sms_SmsDatabaseService_h

#include "nsISmsDatabaseService.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsDatabaseService MOZ_FINAL : public nsISmsDatabaseService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISMSDATABASESERVICE
};

} 
} 
} 

#endif
