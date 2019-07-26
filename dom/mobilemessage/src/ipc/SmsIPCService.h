




#ifndef mozilla_dom_mobilemessage_SmsIPCService_h
#define mozilla_dom_mobilemessage_SmsIPCService_h

#include "nsISmsService.h"
#include "nsIMobileMessageDatabaseService.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

class PSmsChild;

class SmsIPCService MOZ_FINAL : public nsISmsService
                              , public nsIMobileMessageDatabaseService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISMSSERVICE
  NS_DECL_NSIMOBILEMESSAGEDATABASESERVICE

private:
  static PSmsChild* GetSmsChild();
};

} 
} 
} 

#endif 
