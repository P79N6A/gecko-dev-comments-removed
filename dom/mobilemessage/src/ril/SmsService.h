



#ifndef mozilla_dom_mobilemessage_SmsService_h
#define mozilla_dom_mobilemessage_SmsService_h

#include "nsISmsService.h"
#include "nsCOMPtr.h"
#include "nsIRadioInterfaceLayer.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

class SmsService : public nsISmsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISMSSERVICE
  SmsService();

protected:
  nsCOMPtr<nsIRadioInterfaceLayer> mRIL;
};

} 
} 
} 

#endif 
