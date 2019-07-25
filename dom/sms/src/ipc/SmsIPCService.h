




































#ifndef mozilla_dom_sms_SmsIPCService_h
#define mozilla_dom_sms_SmsIPCService_h

#include "nsISmsService.h"
#include "nsISmsDatabaseService.h"

namespace mozilla {
namespace dom {
namespace sms {

class PSmsChild;

class SmsIPCService : public nsISmsService
                    , public nsISmsDatabaseService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISMSSERVICE
  NS_DECL_NSISMSDATABASESERVICE

private:
  static PSmsChild* GetSmsChild();
  static PSmsChild* sSmsChild;
};

} 
} 
} 

#endif 
