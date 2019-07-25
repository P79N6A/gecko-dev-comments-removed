




































#ifndef mozilla_dom_sms_SmsIPCService_h
#define mozilla_dom_sms_SmsIPCService_h

#include "nsISmsService.h"

namespace mozilla {
namespace dom {
namespace sms {

class PSmsChild;

class SmsIPCService : public nsISmsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISMSSERVICE

private:
  static PSmsChild* GetSmsChild();
  static PSmsChild* sSmsChild;
};

} 
} 
} 

#endif 
