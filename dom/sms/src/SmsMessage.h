




































#ifndef mozilla_dom_sms_SmsMessage_h
#define mozilla_dom_sms_SmsMessage_h

#include "mozilla/dom/sms/PSms.h"
#include "nsIDOMSmsMessage.h"
#include "nsString.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsMessage : public nsIDOMMozSmsMessage
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSMESSAGE

  SmsMessage(const SmsMessageData& aData);

  const SmsMessageData& GetData() const;

private:
  
  SmsMessage();

  SmsMessageData mData;
};

} 
} 
} 

#endif 
