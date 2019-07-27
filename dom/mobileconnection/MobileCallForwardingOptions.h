



#ifndef mozilla_dom_MobileCallForwardingOptions_h
#define mozilla_dom_MobileCallForwardingOptions_h

#include "nsIMobileCallForwardingOptions.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
namespace mobileconnection {

class MobileCallForwardingOptions final : public nsIMobileCallForwardingOptions
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILECALLFORWARDINGOPTIONS

  MobileCallForwardingOptions(bool aActive, int16_t aAction,
                              int16_t aReason, const nsAString& aNumber,
                              int16_t aTimeSeconds, int16_t aServiceClass);

private:
  
  MobileCallForwardingOptions() {}

  ~MobileCallForwardingOptions() {}

  bool mActive;
  int16_t mAction;
  int16_t mReason;
  nsString mNumber;
  int16_t mTimeSeconds;
  int16_t mServiceClass;
};

} 
} 
} 

#endif 