




#ifndef mozilla_dom_mobilemessage_SmsIPCService_h
#define mozilla_dom_mobilemessage_SmsIPCService_h

#include "nsISmsService.h"
#include "nsIMmsService.h"
#include "nsIMobileMessageDatabaseService.h"
#include "nsIObserver.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

class PSmsChild;

class SmsIPCService MOZ_FINAL : public nsISmsService
                              , public nsIMmsService
                              , public nsIMobileMessageDatabaseService
                              , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISMSSERVICE
  NS_DECL_NSIMMSSERVICE
  NS_DECL_NSIMOBILEMESSAGEDATABASESERVICE
  NS_DECL_NSIOBSERVER

  SmsIPCService();

private:
  uint32_t mMmsDefaultServiceId;
  uint32_t mSmsDefaultServiceId;
};

} 
} 
} 

#endif 
